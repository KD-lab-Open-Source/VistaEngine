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

### The minimap's rotation — three fields that drifted apart

Maelstrom's worlds are **2048×4096** — twice as deep as they are wide — and turn the minimap
90° so it fits a landscape panel. Getting that back needs three separate pieces, and any two
of them leave it portrait:

- **`minimapAngle` changed owner.** It was an `Environment` field and is a `Universe` one
  now, so a pre-2008 world writes it in its `environment` block rather than in `universe`,
  and `Universe::serialize` never sees it. `Environment` is deserialized first (inside
  `Universe::Universe`), so the conversion reads it there and hands it across.
- **`getAngleFromWorld` became unreachable.** It and `rotateByCamera` used to be independent
  flags written side by side; 2008 made the second exclusive with a new
  `rotateByCameraInitial` and put `getAngleFromWorld` in the `else`. Maelstrom's data sets
  both, so the current reader takes the `if` and never asks for the angle at all.
- **`rotationScale` did not exist.** Rotating the map always rescaled it to fit the control,
  and `UI_Minimap::reposition` now does that only when the flag is set. Without it the quad
  spins inside a box still fitted to the *unrotated* 1:2 world.

`getAngleFromWorld` appearing next to a true `rotateByCamera` is a combination the current
writer cannot produce, so it is the marker for old data and what the `rotationScale` default
keys off. Perimeter 2 reads `rotateByCameraInitial` instead and is untouched — measured:

```
MAEL: rotByCam=1 init=0 fromWorld=1 rotScale=1 ctlAngle=0 worldAngle=90
P2:   rotByCam=1 init=1 fromWorld=0 rotScale=0 ctlAngle=0 worldAngle=0
```

Reading order does not matter here: `XPrmIArchive::openNode` rewinds to the start of the
block and rescans (twice) before giving up, so a field can be asked for out of order.

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
8. **The main menu renders white.** The black screen above was the unread trigger chain and
   is fixed; the menu mission now loads (`Universe created`) and the first frames draw, but
   the screen then turns white. Under investigation.
9. **`OPTION_SCREEN_SIZE` means a different resolution.** It is an *index*, and the list it
   indexes is C++ (`Game/GameOptionsSerialization.cpp:48`) — the `comment` string beside it
   in the data is only a label. Maelstrom's saved index 25 is 1920×1080 in Maelstrom's list;
   in ours, after `filterBaseGraphOptions` drops the modes the display does not support, it
   lands somewhere else entirely (1600×900 here). Harmless in itself, but it changes the
   window aspect, and with it which branch of the letterbox/pillarbox code runs — Perimeter 2
   at its own default of 1280×1024 never takes the wide branch that Maelstrom then does.
   Belongs in the converter, which would have to renumber the index against our list.

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
