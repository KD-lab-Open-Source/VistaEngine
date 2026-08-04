# Running Maelstrom's data on this engine

A sibling to the other registers, but a different kind of problem. Nothing here is about
porting a subsystem off a Windows API — it is about a *second game's* content, shipped on
a different revision of this same engine, and what it takes to load it.

**Maelstrom** (KD Lab, 2007) runs on VistaEngine. So does Perimeter 2. They are not the
same VistaEngine.

## Why the data does not simply work

Maelstrom's content was authored by a build from **before ~August 2007**; this tree is a
**2008+ snapshot**. Roughly a year of schema drift sits between them, and it is visible in
the data itself:

- Maelstrom's `StringTable` entries key on `first`/`second`; ours on `name`/`type`. The
  rename is dated in our own source — `"|name|first"` / `"|type|second"` with
  `// CONVERSION 31.07.07` (`Util/Serialization/StringTableBase.cpp:7`,
  `StringTableImpl.h:570`).
- Our tree carries CONVERSION markers dated `2008-1-18` and `2008-1-24`
  (`Util/TextDB.cpp:29`, `Terra/TerToolCtrl.h:61`); the shipped P2 data files are dated
  2008-10-31.
- Maelstrom's data still names `ActionReseCurrentMission` — the typo — for which we carry
  a `REGISTER_CLASS_CONVERSION` alias (`Game/Actions.cpp:79`).

`XPrmIArchive` already tolerates most of that gap on its own: `openNode` skips field names
it does not know, missing fields keep their defaults, unknown enum values and unregistered
polymorphic classes hit `xassertStr` and carry on. What it cannot absorb is a field whose
**C++ type** changed underneath the name — a float literal read into an integral field
leaves `strtol` (`XLibs.Net/XUtil/XBUFFER/XBCNVOUT.CPP:80`) parked on the `.`, and
`closeNode` then demands a `;` and aborts.

That abort is the whole reason the converter exists.

## Two ways the data drifts, two mechanisms

Drift comes in two shapes, and they are dealt with in different places.

**A field's *type* changed under its name.** Rewriting the value is the whole fix, so the
data is rewritten once, offline, by `tools/maelstrom_convert.py`. See below.

**A field changed *nesting* or *owner*.** There is nothing to rewrite and nothing for the
archive to skip: the reader asks for a block the old writer never wrote, and the entire
subtree behind that name goes unread — every control's show modes, a world's whole
lighting, all of its sources. Fixing that means reading a different shape, and a single
body of code cannot do it without guessing at load time which game it is looking at.

So those sites are written twice and chosen at **compile time**:

```
cmake -S . -B build-mael -DMAELSTROM_DATA=ON
```

`#ifdef MAELSTROM_DATA` selects the pre-2008 layout, `#else` keeps the 2008 one, and a
stock build is the code that was already there — Perimeter 2 carries no Maelstrom
branches, pays for no fallback lookups, and cannot be changed by a bug in one. The cost is
that a binary reads one game's data or the other's, not both. `grep -rn MAELSTROM_DATA` is
the full list; each site is a section below.

This replaced an earlier attempt at runtime detection (read the 2008 name, fall back to the
old one when it is missing). It works, and for one or two fields it is tidy, but it does not
scale: a name the data does not carry costs `openNode` a rescan of the whole enclosing
block — three passes, children and all — so the fallbacks are paid for by the game that
does not need them, on every control in the library. And some of the drift cannot be
detected that way at all: sources have to be read in one place and switched on in another,
which is a different *call graph*, not a different field name.

### Finding the type changes

Diffing the two data sets by field *name* does not work — it conflates unrelated structs.
It reported `SourcesLibrary`'s `r/g/b/a` as int-vs-float when those are a `Color4f`
keyframe ramp on one side and a `Color4c` elsewhere on the other, and flagged `sColor4c`
against `Color4c` when they are the same struct under two spellings (both write `r = 255`).

What does work: pair every `ar.serialize(member, "wire", ...)` call site with the
declaration of `member`, in **both** trees, and compare the resulting types. Extract the
Maelstrom tree with `git archive origin/Maelstrom | tar -x -C <dir>` and run the two
through the same scanner. Across the modules this build compiles that resolves ~1,500 wire
names per side and yields exactly **two** real changes.

## The converter

`tools/maelstrom_convert.py` — byte-level (the files are CP1251 with CRLF and the engine
reads them verbatim, so nothing re-encodes or re-wraps a line it did not have to touch),
idempotent, and it breaks a symlink before writing so it can run over a farm laid on top of
a read-only pristine copy.

```
python3 tools/maelstrom_convert.py <MaelstromData> --font 'Resource\UI\Fonts\ARIALNB2.ttf' --apply
```

| what | why |
|---|---|
| `steering_duration` float → int | `RigidBodyPrm::steering_duration` is `float` in Maelstrom (`Physics/RigidBodyPrm.h:109`), `int` here |
| `groundPass` / `waterPass` `PASSABILITY`→`true`, `IMPASSABILITY`→`false` | `PassabilityFlags` there, `bool` here. `IMPASSABILITY = 0`, `PASSABILITY = 1`, and the two engines' constructed defaults agree exactly |
| generate `Scripts/Content/UI_FontAttributes` from `Scripts/Content/UI_FontLibrary` | see below |
| copy `Scripts/Content/GlobalTrigger.scr` to `Scripts/Content/Triggers/` | the chain moved into a subdirectory; see below |

43 of each of the first two, one per entry in `Scripts/Engine/RigidBodyPrmLibrary` — that
single file is the only one in the tree that needed rewriting.

### The global trigger chain is what starts the game

`GameShell::init` loads exactly one path, `Scripts\Content\Triggers\GlobalTrigger.scr`
(`GameShell.cpp:192`). Maelstrom's sits a directory up, at `Scripts\Content\`, and its
`Triggers\` holds only AI scripts — so the chain loaded empty.

That chain is not decoration. Its `START` trigger runs a `Hide Cursor` / `Start Main Menu`
sequence, and `Start Main Menu` carries the `ActionStartMission` that loads
`Resource\Worlds\Menu.spg` — Maelstrom's main menu, like Perimeter 2's, *is* a running
mission. With the file unread nothing ever fires: no mission, no screen selected, not one
control reaching `UI_ControlBase::redraw`. The window stays black, which reads as a renderer
fault and is not one.

All eight classes the chain names (`ActionStartMission`, `ActionShowReel`,
`ActionShowLogoReel`, `ActionSetCursor`, `ActionFreeCursor`, `ActionDelay`,
`ActionGameQuit`, `ConditionEventComing`) still exist here, so it runs as written once
found. The intro reels are skipped cleanly with `DisableVideo` — `ActionShowReel::workedOut`
returns true when video is off, so the chain advances rather than stalling.

Worth knowing when chasing this kind of thing: the actions are serialized as
`"struct ActionFoo"`, not `"class ActionFoo"`. Grepping for the latter finds two entries in
this file and suggests the chain is an empty skeleton.

### Fonts are a translation, not a substitution

Maelstrom rasterised its fonts offline. `Scripts/Content/UI_FontLibrary` names logical
fonts (`MAEL_small`) that resolve to glyph atlases in `cacheData/Fonts/*.xfont` + `.tga`,
and **the distribution contains no `.ttf` at all**. By 2008 the engine had moved to
FreeType reading `Scripts/Content/UI_FontAttributes` (`UserInterface/UI_Types.cpp:68`) — a
different file name *and* a different shape: ours is a `StringTable<UI_LibFont>` with a
nested `font` block, Maelstrom's a `StringTableBasePolymorphic` holding
`second = "class UI_Font"`.

Without that file `cfont()` falls back to a default font that never created, and
`FT::Font::size(this=0x0)` faults inside `UI_TextParser::parseString`. The converter
translates the library and substitutes whatever TTF `--font` names. **The glyphs are not
Maelstrom's.** Matching those means teaching the engine to read `.xfont`, which is a code
change, not a conversion.

## What the engine had to learn

Every one of these is a *fallback*: it runs only when the normal path finds nothing, so on
this engine's own data the open simply fails and behaviour is unchanged.

### Models — `cStatic3dx::loadMaelstromCache`

Maelstrom ships **no `.3DX` for units or buildings**. There are 32 raw `.3DX` in the whole
distribution, all terrain-tool decals and sky helpers. The models exist only as 1896
`Resource\cacheData\baseCache\Models\<NAME>.3DX.dat` (1032 graph + 864 `.logic.dat`).

That cache is **the same chunked container as a `.3DX`, written with the same `C3DX_*` id
table** (`cStatic3dx::saveCacheData` in the Maelstrom tree). All 1896 parse as a top-level
chunk stream and 1895 share one 13-chunk layout. The `'lods'` chunk holds `'head'` (a
count), one sub-chunk per LOD *keyed by index*, then `'debr'`; each LOD is
`C3DX_SKIN_GROUPS` plus `C3DX_BUFFERS`{`HEAD`, `BUFFER_VERTEX`, `BUFFER_INDEX`} carrying
raw VB/IB bytes — the same information as our `LodCache`, and easier to read, being
self-describing rather than a 32-bit memory image.

The vertex layout did not change: `cSkinVertex`'s offsets are identical between the two
engines and ours only appends the optional fur field Maelstrom never had, so the buffers go
to `StaticLod::initBuffersInPlace` verbatim. The chunk-id tables agree on 83 of 92 ids and
**no id ever means two different things**; the reference table is
`origin/Maelstrom:IGameExporter3/3dx.h`.

Traps worth knowing:

- `LoadInternal` **can** be reused for the chains block, nodes, camera, lights and
  materials. `LoadChainData` **cannot**: baking flattened the animation chunks, so
  `C3DX_ANIMATION_GROUP` holds a plain record where a `.3DX` nests sub-chunks under the
  *same id*, and feeding it to `LoadChainData` walks off the end of the block.
- Do **not** point `fileName_` at the cache file. `LoadTexture` and `fixTextureName`
  resolve textures as `<model dir>\Textures\`, so it has to keep naming the model.
- `prepareMesh()` must not run — the geometry is already baked — but the bookkeeping it
  does still has to happen: every visibility group needs its `visibility` bit and its
  `visibleNodes` flags, or `cObject3dx::Update` indexes an empty vector and faults.
- `cTempVisibleGroup` has a fifth field here (`visibilityNodeIndex`) that Maelstrom's
  record does not carry.

### Textures — `cTexLibrary::loadBaseCache`

Shipped the same way: no source images, only
`Resource\cacheData\baseCache\Textures\<detail>\<NAME>.DDS`. The cached files are ordinary
DDS, the three numbered directories are exactly `Option_TextureDetailLevel`, and the name
is the same transformation as models — the texture's own path, already upper-cased by
`normalizePath`, with separators turned into underscores. `cTexture::loadDDS` sets the
dimensions itself. Hooked where `texture->reload()` fails; without it everything renders
white.

### Terrain colour — the `S5L2` branch of `vrtMap::loadVMP`

Maelstrom's `cache.vmp` is `S5L2`; P2's is `S5L4`. `loadVMP` already handled all of
`S2T0`/`S5L2`/`S5L3`/`S5L4`, but `S5L2` stores **one palette index per cell** where this
engine's `clrBuf` holds RGB565 outright. The existing conversion from that shape needs
`supBuf`, which is only allocated when `flag_useTryColorBuffer` is on — and it is not at run
time, so the branch fell through to a release-mode no-op assert and left `clrBuf` zeroed.
It now reads the indices and puts them through the world's own `inDam.act` palette, the
same lookup `getColor16T` does for a byte-wide `clrBuf`.

### The interface frame — `UI_BackgroundScene::serialize`

The in-game HUD chrome is **not** UI sprites. It is a 3D model, one per race
(`Remnant interf.3DX`), drawn by `UI_BackgroundScene` through a camera of its own, with the
buttons, minimap and text laid over it as ordinary 2D controls. So when it is misplaced the
2D layout is still exactly right, which sends you looking in the wrong file — `screenCoords`,
`aspectedWorkArea` and `UI_RenderBase` are identical in the two engines, and the controls
land within a pixel of where the original puts them.

That camera used to be described in the data, once for the whole scene:

```
position = { x = 0.; y = -130.5; z = 1000.; };
focusx = 1.8;
perspective = true;
```

By 2008 all three were gone: the focus became a per-model `scale` (× `scale2focus`), the
camera became orthographic, and a new `modelPosition_` pushed the model 1024 units off the
origin. The read of the old block had been left commented out, so Maelstrom's model was
drawn orthographic, at Perimeter 2's focus, at twice the distance — too large and too high.
Perimeter 2's own `backgroundScene` ends after `lights` and carries none of the three, so
reading them back costs it nothing.

Two traps, both settled by reading `origin/Maelstrom:UserInterface/UI_BackgroundScene.cpp`:

- The focus is useless without the placement. Honouring `focusx` alone swaps one wrong size
  for another, because the perspective divide then happens at the 2008 distance.
- Only the **translation** is new. `modelAngles_(90,0,0)` is the same constant in both
  engines — old `selectModel` built its matrix from the rotation and `Vect3f::ZERO`. Zeroing
  the rotation too makes the model vanish edge-on.

### Control show modes moved a level deeper — `UI_ControlState::serialize`

The single largest visual difference, and the one that looks least like a schema problem.
Pre-2008 a control state wrote its show modes **flat**, one field per
`UI_ControlShowModeID` name, inside a transparent `openBlock("")`:

```
{ name_ = "Default"; UI_SHOW_NORMAL = "class UI_ControlShowMode" { sprite_ = { ... } }; }
```

By 2008 the same table had moved under a named block, `showModes = { UI_SHOW_NORMAL = … }`.
The contents are byte-identical either way — `EnumTable::serialize` is what wrote them then
and what writes them inside the block now — but our reader asks for the block, `openStruct`
fails, and it moves on. Every control in Maelstrom's data therefore loaded with an **empty
show-mode table**, and a control with no show mode draws no sprite at all.

That is what made the interface look like a texture-loading fault: blank menus, white boxes
where the HUD's resource icons belong, a white panel where the minimap belongs. No texture
ever failed to load — none was ever asked for. The fix is to read the table at the state's
own level when the block is absent.

Worth checking early on anything that renders blank rather than wrong: `showMode()` returning
null is indistinguishable at a glance from a missing texture, and the two lead to opposite
places.

### The start screen — four fields on every control — `UI_ControlBase::serialize`

With the show modes read, the start screen came up as a **white sheet with untitled buttons**.
Neither is a renderer fault; four fields of `UI_ControlBase` drifted, and each is a different
*kind* of drift, so they are worth keeping apart:

| pre-2008 | now | what breaks when it is not read |
|---|---|---|
| `locText = { key; id; }` | `text` (literal) | every caption is blank |
| `borderColor` — `sColor4f` | `borderClr` — `Color4c` | fills paint at the constructed opaque white |
| `borderOutlineColor` — `sColor4f` | `borderOutlineClr` — `Color4c` | outlines likewise |
| `borderEnabled` | *gone* | "keep the colours, don't draw" becomes a visible rectangle |
| `screenZ_` | `screenZ` | depth is 0 everywhere; the screen draws in list order |

- **Captions were a key, not a string.** A control named its text through the localization
  database and the engine resolved it at load (`TextDB::getText`); the literal `text_` beside
  it was only written when that key was empty. By 2008 the indirection was gone and
  `UI_Attributes` is itself per-language. Reading `locText` when neither literal is present
  restores every button label, screen title and prompt at once.
- **The border colours are a rename *and* a type change** — float RGBA 0…1 to byte RGBA
  0…255 — so neither the archive's name-skipping nor a plain re-read bridges it: pointing
  `Color4c::serialize` at `r = 0.321569` stops the parser mid-number and aborts the load with
  `Expected Token: ";", Received Token: ".321569"`. Read it into a `Color4f` and convert.
- **That is where the white screen came from.** Every screen has a full-screen `background`
  control whose only job is a `borderFill` dimming the world behind it to 30% black. The flag
  kept its name, the colour did not, so the fill drew — in opaque white.

All five are `#ifdef MAELSTROM_DATA`, and this section is why. Read once for both games, they
would be five extra name lookups on **every control in the library**, and a name the data does
not carry is not free: `openNode` scans to the end of the block, rewinds and rescans twice
before giving up, and a control's block contains all of its children — so a failed lookup on a
container costs a walk of its whole subtree. `borderEnabled` is the only one that could be
cheaply gated (it is asked for only when a border is switched on at all); the other four are
asked on every control or not at all.

### The world's own lighting — `Environment::serialize` / `EnvironmentTime::serializeMaelstrom`

Everything the engine lights a world with — the sun, sky, fog and shadow gradients, the sky
models, the latitude and slant of the sun, the time of day — used to be written **flat** in
the world's `environment` block. 2008 moved the lot under an `environmentTime` sub-block, so
`openStruct` fails and a pre-2008 world lights itself entirely from constructed defaults.

| the world asks for | it got instead |
|---|---|
| `dayTime = 9.2` | 14.0 — the sun in a different place in the sky |
| `latitude_angle = 11`, `slant_angle = 5` | 36, 0 |
| `shadowing = { 0.5, 0.5, 1.0 }` | 0.75, **0.2**, 1.0 |
| its own 8-key `sun_color` ramp | the built-in default ramp |
| `Sky_Clouds_Day_Default_90grad.3DX` + stars + night clouds | one default cloud layer |

The visible half of that is `ambient_maximal`. The terrain survives it, because its colour is
baked per fine cell in the height map and only modulated by the light — but an object is lit
from the sun alone, so at ambient 0.2 every surface facing away from it, the whole shaded side
of a tower block, comes out near black. Measured over matching patches of the same shot, the
terrain came in at 0.42× the original's brightness and the shaded tower at 0.06×.

Two smaller things ride along in the same reader:

- **`objectShadowing` did not exist.** One `ShadowingOptions` lit the ground and the objects
  standing on it alike; 2008 split it in two. The conversion gives the objects the ground's
  numbers, which is what the old engine did.
- **`global_<name>_color` is not read.** Each gradient is preceded in the file by a flag
  saying the world defers to a global set (in its own `environmentColors` block, or in
  `Scripts\Content\GlobalAttributes` when it declared none). It can be ignored: the writer
  resolved the flag before saving, so the copy sitting in the world is already the global
  gradient — `c1_m1.spg` sets all six flags and carries `Scripts\Content\GlobalAttributes`'
  11-key `sun_color` verbatim.

Perimeter 2 writes the block, and its build compiles the `#else`, so nothing here is on its
path at all.

### The minimap's rotation — three fields that drifted apart

Maelstrom's worlds are **2048×4096** — twice as deep as they are wide — and turn the minimap
90° so it fits a landscape panel. Getting that back needs three separate pieces, and any two
of them leave it portrait:

- **`minimapAngle` changed owner.** It was an `Environment` field and is a `Universe` one
  now, so a pre-2008 world writes it in its `environment` block rather than in `universe`,
  and `Universe::serialize` never sees it. `Environment` reads it there and hands it across —
  which works in either load order, the two objects both existing by then.
- **`getAngleFromWorld` became unreachable.** It and `rotateByCamera` used to be independent
  flags written side by side; 2008 made the second exclusive with a new
  `rotateByCameraInitial` and put `getAngleFromWorld` in the `else`. Maelstrom's data sets
  both, so the current reader takes the `if` and never asks for the angle at all.
- **`rotationScale` did not exist.** Rotating the map always rescaled it to fit the control,
  and `UI_Minimap::reposition` now does that only when the flag is set. Without it the quad
  spins inside a box still fitted to the *unrotated* 1:2 world.

`getAngleFromWorld` appearing next to a true `rotateByCamera` is a combination the 2008 writer
cannot produce — it was the marker for old data while this was detected at runtime, and it is
worth keeping in mind if you are ever staring at a world file wondering which schema it is.
Perimeter 2 reads `rotateByCameraInitial` instead and is untouched — measured:

```
MAEL: rotByCam=1 init=0 fromWorld=1 rotScale=1 ctlAngle=0 worldAngle=90
P2:   rotByCam=1 init=1 fromWorld=0 rotScale=0 ctlAngle=0 worldAngle=0
```

Reading order does not matter here: `XPrmIArchive::openNode` rewinds to the start of the
block and rescans (twice) before giving up, so a field can be asked for out of order.

### The world's sources — and the load order that comes with them

A world's **sources** are the zones that hold its standing effects, its damage areas and its
unit generators. Pre-2008 they were written in the `environment` block; 2008 split
`SourceManager` out of `Environment` and moved them into `Universe`'s `sourceManager` block.
Unread, every *placed* effect in a world is simply absent — in Maelstrom's menu that is each
building fire, every smoke column and the green outflow from the pipe, all of them
`SourceZone`s.

This one is worth reading closely because it looked, for a long time, like a renderer fault
(see the old register item 11, now deleted). The `.effect` files all load, the world builds
205 `cEffect`s, and the particle renderer is reached thousands of times a frame by the sun,
the moon and the coast foam — so every measurement short of *which texture reached
`SetMaterial`* said the sprites were being submitted and dropped. They were never created.

The read is called **inline**, not through a named block: `sources` and `anchors` sit at the
environment's own level. And it forces a **load-order swap**, which is the part that cannot
be expressed as a different field name:

```
2008:       environment, camera, universe
Maelstrom:  camera, universe, environment
```

A source refers to units — its owner, its targets, the squads a generator fills — and
`SourceManager::serialize` switches each one on as it finishes reading it. Read the
environment first and those references resolve to nothing; the first quant then goes down on
a legionary whose squad never existed. Maelstrom's own engine read the environment last, and
its data depends on that.

Two engine bugs surfaced behind this one, both in code shared with Perimeter 2 and both fixed
unconditionally:

- **`if(&*contextUnit_)`** in `ActionAIUnitCommand::activate` — a null test written by taking
  the address back off a dereferenced `UnitLink`. Dereferencing null is undefined, so clang at
  `-O2` folds the test away; MSVC kept it, which is why the original never saw it. The trigger
  chain in Maelstrom's menu kills the unit in `executeCommand`, and the next line then called
  `setUsedByTrigger` on a null `this`. The only `&*` used as a null test in the tree.
- **`StateTouchDown` cast to `UnitLegionary*`.** Anything born in the air lands in that state,
  and `SourceZone::generateUnits` drops buildings as well as legionaries. `makeStaticXY` /
  `makeDynamicXY` are `UnitActing`'s — virtual, empty there, overridden only on
  `UnitLegionary`, which is the author saying a unit with nothing to pin in XY passes through.
  `StateBirthInAir` already casts to `UnitActing*` for the same pair; `StateTouchDown` did not,
  so `safe_cast` handed back null. Retail Perimeter 2 only ever drops legionaries here.

A third guard is genuinely shared: `UnitLegionary::Quant` assumed a squad. `squad_` is a
`UnitLink` and `UnitSquad::removeUnit` clears it before deciding whether anything is left to
kill, so a legionary can outlive its squad — which Maelstrom's worlds reach, their source
zones damaging what stands in them.

### The preset group — the world's own `environmentColors`

One object used to own it: `EnvironmentAttributes`, written as the world's `environmentColors`
node. 2008 dissolved that object — its fields became `Environment`'s own, its fog-of-war
colours `FogOfWar`'s, two of its constants `cWater`'s — and moved the group out of the world
and into a preset file, `Scripts\Content\Presets\global.set`.

Maelstrom ships no `Scripts\Content\Presets\` at all, so `loadPreset()` opened nothing and the
whole `if(ar.filter(SERIALIZE_PRESET_DATA))` branch of `Environment::serialize` never ran, on
any world: fog at 1000–1400 where `Menu.spg` asks for 900–1200, a 30–4000 camera frustum where
it asks for 2–1300, and no weather, shore, lens flare or underwater effect at all.

Two separate things had to change, because the group is split across two depths:

- **The branch has to run during a world load**, not only from `loadPreset()`. Most of the
  group — `fog_enable`, the underwater, bloom and DOF settings, `outside`, `lensFlare_`,
  `fallLeaves`, the ice and chaos textures, the cloud shadow — is written flat in the world's
  `environment` block, exactly where this already reads it.
- **The rest needs a real descent.** `openBlock` is a no-op in an XPrm archive — editor-only
  grouping, it does not nest — and `openNode`'s rescan skips a nested block whole
  (`skipValue` counts braces), so from the environment's level not one name inside
  `environmentColors` is visible. `openStruct` descends for real.

`ownAttributes` decides which copy to read: 11 of the 51 worlds carry their own node, the
other 40 taking the global one. Maelstrom kept that in `Scripts\Content\GlobalAttributes` —
that file **is** its preset file, and `loadPreset()` now reads its `environmentColors` for
exactly the worlds that ask for it, which is what the original did with
`environmentAttributes_ = GlobalAttributes::instance().environmentAttributes_`.

One member is deliberately left unread: `timeColors_`, the node's own copy of the six sky
gradients. A world is not lit from it. Maelstrom lit from `EnvironmentTime`'s gradients,
written flat in the environment block, and reached into a `timeColors_` only for the *global*
set — its `Environment.cpp` has
`ReplaceGlobal(GlobalAttributes::instance().environmentAttributes_.timeColors_)`. What the
editor saved beside them inside the world is that global set: a 9-key ramp against `Menu.spg`'s
own 8-key one, so reading it would overwrite a world's own lighting with the global default.
`miniDetailTexResolution` has no reader in this engine at all.

Two names drifted rather than moved: `outside` was capitalised to `Outside` (taken with the
archive's own `|a|b` alias, so Perimeter 2 matches on the first name and pays nothing for the
second), and `FogOfWar`'s `fogColor` / `scoutAreaAlpha` were `fogOfWarColor` /
`scout_area_alpha`.

Measured on `Menu.spg` after the change: fog 900–1200, `height_fog_circle` 500, frustum
2–1300, `hideSmoothly` true, effects 0 / 0 / 1e6, `outside = ENVIRONMENT_WATER`, and the
underwater and ice textures resolving to real paths instead of empty strings.

### Silhouettes — `Camera::DrawSilhouetteObject`

Stencil work that was never ported. Retail Perimeter 2 never fills its draw list so it went
unnoticed; Maelstrom's data does use it and faulted on the null `gb_RenderDevice3D`. Guarded
off and registered as **Render-PORTING.md #22**.

## What the rest of the binary formats do

Mostly nothing — they already load:

| | Maelstrom | ours | |
|---|---|---|---|
| `cache.vmp` | `S5L2`, 2048×4096 | `S5L4`, 2048×2048 | handled, plus the colour fix above |
| `surkind.bin`, `cache.tga`, `GrassMap.tga`, `inDam.act` | | | same layouts, only map size differs |
| `world.cls` | text XPrm, 52 fields | 12 fields | **zero shape conflicts**; ours is a strict subset |
| `.spg` | text XPrm | | 201 common fields, one shape conflict (`time`) |
| `.tdb` | pre-2008 `TextDB` | `TextDB_unicode` | the CONVERSION fallback at `Util/TextDB.cpp:29` already re-encodes it |
| `.effect` | chunked Saver | same | same leading ids |
| `inGeo.act` | present | — | the "geo" feature P2 dropped; harmless extra file |

## Still open

1. **Visibility sets are approximated.** Maelstrom's per-LOD group indices
   (`C3DX_AVS_ONE_RAW` / `_LODS`) index a structure this tree replaced, so each set is
   collapsed to a single catch-all group and **parts the original hid per animation state
   are all shown at once**. First suspect for any model that looks wrong.
2. **Animation frame intervals are absent from the cache.** `StaticAnimationChain` keeps
   its constructed `begin_frame`/`end_frame`, so animation may not play correctly.
3. **Fonts are a substitute typeface.** Reading `.xfont` + its `.tga` atlas would restore
   the original lettering.
4. **Maelstrom-only `.spg` camera fields go unread** — `FarPlane`, `NearPlane`,
   `CAMERA_ZOOM_*`, `CAMERA_MAX_HEIGHT`, `CAMERA_MIN_HEIGHT` — so the camera uses P2
   defaults on larger maps. Not known to matter; not investigated.
5. **Ten polymorphic classes in Maelstrom's data do not exist in this source**, and resolve
   to null objects rather than failing: the `AiAction_*` / `AiCondition_*` action-chain
   system (matching its `Scripts/Engine/AiActionChainList`, which P2 has no equivalent of),
   plus `ActionSquadMove`, `ActionSetCoastSprites`, `AttributeReal` and
   `ConditionObjectNearObjectByLabel`.
6. **`C3DX_BASEMENT`** (500/501/502) — building foundation geometry, a Maelstrom feature P2
   dropped — is silently ignored by the chunk switch.
7. **Not every mission has been run.** `c1_m1` and `c1_m2` load; the rest are untested.
8. **Tooltips are missing.** A state used to carry its hover text directly
   (`hoveredTextLoc`, a localization key, read by `UI_ControlState::serialize`); 2008 moved
   it into a `UI_ACTION_HOVER_INFO` action. Maelstrom's data writes the old field, nothing
   reads it, and no control shows a tooltip. Not converted — the screens themselves are
   readable without it.
9. **`OPTION_SCREEN_SIZE` means a different resolution.** It is an *index*, and the list it
   indexes is C++ (`Game/GameOptionsSerialization.cpp:48`) — the `comment` string beside it
   in the data is only a label. Maelstrom's saved index 25 is 1920×1080 in Maelstrom's list;
   in ours, after `filterBaseGraphOptions` drops the modes the display does not support, it
   lands somewhere else entirely (1600×900 here). Harmless in itself, but it changes the
   window aspect, and with it which branch of the letterbox/pillarbox code runs — Perimeter 2
   at its own default of 1280×1024 never takes the wide branch that Maelstrom then does.
   Belongs in the converter, which would have to renumber the index against our list.
10. **Unit silhouettes are not drawn.** Stencil work that was never ported —
    **Render-PORTING.md #22**. Unreachable on retail Perimeter 2, so this build is the only
    way to exercise it.

## A note on judging behaviour

Fog of war is **on** in Maelstrom (41 of 51 missions) and **off** in Perimeter 2, whose
`GlobalAttributes` sets `enableFogOfWar = false` and whose `.spg` files omit the flag
entirely. Maelstrom's data omits the *global* flag, so our `true` default stands and the
per-mission flag takes effect. A good deal of this engine's fog-of-war code therefore runs
here for the first time on data that uses it.

That cuts both ways, and it is worth stating plainly: **unfamiliar-looking is not the same
as broken.** Buildings visibly rendering through the fog looked like a bug and was traced a
long way into `cScene::AddPlanarCamera`'s visible-box computation before the original
Windows build was checked — which does exactly the same thing. Compare against the original
before opening the renderer.
