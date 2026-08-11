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

### Setting up the run directory

Nothing in this tree builds it; the converter rewrites a data root, it does not create one.
Unpack the distribution and convert the copy:

```
unzip -q MaelstromEnhanced.zip -d GameData     # ~6.3 GB, Resource/ Scripts/ cacheData/
```

The alternative, when the disk matters, is a farm: mirror the directory tree of a read-only
pristine copy and symlink every file into it. That is the case the unlinking above is for —
the three files the converter touches become real, the rest stay links, and the pristine
copy is untouched. Verified by checksumming it across a full run.

The game writes `iniFile.cfg` itself on first start, so there is nothing else to seed.

### Running it

Run it over the **run directory**, not the pristine copy. Without `--apply` it only reports,
so the first pass is free:

```
$ python3 tools/maelstrom_convert.py ~/Projects/MaelstromEnhanced/GameData
  Scripts/Content/GameOptions                                            3
  Scripts/Engine/RigidBodyPrmLibrary                                     129

  Scripts/Content/UI_FontAttributes  <- UI_FontLibrary  (Aero 20 @16 MAEL_small, Aero big @32 MAEL_big, Aero medium @24 MAEL_big)

  Scripts/Content/Triggers/GlobalTrigger.scr  <- Scripts/Content/GlobalTrigger.scr

scanned 299 text files, 2 needed changes  (dry run -- pass --apply to write)
  steering_duration         43  -- RigidBodyPrm::steering_duration is float in Maelstrom, int here
  groundPass                43  -- RigidBodyPrm::groundPass is PassabilityFlags in Maelstrom, bool here
  waterPass                 43  -- RigidBodyPrm::waterPass is PassabilityFlags in Maelstrom, bool here
  OPTION_LANGUAGE            1  -- index into a C++ list that changed shape
  OPTION_SCREEN_SIZE         1  -- index into a C++ list that changed shape
  OPTION_SHADOW              1  -- index into a C++ list that changed shape
```

Then commit to it — nothing else has to be supplied, the fonts come from Maelstrom's own
masters (see below):

```
$ python3 tools/maelstrom_convert.py ~/Projects/MaelstromEnhanced/GameData --apply
  Scripts/Content/GameOptions                                            3
  Scripts/Engine/RigidBodyPrmLibrary                                     129

  Scripts/Content/UI_FontAttributes  <- UI_FontLibrary  (Aero 20 @16 MAEL_small, Aero big @32 MAEL_big, Aero medium @24 MAEL_big)

  Scripts/Content/Triggers/GlobalTrigger.scr  <- Scripts/Content/GlobalTrigger.scr

scanned 299 text files, 2 needed changes
```

Re-running is a no-op: every rule returns "already in our shape" for a value it has already
converted, and the two generated files are skipped once they exist — so a second pass over a
partly-converted tree changes nothing.

| what | why |
|---|---|
| `steering_duration` float → int | `RigidBodyPrm::steering_duration` is `float` in Maelstrom (`Physics/RigidBodyPrm.h:109`), `int` here |
| `groundPass` / `waterPass` `PASSABILITY`→`true`, `IMPASSABILITY`→`false` | `PassabilityFlags` there, `bool` here. `IMPASSABILITY = 0`, `PASSABILITY = 1`, and the two engines' constructed defaults agree exactly |
| generate `Scripts/Content/UI_FontAttributes` from `Scripts/Content/UI_FontLibrary`, pointing at Maelstrom's own `*.font` masters | see below |
| copy `Scripts/Content/GlobalTrigger.scr` to `Scripts/Content/Triggers/` | the chain moved into a subdirectory; see below |
| renumber `OPTION_SCREEN_SIZE` / `OPTION_SHADOW` / `OPTION_LANGUAGE` in `Scripts/Content/GameOptions` | they are indices into a C++ list that changed shape; see below |

43 of each of the first two, one per entry in `Scripts/Engine/RigidBodyPrmLibrary` — that
single file is the only one in the tree that needed rewriting.

### Indexed options point into a list that is not in the data

`Scripts/Content/GameOptions` stores an option as `number = <index>`, and the list it
indexes lives in **C++** (`Game/GameOptionsSerialization.cpp`), not beside it. The `comment`
string next to the number looks like the list but is only a label — the engine never reads
it. So an index written by one revision silently selects a different entry in the other, with
nothing to warn you:

| option | Maelstrom | means | in our list |
|---|---|---|---|
| `OPTION_SCREEN_SIZE` | 25 | `1920*1080` | index **27** — we added `858*484` and `1680*945` |
| `OPTION_SHADOW` | 3 | `High` | index **2** — Maelstrom's `Circle` shadows were dropped |
| `OPTION_LANGUAGE` | 0 | `Use Steam Language` | **0**, English — we have no auto-detect |

`OPTION_SCREEN_SIZE` is the one that shows. Index 25 here is `1680*945`, and after
`filterBaseGraphOptions` drops the modes the display cannot do, the window ends up at
something like 1600×900 — which changes the aspect, and with it which branch of the
letterbox/pillarbox code runs. It also scales the UI font: `UI_Font::createFont` computes
`fontSize_ * windowHeight / 768`, so a 16-point font at a 900-pixel window asks FreeType for
19. That is the `":19"` in a Windows 10 user's `UI_Font.cpp:80` assert.

The conversion is self-contained, because **the comment is Maelstrom's own list**: resolve the
index to a name against it, then look that name up in ours. The converter rewrites the comment
too, and that is what keeps the pass idempotent — leaving the old list in place would make a
second run resolve the *new* index against the *old* names and convert twice.

The remaining indexed options (`OPTION_ANTIALIAS`, `OPTION_ANISOTROPY`,
`OPTION_TEXTURE_DETAIL_LEVEL`, `OPTION_SOFT_SMOKE`) have identical lists in both games and are
left alone. Note that a bad *antialias* index is a different fault: those are filtered against
what the GPU reports at run time (`raw2filtered`), so no offline pass can settle them.

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

### Fonts are Maelstrom's own, not a substitute

Maelstrom rasterised its fonts offline and **ships no TrueType at all** — zero `.ttf` or
`.otf` in the distribution. `Scripts/Content/UI_FontLibrary` names logical fonts
(`Aero 20`) that resolve to a face (`MAEL_small`) and a pixel size; by 2008 the engine had
moved to FreeType reading `Scripts/Content/UI_FontAttributes`
(`UserInterface/UI_Types.cpp:68`) — a different file name *and* a different shape: ours is a
`StringTable<UI_LibFont>` with a nested `font` block, Maelstrom's a
`StringTableBasePolymorphic` holding `second = "class UI_Font"`.

The faces are **1-bit-per-pixel bitmap masters**, one file each, rasterised at a fixed height
and scaled to whatever size the UI asks for. `Render/src/BitmapFont.cpp` reads them, using the
same layout the original had in `Render/src/Font.cpp::LoadFontImage`:

```
"font" | int32 real_height | uint16 char_min | uint16 char_max
per char in [char_min, char_max):  int32 width | uint8 bits[((width + 7) / 8) * real_height]
```

Rows top to bottom, most significant bit first. All 15 masters parse with every byte consumed
and nothing left over; `Courier New.font` reports the same width for every glyph, which is a
free check that the width field is being read correctly.

**The `.xfont` files are not the source.** `cFontInternal::CreateTexture` tries `Load(name)`
first and, failing that, builds from the master and writes the `.xfont` back — they are a
cache of the six sizes one machine happened to ask for. Reading them would have bought fewer
faces at fixed sizes; the masters give every face at any size.

Three things are worth knowing before touching this:

- **The masters are not all one codepage.** Byte `0xC0` draws a plain `А` in
  `Russian/MAEL_small.font` and an accented `À` in `English/MAEL_small.font` — CP1251 against
  CP1252. The LocData language directory decides, and the developers' own faces under
  `Scripts/Resource/fonts` are Cyrillic. Assuming one codepage silently mangles every Western
  language's accented text. The UTF-16 reverse map is built with the same
  `MultiByteToWideChar` that built the char table, so the two agree by construction rather
  than by a hand-copied table.
- **The whole cell is scaled, padding included**, which is what `cFontInternal::CreateImage`
  did. The masters carry ~18px of leading above and below the ink, so glyphs occupy roughly
  70% of their nominal height. Trimming to the ink would give a visually larger font than
  Maelstrom shipped.
- **The greys come from the downsample.** The source is one bit deep; averaging the master
  pixels each destination pixel covers is the whole of the antialiasing, exactly as the
  original got its greys by rendering at master resolution and resampling down.

The result is an ordinary `FT::Font` — same atlas texture, same `charTable_`, same metrics —
so nothing downstream knows the difference, and `createFont` dispatches on the `.font`
extension with the TrueType path untouched. All of it is `#ifdef MAELSTROM_DATA`: Perimeter 2
ships TrueType and compiles the file to nothing.

Because the masters are per-language, `UI_FontAttributes` names them relative to the language
directory (`LocData\Fonts\MAEL_small.font`) and `UI_Font::createFont` fills in the current
one. A bitmap face and a TrueType face are indistinguishable downstream, so a `BitmapFont:`
line is logged when one is built — otherwise "the text looks wrong" is impossible to
attribute.

`--font` still overrides the lot with a TrueType face, which is only useful for deliberately
substituting a different typeface. It is no longer needed, and with it goes the whole class of
failures it used to invite: the converter never had to check that the named file existed, so a
wrong path converted cleanly and then faulted at run time in `FT::Font::size(this=0x0)`.

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
- **A cached chain carries no frame interval**, baking having already resolved the splines it
  was cut from, so `loadMaelstromChains` reads a chain's `name` and `time` and leaves
  `begin_frame` / `end_frame` / `cycled` at their constructed defaults. Nothing misses them:
  the interval's only consumer is `StaticAnimationChain::intervalSize()`, which has no callers
  anywhere in the tree, and the runtime drives a chain from `time` alone. (The `cycled` that
  the animation code does read belongs to `AnimationChain`, a different struct on the unit
  attribute side.)
- `prepareMesh()` must not run — the geometry is already baked — but the bookkeeping it
  does still has to happen: every visibility group needs its `visibility` bit and its
  `visibleNodes` flags, or `cObject3dx::Update` indexes an empty vector and faults.
- `cTempVisibleGroup` has a fifth field here (`visibilityNodeIndex`) that Maelstrom's
  record does not carry.
- **Two bound boxes, and only one of them is the model's extent.** Maelstrom's record
  carries `cStaticLogicBound::bound` — an optional collision box whose constructor zeroes
  it and which almost no model fills — and then `cStatic3dx::bound_box`, the real extent
  guarded by `is_inialized_bound_box`. This tree merged the two into one `boundBox`
  (renamed from `logicBound` by CONVERSION 18.02.08), so the reader has to choose, and the
  choice is settled by the original's `cObject3dx::GetBoundBox`, which returns `bound_box`.
  Taking the logic box instead leaves `boundBox` empty for nearly every model, and nothing
  repairs it later: `cObject3dx`'s constructor only recomputes a box when
  `isBoundBoxInited` is false, and these files store that flag **true**. The damage
  surfaced far away, in `UnitEnvironmentBuilding::setModel`'s
  `radius()/max(boundBox.radius2D(), 0.001f)` — an empty box turns that floor into a
  x1000 multiplier, so corpses and decor were built at scale ~10^4. Drawn into the shadow
  map (whose caster pipeline clamps depth rather than clipping it, so oversized geometry
  is not thrown away) they pinned every texel to 0 and the whole terrain read as
  shadowed.

### Visibility groups — `cStatic3dx::loadMaelstromVisibilitySets`

A model's meshes are divided into **visibility groups**, and the object shows one group at a
time: a transformer's `robo` / `transform` / `tank` forms, a building's `build` stages, a
wreck's `debris`. The unit picks one by *name* — `AnimationChain`'s `VisibilityGroup` — which
`VisibilityGroupName::update` resolves through `GetVisibilityGroupIndex`, falling back to
group 0 when the name is not found.

The cache carries all of this and it was being thrown away. A set is written as a
`C3DX_AVS_ONE_HEAD` record, then each group in full under `C3DX_AVS_ONE_RAW`, then
`C3DX_AVS_ONE_LODS` — three lists of indices back into the raw groups, one per LOD. Only the
head was read; every set then got `DummyVisibilityGroup()`'s single catch-all, so **every name
in the data failed to resolve and fell back to group 0**. Measured on the shipped data: of
3436 `VisibilityGroup` requests, 3294 name a group that exists in some cache.

`StaticVisibilityGroup::Load` reads Maelstrom's record **byte for byte** — it kept the
throwaway `lod` field through the rewrite. The reader survived; nothing called it here.

Two things had to be settled before the raw groups could be used:

- **Which LOD's list.** 836 of 1031 sets split their raw groups across the three LODs, which
  this tree's single list per set cannot express. It does not matter: all 836 name the same
  groups in the same order in every LOD, and each LOD renumbers its own `visible_shift` from
  bit 0 (`CalculateVisibleShift`), so LOD 0's list matches the bunch masks of every LOD. The
  node flags differ across LODs in 107 sets, and those are reached only through
  `visibilityNodeIndex`, which the cache path leaves at -1.
- **Where the node flags come from.** Not from the mesh names: `temp_visible_object` is empty
  in all 4235 cached groups, baking having resolved it into `visible_nodes`, which is written
  one entry per node in every one of them. The old code derived the flags from a mesh list
  that is never populated.

**What this actually broke is worth stating, because the earlier note in this file had it
backwards.** It said the parts the original hid per animation state were all shown at once.
They were not shown at all. `isVisible` tests `vg.visibilities & group->visibility`, and the
dummy group's `visibility` is `1 << 0`, so only bunches whose mask carries bit 0 ever passed.
Of 3475 LOD-0 bunches, 2151 are reachable on group 0 and all 3475 across the full group list —
so **1324 bunches in 491 models could not be drawn in any state**. A scene where nothing
switches groups renders identically either way, which is why this survived so long: the fault
is invisible until a unit transforms, finishes building, or dies.

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
  numbers, which is what the old engine did — and then doubles them, which is also what the
  old engine did. See the next section.
- **`global_<name>_color` is not read.** Each gradient is preceded in the file by a flag
  saying the world defers to a global set (in its own `environmentColors` block, or in
  `Scripts\Content\GlobalAttributes` when it declared none). It can be ignored: the writer
  resolved the flag before saving, so the copy sitting in the world is already the global
  gradient — `c1_m1.spg` sets all six flags and carries `Scripts\Content\GlobalAttributes`'
  11-key `sun_color` verbatim.

Perimeter 2 writes the block, and its build compiles the `#else`, so nothing here is on its
path at all.

### Two lighting formulas that changed under the same field names — `EnvironmentTime::SetTime`

Reading the right numbers is not enough here. `SetTime` turns them into the sun and shadow
colours, and between the two engines that arithmetic changed twice, in both cases without
renaming anything — so the fields load cleanly and light the world wrongly. Both are
`#ifdef MAELSTROM_DATA` in `Water/SkyObject.cpp`; the `#else` is 2008's, untouched.

**Objects were lit at double the ground's strength.** The old `SetTime` wrote
`tilemap_color.a*2` and `tilemap_color.rgb*2` straight into `SetSun`, and `SetSunColor`
clamped the result to 1. Splitting `objectShadowing` out in 2008 dropped the factor, because
a world can now just write the doubled numbers itself. Maelstrom's worlds cannot: every one
of them asks for `ambient_factor = 0.5`, and with `sun_color` at (1, 1, 0.859) that is

| | ambient | diffuse |
|---|---|---|
| the original gave objects | 0.953 | (1, 1, 1) |
| we gave them | 0.476 | (1, 1, 0.859) |

Exactly half the ambient, which is the *whole* of the light on any surface facing away from
the sun. The terrain is untouched — it never had the factor — so this is the second time a
lighting bug has shown up on the buildings and not the ground, after `ambient_maximal` above.

**`shadow_color` means something different.** 2008 divides the gradient by its own darkest
channel, so the colour is only a hue and the depth of the shadow comes from `shadow_intensity`
alone — hence the editor caption asking for "normal grey, about 0.5". Before 2008 the colour
*was* the shadow: doubled, and faded by the sun's height. `c1_m1` asks for (0.23, 0.27, 0.47)
at noon, which the 2008 reading normalises to (0.48, 0.54, 0.90) — barely a shadow, and the
blue gone with it. The fade is by `light_angle_shadow` against two constructor constants that
were never serialized (`time_shadow_off`, `speed_shadow_off`): nothing happens until the last
30° before the horizon, and then it is quick. `shadowDecay`, which replaced them in 2008, has
no counterpart in the file and is left unread.

Worth stating plainly, because it points at where to look next: this one makes the port's
shadows *lighter* than the original's, not darker. It was found while chasing the opposite
complaint, and it is not the cause of it — the doubling above is.

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

`dayTimeScale` / `nightTimeScale` are the trap in this group, and are **deliberately left
unread** — the reasoning is worth recording, because the file makes the opposite look obvious.
The pair sits in Maelstrom's `GlobalAttributes` at 250 / 500, right between `hideSmoothly` and
the water constants this reader does take; 2008 moved it *down* into `EnvironmentTime`'s own
preset block, out of reach here. So it reads exactly like the other moved fields: a value the
data supplies, a reader that cannot see it, and a live default (`EnvironmentTime`'s 500 / 1000)
at twice the speed.

It is not. **A/B against the original shows the same clock rate as ours**, so the original does
not run a mission at 250 either. What actually governs a mission is `EnvironmentTime`'s own
pair, which only `ActionSetTimeScale` changes, and the one non-zero setter in the whole data set
is `MAIN MENU.scr` asking for 500 / 1000 — the default. Reading the preset value here would
have made this build the odd one out. (`Environment::dayTimeScale_` / `nightTimeScale_`,
`Environment.h:151`, hold 250 / 500 as a leftover of the pre-2008 ownership and are serialized
and read by nothing.)

Worth knowing when reading that clock, since it is what made the pair look wrong: **there are
two of them**, adjacent in the top bar.

| control | source | reads |
|---|---|---|
| `astro time` | `UI_ACTION_EXPAND_TEMPLATE` on loc template `{time_h12} : {time_min} {time_ampm}`, against `Environment::getTime()` | the world's **time of day**, 12-hour + a.m./p.m. — does not start at 00:00 |
| `TIME` | `UI_ACTION_SHOW_TIME` | **elapsed** `h:mm:ss` since mission start; identical in both engines |

At 500 the day scale is about eight game-minutes per real second, which is why `astro time`'s
minutes field reads like a seconds counter. That is the cycle being fast, not the field being
wrong.

Measured on `Menu.spg` after the change: fog 900–1200, `height_fog_circle` 500, frustum
2–1300, `hideSmoothly` true, effects 0 / 0 / 1e6, `outside = ENVIRONMENT_WATER`, and the
underwater and ice textures resolving to real paths instead of empty strings.

### The water's wave maps — a preset field with no preset — `cWater::cWater`

Maelstrom's water had no waves at all: a flat sheet of colour, moving only where the shoreline
faded it.

The two scrolling bump maps that *are* the waves (`waves.dds` / `waves1.dds`, D3DFMT_V8U8 slope
maps) are loaded in exactly one place — `cWater::serialize`, inside its
`SERIALIZE_PRESET_DATA` branch:

```cpp
bumpTexture_  = GetTexLibrary()->GetElement3D(bumpTextureName_.c_str());
bumpTexture1_ = GetTexLibrary()->GetElement3D(bumpTextureName1_.c_str());
```

For Perimeter 2 that branch runs: `Environment::loadPreset()` opens
`Scripts\Content\Presets\global.set` under that filter and descends through `Environment`
into the water block. **Maelstrom ships no `Presets` directory at all.** Its `loadPreset()` is a
different function — it opens `Scripts\Content\GlobalAttributes` and descends into
`environmentColors` alone, which never reaches `cWater::serialize` — and the world itself is
read under `SERIALIZE_WORLD_DATA`. So the branch never ran, both handles stayed null, and
`SDLWaterRenderer` bound its flat 1x1 stand-in: a zero slope, which is no waves in *any*
technique.

The fix is what the pre-2008 engine did anyway. Its constructor loaded them from two literal
names (`origin/Maelstrom:Water/Water.cpp:88`); 2008 turned them into serialized fields and moved
the load into the preset branch. Loading them in the constructor again is not a Maelstrom
special case, so it carries no `#ifdef`: `GetElement3D` is a cache, so Perimeter 2's later
preset-driven call finds the same texture and only takes a reference, and no shipped preset in
either game names `bumpTextureName` — the constructor defaults are always what is used. It also
closes the same hole for Perimeter 2 on any path where the preset does not load.

**Everything else in that branch is still unread for Maelstrom** — `waterHeight`,
`relativeWaterLevel`, `reflection_color`, `reflection_brightnes`, the opacity gradient,
`flashIntensity`. Each was compared against the pre-2008 constructor and matches it exactly, so
the defaults are right and nothing else is missing; but Maelstrom water is entirely
default-configured, which is worth knowing before blaming something else for how it looks.

### The animation chains — `AttributeBase::serialize` / `AnimationChain::serialize`

Units loaded, moved, lit and shadowed correctly, and did not animate: legionaries slid across
the ground with their legs frozen. Two independent drifts, and the first hid the second.

**The list is written under a different key.** 2008 rewrote the chain record and renamed the
whole list to `animationChainsNew`. Maelstrom writes `animationChains`, so the read matched
nothing and every unit came up with an *empty* chain list — `ChainController::quant` returns
at `finished()` before starting anything, and `SetAnimationGroupPhase` is never called. This
was the dominant cause, and while it stood, no amount of looking at the model cache would have
helped: the models carry their chains perfectly well, the units simply never asked for one.

**The gait moved out of the chain id.** Maelstrom names a chain per gait — `CHAIN_STAND`,
`CHAIN_WALK`, `CHAIN_RUN`, `CHAIN_TURN`, plus `FIRE_WALKING` / `AIM_RUNNING` and four
`GO_`/`STOP_` transitions. 2008 collapsed them into one `CHAIN_MOVEMENTS` whose members are
told apart by `MovementState`: a pose bit (`WALK`/`RUN`/`CRAWL`) and a movement bit
(`STAND`/`MOVE`/`TURN`/`WAIT`). `findAnimationChainInterval` compares `chainID` for
**equality** and tests the state as a **superset** — `(chain & query) == query` — so the old
records, which carry no state bits at all, cannot match anything the 2008 code asks for.

They are folded onto the new shape on read (`convertMaelstromChain`). Three things make that
less mechanical than it sounds:

- **The old ids cannot keep their values.** `CHAIN_RUN` was `CHAIN_MOVEMENTS + 3`, which is
  this tree's `CHAIN_BUILDING_STAND`; `CHAIN_TURN` was `+ 4`, now `CHAIN_PAD_STAND`. The
  `MAELSTROM_DATA` enumerators therefore take fresh values after `CHAIN_UNINSTALL`, and the
  descriptor maps the old *names* onto them with `add(value, "CHAIN_RUN", 0)` — `REGISTER_ENUM`
  would stringify our own identifier and defeat the point.
- **The state sets have to be generous.** `getMovementState` always names one pose *and* one
  movement, so a standing unit still reports whichever gait it would walk with. A stand chain
  claiming only `STAND` fails the superset test the moment a gait is set; it has to accept all
  of them, hence `ALL_POSE`.
- **The four gait transitions have no equivalent.** `CHAIN_TRANSITION` replaced them and is
  driven by `transitionToState`, which these records do not carry. They fold onto the gait
  they end in — the unit animates rather than freezing mid-step, but the transition itself is
  not played.

`MovementState`'s flat-to-nested move needed nothing new: the engine's own
`// conversion 15.02.08` already reads the old flat `movementState`.

**Enabling animation woke three dormant crashes**, all in `cObject3dx::Update` and all the
same shape — an index validated against the *model's* chain list, then used on a shorter one.
`SetAnimationGroupChain` checks against `animationChains_`, but lights, materials and nodes
each keep their own list; `UpdateVisibilityGroup` took `&groups[index]` with no check at all,
which is null for an empty vector. The guards are unconditional: retail data simply never
reaches them.

### Aircraft banking — `FormationUnit::quant`

Maelstrom's helicopters flew a clean path while spinning about their own axis: the heading
swung ±40–57° per quant and the model rolled through ~140°, reversing whenever the turn rate
changed sign. The position was never wrong — traced, `step` held 4.99–5.10 per quant the whole
time — which is why this reads as a rendering or animation fault and is neither.

A flying unit's bank comes from one term:

```
clamp(rotSpeed() / G2R(pathTrackingAngle), -1, 1) * additionalHorizontalRot
```

Pre-2008 divided by `pathTrackingAngle` **as written, in degrees**
(`origin/Maelstrom Physics/RigidBodyUnit.cpp:1939`). 2008 converted the denominator to
radians. `G2R(30) = 0.524` against `30` is a **57× larger ratio**, so the clamp saturates and
the bank becomes the whole of `additionalHorizontalRot` — which is a bank angle in *radians*,
and Maelstrom's aircraft prms carry `20`. Twenty radians where the old divisor gave a degree
or two.

Three things make this worth its own section:

- **Our own tree disagrees with itself.** `Physics/PathTracking.cpp:507` still carries the
  pre-2008 formula verbatim, `G2R`-free and unclamped. Only the `FormationController` path
  converts, and that is the path a flying unit in a formation takes.
- **It is not a port regression.** `git log -L 354,354:Physics/FormationController.cpp` shows
  the line unchanged since the initial commit — stock 2008 code. Perimeter 2 reads the same
  `20` and `45` in three of its own flying prms, so retail does this too. The fix is therefore
  `#ifdef MAELSTROM_DATA` and the `#else` is byte-identical: Maelstrom's prm values were tuned
  for the old divisor, Perimeter 2's for whatever 2008 intended.
- **Measure amplitude, not reversals.** After the fix the turn-direction reversal *rate* is
  still 11–24% on busy units; what changed is that each one is now fractions of a degree
  instead of forty. Peak heading swing fell 118° → 11°, peak roll 0.94 → 0.18 (quaternion `y`).
  Counting reversals would have said the fix barely worked.

Found by recording the unit's pose per logic quant and per rendered frame and comparing the
two series. That also cleared the three things it was *not*: the control loop (heading decays
smoothly, 1 reversal in 91 quants on a clean flight), the interpolator (the factor advances
0.00999 per ms against an ideal 0.01000, tracking real elapsed time exactly), and frame
delivery (60 fps, ±2 ms, after a spawn transient in the first 2.5 s).

### Silhouettes — `Camera::DrawSilhouetteObject`

The outline itself is stencil work that was never ported (**Render-PORTING.md #22**), and
guarding the whole function off on the null `gb_RenderDevice3D` looked like the safe thing to
do. It was not. `cObject3dx::PreDraw` routes an object carrying
`ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE` to `SCENENODE_FLAT_SILHOUETTE` **instead of**
`SCENENODE_OBJECT`, so returning early dropped those units out of the frame altogether rather
than merely dropping their outline. Every unit whose attributes set `showSilhouette` — the
guard tower, the legionaries, the warship — was invisible.

Its shadow was not, which is what makes this worth writing down: the shadow camera is attached
earlier in the same `PreDraw`, before the visibility test, so the ground showed the shadow of a
unit that was not being drawn. That reads as a shadow-map bug and is not one.

The list is now drawn plainly, exactly as the child-camera branch below it already did. Retail
Perimeter 2 never fills it, which is why none of this surfaced there.

### The camera's limits — `Environment::serialize` / `CameraRestriction::serialize`

How far the camera may be pushed past the map's edge, how close it may zoom, how far it may
tilt: `selfCameraRestriction`, `cameraBorder` and a flat run of `CAMERA_*` names. All of it
was an `Environment` field and is a `CameraManager` one now, so a pre-2008 world writes the
group in its `environment` block and `CameraManager::serialize` never sees a name of it.

Two levels, and only one of them was broken:

- **The global set is in `Scripts\Content\GlobalAttributes`**, under a `cameraRestriction`
  node this tree already opens — so the names the two schemas share were arriving all along
  and only the renamed ones fell back to constructor defaults. That is where the visible part
  was: `zoomMin` 300 against the file's 50, `heightMin` 0 against 50, and `zoomDefault` 300
  against 500, which is the distance every mission opens at (`Player.cpp:554` places the
  camera from the *global* set, before a world's own is read).
- **The per-world copy is in the `.spg`.** All 51 write `cameraBorder`; three — `menu.spg`
  and the two `TEST_Environment` worlds — set `selfCameraRestriction` and write the whole
  restriction, the rest deferring to the global one.

| pre-2008 | now | |
|---|---|---|
| `CAMERA_ZOOM_MIN` / `_MAX` / `_DEFAULT` | `zoomMin` / `zoomMax` / `zoomDefault` | rename |
| `CAMERA_MIN_HEIGHT` / `CAMERA_MAX_HEIGHT` | `heightMin` / `heightMax` | rename |
| `CAMERA_THETA_MAX` | `thetaMaxLow` | see below |
| `CAMERA_THETA_MIN` | `thetaMaxHigh` | see below |
| `CAMERA_THETA_DEFAULT` | `thetaDefault` | rename, and degrees against radians |
| `CAMERA_ZOOM_SPEED_DELTA`, `_MOUSE_MULT`, `_SPEED_DAMP` | `zoomKeyAcceleration`, `zoomWheelImpulse`, `zoomDamping` | **not read** |

**`CAMERA_THETA_MIN` is not a floor under the tilt.** The maximum tilt falls off with
distance — `CameraCoordinate::check` interpolates it — and the two ends of that ramp are what
the old pair held: `CAMERA_THETA_MAX` is the ceiling zoomed *in*, `CAMERA_THETA_MIN` the
ceiling zoomed *out*, and the floor was a plain 0. So they pair with `thetaMaxLow` and
`thetaMaxHigh`, `zoomMaxTheta` takes `CAMERA_ZOOM_MAX`, and `thetaMinLow`/`thetaMinHigh` stay
at 0. The 2008 defaults for that pair are 60° and 18°, which are Maelstrom's own global
values — a free check on the pairing, and the reason the tilt looked right while the zoom
did not.

Three smaller things:

- **The zoom dynamics were rewritten, not renamed.** A key press used to add
  `CAMERA_ZOOM_SPEED_DELTA` to the zoom force outright; it now adds `zoomKeyAcceleration`
  *times the distance*. There is no value of the new field that reproduces the old one, so the
  three old names are left unread and the 2008 defaults stand.
- **`aboveWater` is a 2008 field the data cannot carry**, and the question is what the
  original did without it: it tracked the ground with `vMap.GetApproxAlt` outright. Ours
  routes that through `CameraCoordinate::height`, which with `aboveWater` takes
  `cWater::GetZFast` — and that returns the water surface *everywhere*, over land as well, so
  the camera's focus sinks to water level on a world with terrain above it. False here.
- **A `RangedWrapper` does not clip outside the editor**; at load it is a plain float read. The
  clamps in the pre-2008 serializer were ordinary `clamp` calls that ran either way, so the
  Maelstrom branch spells them out — which matters, `menu.spg` asking for `CAMERA_ZOOM_MIN = 0`
  where the 2008 wrapper's editor range starts at 20.

Measured, against the values in the files:

```
c1_m1:  own=0 border=0/1344/-64/-64  zoom=50..1000 def=500  h=50..1000  thetaMax=60/18
menu:   own=1 border=-1000 x4        zoom=0..5000  def=300  h=0..2000   thetaMax=85/5
        scrollSpeed=0  mouseAngle=0   <- the menu camera is meant to be locked
```

`menu.spg`'s zeroed scroll and mouse-rotation speeds are the clearest single symptom: with
them unread the main menu's camera was free to be dragged and spun.

**`FarPlane` and `NearPlane` are not camera fields**, though an earlier note in this file
listed them as such. They are the depth-of-field pair in the world's `environment` block,
where `Environment::serialize` reads them already (`DofParams.x`/`.y`).

### The classes the data names — three of eleven were worth porting

Maelstrom's data names eleven polymorphic classes this source does not have. `XPrmIArchive`
reports each miss, `skipValue`s the block and carries on
(`Util/Serialization/XPrmArchive.cpp:1172`), so they surface as `ERROR! no such class
registered` rather than as a failure to load — and, being non-fatal, they had never been
sorted by whether anything reaches them.

| class | in the data | in `origin/Maelstrom` | verdict |
|---|---|---|---|
| `ActionSetCoastSprites` | `MAIN MENU.scr` ×14 | `Environment/ActionsEnvironmental.cpp` | **ported** |
| `ConditionObjectNearObjectByLabel` | 7 chains ×12 | `Util/Conditions.cpp` | **ported** |
| `UI_ACTION_EXPAND_TEMPLATE` | `UI_Attributes` ×6 | `UserInterface/UI_ControlsGame.cpp` | **ported** |
| `ActionSquadMove` | `MISSIA.scr` ×1 | `Util/Actions.cpp` | unreachable |
| `AttributeReal` | `Scripts/Engine/AuxDictionary` | — | not a class |
| `AiAction_{ExecuteChain,MoveToFireRadius,RotToPoint,Stop}`, `AiCondition_{FireRadius,RotToPoint}` | `Scripts/Engine/AiActionChainList` | — | dead in Maelstrom too |

**Seven of the eleven exist in no C++ at all — including Maelstrom's.** The AI action-chain
system reads like a feature this tree lacks; `git grep -E 'AiAction|AiCondition'` over
`origin/Maelstrom` returns the data file and one `.vcproj` line listing it, and nothing else.
`AiActionChainList` is 771 bytes holding two chains, one of them named `Test_2`. `AuxDictionary`
is a four-line rename table whose *key* is `"class AttributeReal"` — a name being converted
away from, not a class being instantiated. Neither file is opened by either engine. The same
method note that settles unread wire names settles unresolved classes: ask what the **original**
did with it.

**`ActionSquadMove` is live in Maelstrom and still unreachable here**, because the chain that
uses it is not loaded: mapping all 128 `.scr` files against every reference to them leaves four
orphans — `MISSIA`, `AI_C1M1_E`, `AI_C2M1` and `TEST_!` — named by no world and no other chain.

The three that were ported:

- **`ActionSetCoastSprites`** sets the shoreline's sprite parameters. `cCoastSprites::serialize`
  here *is* Maelstrom's `Init()` with the read folded into it, so the apply half is split back
  out and the action calls it. Every screen of the main menu sets its own, in the `Waves*`
  trigger of its environment block.
- **`ConditionObjectNearObjectByLabel`** measures from a labelled *unit*; 2008 kept only the
  anchor sibling, `ConditionObjectNearAnchorByLabel`. Ten of its twelve uses are in AI chains
  that `c1_m1`, `c1_m7`, `c2_m1` and `c2_m7` load, where an unregistered condition reads as one
  that is simply never true.
- **`UI_ACTION_EXPAND_TEMPLATE`** is the pre-2008 form of `UI_ACTION_LOCALIZE_CONTROL`: it
  carries no string and expands the control's *own* caption. Six controls use it, and the
  in-game clock is one — `L_ASTRO TIME` resolves to `{time_h12} : {time_min} {time_ampm}`, which
  without the action is drawn literally, braces and all. The caption has to be kept unexpanded
  beside `text_` (the original kept the same thing under the same name, `locText_`), because
  expanding into `text_` alone consumes the template on the first update.

Worth knowing for next time: **`unresolved.txt` is one `fopen` away.** Logging `str` at the
`result == -1` branch of `XPrmArchive.cpp` turns "which classes are missing" into a measurement
instead of a grep — it counts only the ones actually reached, and it distinguishes a class that
resolves from one whose file is never opened. Removing a single `REGISTER_CLASS` and re-running
is the control that proves the probe is live: `ActionSetCoastSprites` came back 14 times, once
per call site. With all three registered, a boot to the menu and a load of the four campaign
worlds above report **nothing** unresolved.

### Tooltips — `UI_ActionDataHoverInfo::appendMaelstromHover`

A control used to carry its hover text and hover cursor itself, in a transparent
`openBlock("hover")` — so `hoveredTextLoc` and `hoveredCursor` sit at the control's own level,
and at the control *state's*, which had the same pair. 2008 collected them into a
`UI_ACTION_HOVER_INFO` action.

Rather than teach the tooltip popup a second way to find its text, the old fields are read back
into the action that replaced them: `UI_ControlBase::findAction` already looks in the control's
own list and then in the current state's, which is exactly the fallback the old
`UI_ControlBase::hint()` did by hand. Everything downstream — the show delay, the type match,
the template expansion, the hovered cursor — is then the code that was already there. The
synthesis has to run *after* `actions_` is read, since reading it replaces the vector.

It is worth the trouble: **757 of Maelstrom's controls name a tooltip key and not one names a
`UI_ACTION_HOVER_INFO`**, so on this data the 2008 path is dead end to end. Where they are
matters for testing them —

| screen | tooltips |
|---|---|
| `ALIENS` / `ASCENSIONS` / `REMNANTS` (the three race HUDs) | 292 / 232 / 203 |
| `Select Mission` | 25 |
| `Main Menu`, `Select CAMPAIGN` | 3, 2 |

— so the main menu shows almost none of it. Load a mission and hover a HUD button for
`tipsDelay`, half a second.

### Hints that cannot be closed — `ActionMessage::activate` / `::workedOut`

The only drift here that is neither a type nor a nesting change: the same field, the same C++
type, a different *meaning*. `messageSetup.displayTime` reads 0 on every Maelstrom hint, and

- **pre-2008** (`origin/Maelstrom:Util/Actions.cpp`) started a `DurationTimer` at 0, so the
  action **finished on its first quant**, and a finishing `MESSAGE_ADD` never took its own
  message down — that was a separate `MESSAGE_REMOVE` trigger's job;
- **2008** reads the same 0 as "show forever" (`workTimer_.start(time ? time : INT_INF)`) and
  removes the message when the action finally completes.

Read the 2008 way, the trigger that shows a hint never leaves `WORKING`. That is not a cosmetic
difference, because a trigger activates its outgoing links **only on reaching `DONE`**
(`Trigger::setState`, `TriggerEditor/TriggerExport.cpp`). Maelstrom builds each tutorial hint as
a chain — show it, then a `ConditionClickOnButton` on `REMNANTS.TEXT.message inf.x` (the red ✕),
then `MESSAGE_REMOVE` — so a stalled show-trigger strands **everything behind the first hint**,
not just the close button. `C1M1.scr` alone hangs eight of these off one another.

Both halves are switched. `UI_Message::aliveTime_` (`UI_Types.cpp`) already reads `displayTime`
the pre-2008 way — a real time, or ~10000s when it is 0 — so once the action stops removing the
message the UI keeps it up on its own, which is exactly the old division of labour.

Worth knowing for diagnosis: the click, the `EventButtonClick`, and the delivery to the trigger
system were all working. What localised it was logging *which* `ConditionClickOnButton`s were
ever evaluated — all 55 belonged to the global and menu chains, none to the mission's.

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

## The load pass, and the crash it found

Every `.spg` in the distribution was started with `-world <name>`, given fifteen seconds to
reach a steady state and then killed. A world still running at the end had loaded; one that
exited on its own had not. **47 of 51 build their universe with not one assertion between
them** — all four campaigns, the multiplayer maps, the cutscene worlds and the menu. Only
`c1_m1` and `c1_m2` had ever been loaded before this.

The four that failed — `TEST_Effects`, `TEST_Environment_Buildings`, `TEST_Environment_Trees`,
`TEST_World_Tutorial` — all segfaulted the same way, in `vrtMap::getAlt` (`VMAP.H:122`)
dereferencing a null `vxaBuf`. **This was the long-standing "water vxaBuf null crash", open
and unreproduced until the pass handed over four reliable repros.**

It is not schema drift. Those four are `.spg` files with **no world directory beside them** —
no `world.cls` to open, so `allocMem4Buf` never runs and `vxaBuf` stays null. `vrtMap::load`
handles that correctly and returns `false`; `GameShell::GameLoad` called it as a statement and
discarded the result, so the mission went on to build a universe over a heightfield that was
never allocated, and the first `getAlt` faulted.

**Fixed unconditionally, because Perimeter 2 ships orphan worlds too** — `cs_c1_open` and
`intro_01` have no directory either, so the same segfault is reachable in retail. The load now
aborts with the world's name.

Two things worth keeping from this:

- **`ErrH.Abort`'s message does not reach the log at that point.**
  `XErrorHandlerStub::Abort` prints to `stderr`, and stderr stops being captured somewhere
  after renderer init — the renderers' own "ready" lines are the last thing through it. The
  first cut of the fix therefore turned a crash into a *silent* `exit(1)`, which is barely an
  improvement. `dprintf` still works there (it is what prints `Universe created`), so the name
  goes out through both.
- **A load-only pass needs no visual judgement**, which is what makes it cheap enough to
  re-run after any change to the schema readers. Worlds that merely *load* are not proven
  playable — nothing here drives a mission past its first frames.

## Ruled out — physics, and a method note

Maelstrom's units move and take hits differently enough to look wrong, and the obvious place
to blame is `RigidBodyPrm`: its wire list is much longer on the old side. Three separate
theories were chased from that and **all three are dead ends**. They are recorded here
because each is easy to re-derive and cost real time.

**The `RigidBodyPrm` wire lists differ by 23 names — and 19 of them are dead in Maelstrom
too.** `RigidBodyPrm::serialize` reads 63 names here against 86 there. The 23 the old data
writes and we never read look alarming, and the flight and steering cluster
(`flying_stiffness`, `flying_oscillator_z`, `flying_vertical_direction_{x,y}_factor`,
`zero_velocity_z`, `rudder_speed`, `steering_acceleration_max`,
`steering_linear_velocity_min`, `brake_damping`, `point_control_slow_{distance,factor}`,
`speed_map_factor`, `Dxy_minimal`, `analyze_points_density`, …) looks like the whole
pre-2008 flight model gone missing. It is not: `git grep` each name across
`origin/Maelstrom` and they appear **only in `Physics/RigidBodyPrm.h` and the constructor
and serializer in `Physics/RigidBody.cpp`**. The original declared, defaulted, saved and
loaded them, and never read one. Nothing that a field is not consumed by can change
behaviour.

Only **four** of the 23 are live in the original, and they are the only ones worth porting
if a symptom ever points at them: `flying_down_without_way_points`
(`RigidBodyUnit::checkDeepWater`, and the obstacle check for an uncontrolled flyer),
`debris_angular_velocity` (`RigidBody::startDebris`, debris spin), and `impassabilityPass` /
`ptImpassabilityCheck` (`PathTracking`). Two more that read as live are not:
`isotropic` survives only as a constructor default and inside the words *isotropic* and
*anisotropic* in comments, and `tree` is too common a word to grep for.

**`hoverMode` and `alwaysMoving` are 2008 inventions, and their defaults are already
correct.** Both are in the four fields *we* read that the old schema never writes, and both
are tempting to set for Maelstrom's aircraft. Neither should be.
`hoverMode` does not mean "hovers in place" — it means the unit rides on top of whatever is
beneath it (`setDownUnit` / `isDownUnit`, and `checkPenetration` returning false for a
non-hover flyer); the original tree contains **no** `downUnit` machinery at all, so its
flyers pass over everything, which is what `hoverMode = false` gives. `alwaysMoving` gates
`canRotate_` and `canMoveBack_` (`FormationController.cpp:990`); the original has no
`canRotate_`, no `canMoveBack_` and no `alwaysMoving`, so its units could turn on the spot
and reverse, which is what `alwaysMoving = false` gives. Setting either would move this port
*away* from the original. The other two, `fieldPass` and `placeOnWaterSurface`, are likewise
absent from the data and left at their defaults.

**Missile knockback cannot fire here at all.** `IronBullet` throws its target with
`attr().impulseStrength * sqrtf(mass)` through `addImpulseLinear`
(`Units/IronBullet.cpp:189`), which is a plausible suspect for a unit that leaps when hit.
It is unreachable twice over: the feature is a 2008 addition — the original's
`Units/IronBullet.cpp` has no impulse code, and `addImpulseLinear` / `impulseStrength` occur
in the whole Maelstrom tree only in `EnvironmentSimple.cpp` — and Maelstrom's data never
writes `applyImpulse`, so our `false` default stands and the branch is never entered. A unit
that appears to be thrown is arriving some other way; anything dropped from the sky comes
through `StateBirthInAir` → `StateTouchDown`.

Two smaller results from the same pass: the converter's `steering_duration` float→int rewrite
is lossless on the shipped data (every value is `3000.`, `1500.`, `1000.` or `5000.`), and
`gravity`, `restitution`, `friction`, `TOI_factor` and `relaxationTime` are read by **both**
engines, so none of them is a candidate either.

**The method note, which is the part worth keeping.** A wire name the old data writes and
this tree does not read is *not* evidence of anything on its own. Before treating one as a
regression, check that the **original** consumed it — `git grep -l <name> origin/Maelstrom`
and discard the hits in the prm header and its serializer. Nineteen of these twenty-three
fell at that step. The same test in reverse settles the fields we read and the data omits:
ask what the original did *without* the field, not what our default happens to be.

## Still open

1. **The basement is read and thrown away.** `C3DX_BASEMENT` (500/501/502) is building
   foundation geometry, a feature P2 dropped. The raw `.3DX` path ignores the chunks
   outright; the cache path has no choice but to read them — they sit mid-record in
   `otherInfo`, so skipping them would put the reader out of step — and then discards them.
   If that geometry is ever wanted, it is already parsed.
2. **`ActionSquadMove` is unported**, the one class of the eleven that Maelstrom's engine
   consumed and this tree cannot resolve. Its single call site is in `MISSIA.scr`, which no
   world and no other chain names, so nothing reaches it. Port it if that chain is ever
   wired up; `Util/Actions.cpp` in `origin/Maelstrom` has it.
3. **The silhouette outline is still unported.** The units themselves draw now (see above);
   what is missing is the coloured outline they show through a building they walk behind,
   which is stencil work — **Render-PORTING.md #22**. Unreachable on retail Perimeter 2, so
   this build is the only way to exercise it.

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

**The menu is a scripted set-piece, not a world running on its own.** `menu.spg` carries no
triggers at all; the whole scene is driven by `Scripts/Content/Triggers/MAIN MENU.scr` —
80,864 lines, 128 `ActionActivateSources`, 17 `ActionCreateUnit`, 21 `ActionSquadMoveToAnchor`.
The guard-tower fight, the units walking between anchors and the aircraft are all placed by
it. So is the time of day: each screen sets an environment block of

```
Fog3xx  ActionSetFog · TimeStopxxx  ActionSetTimeScale dayTimeScale = 0 ·
SetTimexxx  ActionSetEnvironmentTime time = <hour> · SetHeavenColorxxx · Wavesxx · SetWaterTransxxx
```

with the clock **frozen** at 4.5, 9.0, 15.0, 15.4 or 16.9 and restored to the world's
`dayTimeScale = 500` by exactly one trigger, `TimeStop100`. Day and night in the menu are the
script stepping between pinned hours, not a cycle advancing — so lighting that changes when
you move between screens, and only then, is correct.
