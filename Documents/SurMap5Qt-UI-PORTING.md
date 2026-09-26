# SurMap5Qt UI porting register

The MFC editor (SurMap5/) is the reference; this file tracks what the Qt port
(SurMap5Qt/) still has as a stub or is missing entirely. Status values:

> **Status as of the `qt-editor` branch (session audit).** Parts of this file
> are older than the code: the Qt library editor, the Objects Manager, real
> 3D picking/selection, the transform tools, the Properties dock and the whole
> trigger editor are **implemented now**, not stubs. The tables below were
> corrected against `SurMap5Qt/src`, but the *narrative* sections that follow
> them (Panels, Tools, Camera/Environment, External-tool editors) still carry
> some of the old wording — treat the tables and the "Current gaps" summary at
> the end as authoritative.

- **OK** — real implementation in the Qt port.
- **STUB** — the action/menu exists but only logs "not wired yet" / shows a
  status message / is a placeholder.
- **MISSING** — the original's command/panel/dialog has no Qt counterpart at all.
- **N/A** — the original itself was a no-op or is obsolete (kdw-only editor,
  MFC bar plumbing).

Where a stub can be promoted now that the editor builds a real Universe
(loadWorld -> `new Universe`), the note says so.

## Menus (IDR_MAINFRAME)

### File
| Command | Original handler | Port | Notes |
|---|---|---|---|
| New World | OnFileNew | OK | `newWorld()` → `EngineViewport::createWorld` (`vMap.create/save` + `new Universe`) |
| Open World | OnFileOpen | OK | `openWorld()` → `vMap.load` + `MissionDescription` + `new Universe` |
| Save / Save As | OnFileSave/Saveas | OK | `fileSave()/fileSaveAs()` → `vMap.save` |
| Save MiniMap to World | OnFileSaveminimaptoworld | OK | `fileSaveMiniMapToWorld()` → `vMap.saveMiniMap` |
| Save Without terTool Color | OnFileSavewithouttertoolcolor | N/A | original body is commented out |
| Resave All Worlds | OnFileResaveWorlds | OK | `fileResaveWorlds()` loops load+save |
| Resave All Triggers | OnFileResaveTriggers | MISSING | iterate `Scripts\Content\Triggers`, `TriggerChain::load/save` |
| Save VoiceFile Durations | OnFileSaveVoiceFileDurations | MISSING | `VoiceAttribute::loadVoiceFileDuration` then save |
| Update quick start worlds list | OnFileUpdateQuickStartWorldsList | MISSING | rebuild `Scripts\Content\QuickStartWorlds` list |
| Merge | OnFileMerge | STUB | `fileMerge()` TODO; needs `universe()->mergeWorld` |
| Run World / Run Main Menu | OnFileRunWorld/RunMenu | OK | spawns `Game.exe` (staged next to the editor by `.local/build-game.ps1`) |
| Export VistaEngine | OnFileExportVistaEngine | STUB | packaging pipeline (bat, exe/dll copy, archives, installer) |
| Import/Export Text Excel | OnFileImportTextFromExcel / ExportTextToExcel | STUB | `ImportImpl/ExportImpl` ↔ `TextDB` |
| Export/Import World | OnFileExportImportWorld | PARTIAL | `fileExImWorld()` + `ExImWorldDialog`; drives external `packer.exe` |
| Properties | OnFileProperties | OK | WorldPropertiesDialog (read-only) |
| Statistics | OnFileStatistics | OK | TexturesStatisticsDialog (read-only) |
| Exit | ID_APP_EXIT | OK | |

### Edit
| Command | Original handler | Port | Notes |
|---|---|---|---|
| Undo/Redo | OnEditUndo/Redo | OK | `vMap.UndoDispatcher_*` |
| Map Scenario | OnEditMap | STUB | `editMapScenario()` TODO; original edited `MapSerializer` |
| Game Scenario | OnEditGameScenario | STUB | `editGameScenario()` TODO; original `GameSerializer` |
| Map Preset | OnEditPreset | MISSING | `PresetSerializer` / `environment->loadPreset/savePreset` |
| Preferences | OnEditPreferences | MISSING | full `SurMapOptions` dialog (see Preferences section) |
| Save Camera As Default | OnEditSaveCameraAsDefault | PARTIAL | writes QSettings, but `applySavedCameraDefault()` never calls `setOrbitCamera` |
| PlayPMO | OnEditPlaypmo | MISSING | debug replay (`vMap.playPMOperation`) |
| Rebuild World | OnEditRebuildworld | OK | `vMap.rebuild` + `reinitWorld` |
| Update Surface | OnEditUpdateSurface | OK | `vMap.recalcArea2Grid` |
| Change Total World Param | OnEditChangetotalworldheight | OK | ChangeTotalWorldHeightDialog + `changeTotalWorldParam` |
| Rolling Border | OnEditRollingborder | OK | BorderRollingDialog + `vMap.autoLace` |
| Triggers | OnEditTriggers | OK | SelectTriggerDialog + full TriggerEditorDialog (graph, class tree, conditions, debugger, minimap) |
| Units | OnEditUnits | OK | Qt library editor, `openLibrary("AttributeLibrary")` |
| Objects | OnEditObjects | N/A | original body commented out |
| User Interface | OnEditUserInterface | PARTIAL | launches external `UIEditor.exe` |
| Effects | OnEditEffects | OK | `openLibrary("EffectContainerLibrary")` |
| Sounds | OnEditSounds | OK | `openLibrary("SoundLibrary")` |
| TerTools | OnEditTertools | OK | `openLibrary("TerToolsLibrary")` |
| Cursors | OnEditCursors | OK | `openLibrary("UI_CursorLibrary")` |
| Heads | OnEditHeads | OK | HeadsDialog → `GlobalAttributes::showHeadNames` + `saveLibrary` |
| Sound Tracks | OnEditSoundTracks | OK | `openLibrary("SoundTrackTable")` |
| Reels | OnEditReels | PARTIAL | `ReelsDialog` lists `.bik`; original was a no-op → N/A-ish |
| Command Color | OnEditCommandColor | PARTIAL | `openLibrary("CommandColorManager")` reads real colors; `setCommandColor` is a no-op |
| UITextSprites | OnEditUITextSprites | OK | `openLibrary("UI_SpriteLibrary")` |
| Terrain Type Name | OnLibrariesTerrraintypename | PARTIAL | TerrainTypeDialog reads; `setTerrainTypeNames` write is a no-op |
| Effects Editor (tool) | OnEditEffectsEditor | PARTIAL | launches external `EffectTool.exe` |

### View
| Command | Original handler | Port | Notes |
|---|---|---|---|
| Sources | OnViewSources | OK | flags `EditorVisualOptions::showSources_`; hides/shows sources + anchors + their overlays via `editorVisual().isVisible` |
| Cameras | OnViewCameras | OK | flips `showCameras_`; hides/shows camera-spline overlays (`CameraSpline::showInfo`) |
| Geosurface | OnViewGeosurface | N/A | original is commented out (no-op) |
| Show Grid | OnViewShowGrid | OK | `viewToggleGrid` → EngineViewport `gridVisible_`, persisted |
| Camera Borders | OnViewShowCameraBorders | PARTIAL | flag `showCameraBorders_` persisted; the border overlay itself is not drawn yet |
| Path Finding | OnViewPathFinding | PARTIAL | flag `showPathFinding_` persisted; the impassability aux-unit draw is not ported yet |
| Path Finding - Select Ref Unit | OnViewPathFindingReferenceUnit | DEAD | action disabled (no reference-unit picker) |
| Time Flow | OnViewEnableTimeFlow | PARTIAL | records `timeFlowEnabled_`; does not advance `Environment` time |
| Hide Models | OnViewHideModels | OK | flips `hideWorldModels_`; `UnitBase::showEditor` → `hide(HIDE_BY_EDITOR)` hides units/environment models |
| Animation | OnViewAnimation | PARTIAL | `setUpdatesEnabled` — repaint gate, not engine `flag_animation` |
| Time Slider | CTimeSliderDlg in filtersBar | OK | TimeSliderDialog → `environmentTime()->SetTime`; but modal, not the modeless filters-bar child |
| Objects Manager (bar toggle) | OnViewObjectsManager | OK | dock toggle |

### Workspace (docks/toolbars)
| Command | Original handler | Port | Notes |
|---|---|---|---|
| Reset Workspace | OnViewResettoolbar2default | OK | `restoreState(QByteArray())` |
| Menu Bar | OnBarCheck | OK | |
| Main Toolbar | OnBarCheck | OK | |
| Filters Bar | OnBarCheck | OK | |
| Libraries Bar | OnBarCheck | OK | |
| Editors Bar | OnBarCheck | OK | |
| Status Bar | OnBarCheck | OK | |
| Tools (Tree Bar) | OnViewTreeBar | OK | dock |
| Properties | OnViewProperties | OK | dock — **content still empty** (see Panels) |
| Minimap | OnViewMinimap | OK | dock |
| Objects Manager | OnViewObjectsManager | OK | dock |
| Extended mode tree bar | OnViewExtendedmodetreelbar | MISSING | tree bar extended mode flag |

### Debug
| Command | Original handler | Port | Notes |
|---|---|---|---|
| Editable Tools Tree | OnViewExtendedmodetreelbar (debug) | PARTIAL | sets a Qt property; nothing reads it |
| Save Tools Tree | OnDebugSaveconfig | OK | `ToolsTreePanel::saveState()` (QSettings, not the original XPrm tree file) |
| Edit ZipConfig | OnDebugEditZipConfig | PARTIAL | read-only text view of `Scripts\Content\ZipConfig` |
| Edit debugPrm | OnDebugEditDebugPrm | PARTIAL | read-only text view of `Debug.dat` |
| Edit AuxAttribute | OnDebugEditAuxAttribute | MISSING | library editor on `AuxAttributeLibrary` |
| Edit RigidBodyPrm | OnDebugEditRigidBodyPrm | MISSING | |
| Edit Toolzer | OnDebugEditToolzer | N/A | original body empty |
| Edit ExplodeTable | OnDebugEditExplodeTable | MISSING | |
| Edit SourcesLibrary | OnDebugEditSourcesLibrary | MISSING | `openLibrary("SourcesLibrary")` |
| UI Sprite Lib | OnDebugUISpriteLib | MISSING | `openLibrary("UI_ShowModeSpriteTable")` |
| Show Palette Texture | OnDebugShowpalettetexture | OK | `vMap.toShowTryColorDamTexture` |
| Show Mipmap | OnDebugShowmipmap | STUB | no engine effect |

## Panels

| Panel | Original | Port | Notes |
|---|---|---|---|
| Tools tree | CToolsTreeWindow / CToolsTreeCtrl | PARTIAL | real tool tree + selection + QSettings persistence; nine tools now (Select/Move/Rotate/Scale + Toolzer/Kind/ColorPic/GeoNet/GeoTx), grouped Transform/Terrain/Objects; no per-tool Create/Delete/Properties popup |
| Objects Manager | CObjectsManagerWindow | OK | 5 tabs rebuilt from `EngineViewport::objectList`; **rename/delete only edit the tree row**, not the world object; no drag&drop |
| Properties | propertiesBar_ hosting the current CSurToolBase dialog | OK | `PropertyTree` + `PropertyDelegates` render/edits `editor::PropertyRow` trees fed by the bridge; per-tool panels are `TransformPropertyPanel` / `GeoNetPropertyPanel` / `GeoTxPropertyPanel` |
| Minimap | CMiniMapWindow | OK | real terrain minimap + camera marker, click moves the camera |
| Gradients | CGradientsWindow | STUB | draws hard-coded built-in gradients; no engine gradient list, edits change local data only |
| Camera control | (camera controls) | OK | CameraControlPanel: centre/distance/yaw/pitch/roll, eye/apply/top/overview/fit/reset |
| Time slider | CTimeSliderDlg modeless in filtersBar | OK (modal) | drives `environmentTime()->SetTime`; modal dialog, not embedded in the filters bar |
| Wave dialog | CWaveDlg floating | OK code, UNUSED | `WaveDialog` calls `bridge_->wave*` → `environment->fixedWaves()`; **no menu opens it** |
| Camera dialog | CCameraDlg (IDD_DLG_CAMERA) | OK | `CameraDialog` → `cameraManager` splines; opened from View→Cameras, not its own command |

## Tools (SurTool*, the map-editing tools)

The original registered ~36 tool types through `FactorySelector<CSurToolBase>`
(SurMap5/SurTool*.h/.cpp) and stored the per-world tool tree in
`ToolsTreeCtrl::serialize`.

**Ported and real:** Select (rubber-band **plus** real picking — `RenderViewWidget`
calls `EngineViewport::selectObjectAt` / `selectObjectsInRect`, which set
`unit->setSelected`), Move, Rotate, Scale (they mutate the selected objects'
pose/radius through the `IWorldBridge`), **Toolzer** (raise/lower the terrain by
a circular brush — `vMap.deltaZone`, the port of `CSurToolToolzer`'s circle
variant), **Kind** ("Hardness": paint the surface type — `vMap.drawInGrid` with
`GRIDAT_MASK_SURFACE_KIND`, plus the `toShowSurKind` tint while active),
**ColorPic** ("Texture": paint a bitmap texture with a `ColorModificator` —
`vMap.drawBitmapCircle` / `putBitmap2AllWorld`), **Unit** (place a unit of the
chosen `AttributeLibrary` attribute — `Player::buildUnit` + `setPose` at the
terrain height, with the legionary/squad join), **Source** (`SourceTool`:
SourcesLibrary element + live preview + `addSource`) and **Anchor** (`AnchorTool`:
editable anchor + live preview + `addAnchor`), GeoNet (`bridge_->applyGeoNet`
→ `geoGeneration`; no direct MFC counterpart), GeoTx (re-render only — matches
the original, where the paint call is commented out). The Toolzer/Kind/ColorPic
editors live in the Properties dock (`ToolzerPropertyPanel`,
`KindPropertyPanel`, `ColorPicPropertyPanel`); Source/Anchor/Unit get their own.

Dependency note: selection works for **alive, non-auxiliary units** only.
Sources, anchors and camera splines are not click-pickable, and a real marquee
that picks non-unit objects is still missing.

Not yet ported (original class → what it does):
- CSurTool3DM / CSurToolEnvironment — place a .3dx model / environment object on the map
  (`worldPlayer()->buildUnit(...)`, `setEnvirontmentType/setModel/setRadius/setPose`).
- CSurToolMiniDetaile / CSurToolMiniDetaileFolder — mini-details placement.
- CSurToolRoad — road drawing.
- CSurToolWater / CSurToolWaves / CSurToolWindStatic — water level / waves / wind zones.
- CSurToolLighting — lighting (sun) editing.
- CSurToolSource — extraction/resource source placement (`sourceManager->addSource`). **Ported** (`SourceTool` + `SourcePropertyPanel`): the SourcesLibrary element list
  (cp1251 decoded through `propertytext::displayBytes`), a live `sourceOnMouse_` preview under the cursor, the element's parameters edited through the bridge's
  PropertyRow tree (`sourceElementTree`/`sourceElementSetTree`), and `placeSource` on click.
- CSurToolAnchor — anchor placement (`sourceManager->addAnchor`). **Ported** (`AnchorTool` + `AnchorPropertyPanel`): one editable anchor, live preview, PropertyRow
  tree (`anchorTree`/`anchorSetTree`), unique label via a local `kdw::makeName` equivalent, `placeAnchor` on click.
- CSurToolGrass — grass zones (`environment->grass()->SetGrass`).
- CSurToolSpecFilter ("Detail Filter") — `vMap.specialFilter`.
- CSurToolBlur — `vMap.gaussFilter`.
- CSurToolImp — original stub (no-op) → N/A.
- CSurToolUnitFolder / CSurToolPlayerFolder — the per-player unit tree (the port
  lists the whole AttributeLibrary in one combo instead). Base unit placement is
  done (`UnitTool`); the original's legionary/squad handling and player
  assignment are not.
- CSurToolPathEditor — path (waypoint) editing.
- CSurToolCamera / CSurToolCameraEditor / CSurToolCameraRestriction — placed from
  CameraDialog / CameraControlPanel in the port instead of as map tools.
- CSurToolGeo / CSurToolGeoTx — the port's GeoNet/GeoTx cover the geo tools.

Dependency note: every remaining placement tool needs (a) a Qt property form in
the Properties dock and (b) a pick-from-scene / click-to-place path through
`EngineViewport` that ends in `worldPlayer()->buildUnit(...)` / vMap mutations.

## Camera / Environment editing

- **CameraDialog**: `cameraManager` spline create/delete/play is wired through the
  bridge (`bridge_->cameraNames/createCamera/deleteCamera/playCamera`). Missing:
  the original's mouse modes (CREATE_POINTS / SELECT_POINTS / EDIT_POINTS) — the
  port cannot yet add/select/edit spline points by clicking the map.
- **WaveDialog**: create/remove/apply are wired (`bridge_->wave*` →
  `environment->fixedWaves()`). **It is just never opened** — no menu command
  instantiates it.
- **TimeSlider**: really moves the sun (`bridge_->setTimeOfDay` →
  `environmentTime()->SetTime`), but as a modal dialog; the original was a
  modeless child of the filters bar.
- **Camera restriction** (CSurToolCameraRestriction — the camera border rect on
  the minimap) is not ported.

## Editor state & persistence

- SurMapOptions (SurMap5/SurMapOptions.h) — showSources_/showCameras_/
  hideWorldModels_/showPathFinding_/enableGrid_/gridSpacing_/gridColor_/
  cameraBorder*_/last_dirs_/dlgBarState. The **visibility flags the renderer
  reads** are now ported as `Util/EditorVisualOptions` (engine-side, one copy for
  the game and the editor) and driven from MainWindow's View menu, QSettings-
  backed; defaults match the original (sources/cameras **on**). The rest (last
  dirs, dock state, grid colour, LOD) is still scattered Qt/QSettings state with
  no single struct.
- `EditorVisual::isVisible` (SurMap5/EditorVisualImpl.cpp) — the per-class
  visibility hook the renderer asks. **Ported** in the Qt stub
  (`VistaEngineContext.cpp`): units/environment follow `hideWorldModels_`,
  sources/anchors follow `showSources_`, camera splines `showCameras_`. The
  original's path-finding aux unit (`beforeQuant`/`afterQuant` + the reference
  unit) is not ported yet, and the camera-border overlay is not drawn.
- Tools tree persistence (ToolsTreeCtrl::serialize reads/writes
  `Scripts\TreeControlSetups\...`). The port persists only QSettings state, not
  the original XPrm tree file.

## External-tool editors (kdw-based library editors)

The big MFC chunk — `editLibrary()` over `kdw::LibraryEditorDialog` — is
**ported**: `SurMap5Qt/src/dialogs/LibraryEditorDialog` + `src/panels/LibraryEditor`
+ `PropertyArchive` give a generic library editor (tree of elements + serialized
property form), and every `openLibrary("...")` command uses it. What remains is
per-library write gaps: `CommandColorManager::setCommandColor` and
`setTerrainTypeNames` are no-ops, and the parameter Excel import/export does not
exist yet.

## Priority proposal

1. ~~**Terrain brushes** — SurToolToolzer (raise/lower/level), SurToolKind
   (surface type), SurToolColorPic (texture paint).~~ **Done:** Toolzer, Kind
   and ColorPic are ported (`ToolzerTool`, `KindTool`, `ColorPicTool`); the
   Toolzer's square/Exp/PNoise/MPD variants and the ColorPic live preview are
   the only pieces left of that original set.
2. ~~**`EditorVisual::isVisible` + a Qt `SurMapOptions`**~~ **Done (visibility).**
   `Util/EditorVisualOptions` + the ported `isVisible` make Sources/Cameras/Hide
   Models real and persistable. Left: the path-finding aux unit, the
   camera-border overlay, and the remaining `SurMapOptions` fields (dirs, dock
   state, grid colour, LOD) in one struct.
3. **Placement tools** — ~~SurToolUnit~~ **done** (`UnitTool`); ~~SurToolSource~~
   and ~~SurToolAnchor~~ **done** (`SourceTool`/`AnchorTool`, live preview +
   embedded PropertyRow attrib editor). Still open: SurTool3DM/Environment
   (file-pick a model), then the Road/PathEditor/CameraEditor/MiniDetaile set.
4. **Wire the ready dialogs**: put WaveDialog on a command; give CameraDialog its
   own command and add the CREATE_POINTS/SELECT_POINTS mouse modes.
5. **Close dead/stale items**: `actViewPathFindingRef_` (unconnected), Debug
   Mipmap, `actSaveCameraAsDefault_` (saved but never restored), brush-radius
   combo (never reaches `GeoNetTool`).
6. **Objects Manager interaction**: context-menu delete/rename against the world
   object (not just the tree row), drag&drop.
7. **Remaining File/Edit stubs**: Merge (`universe()->mergeWorld`), Map/Game
   Scenario, Map Preset, parameter Excel import/export, Export VistaEngine.
8. **Smaller MISSING items**: Resave All Triggers, Save VoiceFile Durations,
   Update quick start list, PlayPMO, camera-restriction tool, extended tree-bar mode.

## Effect render filters — diagnostic mode

The world-quad renderer's effect filters can be dropped at runtime to see which
one hides or distorts the effects. Set `VISTA_FX_CLEAN=1` to start in "clean
mode" (all filters off), then list the ones to put back via `VISTA_FX_ON`
(comma-separated, no rebuild needed). Implemented in
`Render/SDLWorldQuadRenderer.cpp` (`openGroup`, `DrawPrimitive`) with the
`CLEAN_KEEP_*` mask; parsed in `SurMap5Qt/src/editor/EngineViewport.cpp`
(`fxSetClean`).

| `VISTA_FX_ON` item | what it restores | observation |
|---|---|---|
| (none — clean mode) | all filters off | effects/particles visible |
| `DEPTH` | depth test | **hides the effects** — the depth test is what suppresses them |
| `SOFT` | soft-fade (scene-depth fade) | almost everything works, but with the fade on part of the units, seen at a certain angle, take the terrain texture. (A single unit rendering as the terrain at a certain angle may show up in every variant — not yet checked across all of them.) |
| `BLEND` | the material's own blend (instead of forcing additive) | **the columns appear**, but part of the effects disappear or dim |
| `COLOROP` | texel×alpha premultiply / COLOR_OPERATION | everything works **except the columns** — the same result as with every filter off, so COLOR_OPERATION is not what breaks the effects |
| `FOG` | distance fog | everything works except the columns — same as all filters off |
| `ZREF` | TRI height-clip (ZREFLECTION) | everything works except the columns — same as all filters off |
| `TRIALPHA` | TRI vertex alpha (instead of forcing 255) | everything works except the columns — same as all-off (effects look a little brighter without the forced alpha=255) |

These are diagnostics; the goal is to find the real filter bug and fix it, then
remove the clean-mode code.

### Findings

Putting any filter back **one at a time** gave, for every item but two, the same
picture: everything works except the columns. Only two items changed anything:

- **`DEPTH`** — with the depth test back, the effects **vanish**. So the depth
  test is what is suppressing the effect groups: they lose against the scene
  depth (terrain/units). In the original the emitters that ask for a no-Z pass
  (`EMITTER_DRAW_AFTER_ALL` and the two grass modes) are drawn with
  `D3DRS_ZENABLE` off; the port reads that from the camera pass
  (`emitterDepthTest`, commit `72e6441f`), so the groups that still fail the test
  are the ones drawn in the *sorted* pass — i.e. the depth they compare against,
  or the depth they are drawn at, is wrong.
- **`BLEND`** — with the material's own blend back, **the columns appear**, but
  part of the effects dim or disappear. The clean mode forces
  `ALPHA_ADDBLEND`; the columns need their own blend to be visible, so a wrong
  blend is a second, separate defect (it does not explain the columns being
  missing in the normal build, where the blend is already the material's own —
  there the depth test hides them like every other effect).

Everything else (`SOFT`, `COLOROP`, `FOG`, `ZREF`, `TRIALALPHA`) is neutral for
the columns.

So the two things to fix are (1) the depth test that hides the effect groups,
and (2) the columns' blend. Neither is a "filter to disable" — the clean mode
only proved where the loss happens.

Next: fix (1) the effect groups' depth test, then (2) the columns' blend.
`VISTA_FX_ON=BLEND` shows the columns reliably, so it is the reference to diff
the blend against.

Note (session, editor rendering): the columns themselves are the `A_MAM` /
`G_Core_Working_001`-style emitters and the core model's `Light` visibility set,
not the particles — a separate line of work from the particle filters above.

## Editor effects: where they are lost (session findings)

After the crossplatform merge (`ab82ddcc`) and the diagnostic clean-up
(`5daa4599`), the light columns appear and a part of the effects does not.

**What is NOT the cause** (all measured, not guessed):

- **Depth near/far.** Forcing `zNear=1, zFar=6000` (20x better precision) changed
  nothing. The editor's `editorZPlane` does extend `zFar` to
  `orbit.distance + map diagonal` (~23000 at the default orbit), where the game's
  `cameraManager->SetFrustumEditor` caps it at `12000` — but that is not what hides
  the effects. It is, however, the obvious candidate for the **jumping sky**.
- **The effects are created and reach the renderer.** `VISTA_FX_TRACE`
  instrumentation showed every effect (`G_Fx_Pump_001`, `G_Fx_Chain_002`,
  `G_Fx_Core_001`, `A_MAM`, …) running `PreDraw` **and** `Draw` every frame, all in
  pass 11 = `SCENENODE_OBJECTSORT`, and the world-quad renderer's `Draw` receiving
  `groups=771`, `quads≈5100`, `tris=3510` per frame. The pass opens with
  `clear=0, clearDepth=0` (it loads). So geometry is recorded and a pass runs.
- **Distance LOD.** The editor already clears `ATTRUNKOBJ_HIDE_BY_DISTANCE` and
  `cEffect::setVisibleRange(false, …)`; emitters show `rateReal≈0.64` (not 0).
- **The zMode pass split.** No effect is dropped by pass assignment; they are all
  `EMITTER_USE_ZBUFFER` → the sorted pass.

**So the loss is at the draw itself** — the geometry reaches the pass but does not
appear. The remaining candidates, in order of likelihood:

1. **The quad batch (`7d287404`, our only kept render change).** It defers the
   sorted pass's world-quad flush to `endQuadBatch`. It is the one thing in the
   sorted pass that differs from crossplatform. Test: bracket-free
   `Camera::DrawSortObject` (revert to flushing per `drawWorldQuads`).
2. **A later pass overwriting the effects' pass.** The effects land in
   `SCENENODE_OBJECTSORT`, and `Camera::DrawScene` draws `DrawObjectNoZ(SCENENODE_OBJECT_NOZ)`
   after it; the sky cubemap (`environmentTime()->Draw()` → `cRenderCubemap::DrawFace` →
   a child camera) now renders a face every frame *before* the world, and shares the
   command buffer. Worth checking the target/clear bookkeeping across those passes.
3. **Premultiply/blend after crossplatform's `1e67532e`** (the DDS premultiply was
   removed and `colorOp[1]` now decides). If the particle textures are `.tga`
   (straight alpha) the flag should be 1; a wrong flag draws fully transparent or
   fully black.

**Next step:** test (1) first — it is one line and reversible — then (2).

### Result of the render-side probes (session, continued)

All three candidates were measured, and a fourth fact came out of it:

- **The quad batch is not it** (the user confirmed, and the traces agree: the
  pass runs).
- **Premultiply/blend is not it.** `VISTA_FX_OPAQUE=1` (every group drawn
  `ALPHA_NONE`, no premultiply, no depth test) showed **nothing** — so the loss is
  not alpha or blend.
- **The pass is not overwritten.** The effects' world-quad pass goes straight to
  `screen_` (`captureArmed=0`), opens with `clear=0, clearDepth=0` (LOAD), and the
  geometry is in it every frame.
- **The geometry projects on screen.** `VISTA_FX_PROJ` showed the particle groups
  at `ndc≈(-0.96, 0.0, 0.97)`, screen `(19,438)` etc. — on screen, correct
  viewport.

**The actual split:** `VISTA_FX_ZMODE` shows the visible and invisible effects
differ by **`zMode` alone**:

| effect | emitters | zMode | pass | visible |
|---|---|---|---|---|
| `A_MAM` (columns) | 3 (1 useZ, **2 nozBefore**) | NOZ-before-grass | 2 | **yes** |
| `G_Fx_Pump_001` | 3 | **all `EMITTER_USE_ZBUFFER`** | 11 (sorted) | no |
| `G_Fx_Chain_002` | 7 | all `EMITTER_USE_ZBUFFER` | 11 | no |
| `G_Fx_Core_001` | 4 | all `EMITTER_USE_ZBUFFER` | 11 | no |
| `G_Fx_Sign_Disconect` | 4 | all `EMITTER_USE_ZBUFFER` | 11 | no |

So: **effects whose emitters ask for a NOZ pass are visible; effects whose
emitters all ask for the z-buffer pass are not.** The NOZ passes are exactly the
ones the original drew with `D3DRS_ZFUNC = D3DCMP_ALWAYS` (verified in `ca9aa43`'s
`Camera::DrawObjectNoZ`), i.e. an always-passing depth test.

The sorted pass (`EMITTER_USE_ZBUFFER`) has the default LESS-EQUAL depth test on
both backends, so the original *did* depth-test those sprites. The question the
data now points at is **what depth they test against**: the emitter's world z is
~95-100 while the terrain under it is at the same height, so a sorted, depth-tested
particle at `ndc.z≈0.967` loses to the terrain unless it is drawn **before** the
terrain writes depth, or with an always-pass test.

**The one experiment left** (and the one that settles editor-vs-engine): run the
game (`VistaEngineDbg.exe`) on the same world and look at `G_Fx_Pump_001` /
`G_Fx_Chain_002`. If the game shows them and the editor does not, the delta is
editor-only (draw order / target / camera); if the game also hides them, it is
engine-wide and belongs in `Render-PORTING.md`, not the editor register.

### Found: the frame's screen clear lands after the effects pass (game vs editor)

The game was built from this tree (`Game` target, run with the editor's working
directory so both load the same content) and the two frames were traced with the
same instrumented renderer. The game shows the effects; the editor does not, and
the pass order is where they part.

Game, one frame:

```
#9370 WORLDQUAD  cubemap 256²   clear=1
#9371 WORLDQUAD  screen 1920²   clear=1   <- the frame's only screen clear
#9372 WORLDQUAD  screen 1920²   clear=0
#9373 WORLDQUAD  reflection 1024² clear=1
#9374 WORLDQUAD  256²           clear=0
#9375 TERRAIN    1024²           clearDepth=1
#9376-9379 WORLDQUAD 1024²      clear=0
#9380 TERRAIN    screen 1920²   clear=0
#9381-9386 WORLDQUAD screen 1920² clear=0   <- main scene + effects
```

Editor, one frame:

```
#402 WORLDQUAD screen 1035²  clear=0        <- effects drawn here
#403 WORLDQUAD cubemap 256²  clear=1
#404 WORLDQUAD screen 1035²  clear=1        <- the clear lands AFTER #402
#405 WORLDQUAD screen 1035²  clear=0
```

In the game the screen is cleared **once, before** anything draws into it; in the
editor the screen's clear opens **after** a world-quad pass already drew the
effects into it, so that pass is wiped. The `#403` cubemap face (the sky, which
`EnvironmentTime::Draw()` now renders — crossplatform's `Environment::graphQuant`
calls it) sits between the two screen targets, and the editor's two `...EB0` /
`...F08` screen images show the clear being armed on the wrong one.

The original editor's `CGeneralView::graphQuant` called `environmentTime()->Draw()`
**once, explicitly, right after `BeginScene` and before `environment->graphQuant`**
(`SurMap5/GeneralView.cpp:297`); crossplatform moved that call **inside**
`Environment::graphQuant`. The fix is to restore the original ordering for the
editor's frame (or make the cubemap render not re-arm the frame's screen clear),
not to touch the effects.

### RESOLVED: the light columns were a missing window, not lost logic

The whole hunt above (pass order, clear, premultiply, `zMode`, triggery) described
effects that *reached* `Draw` and then did not appear. The in-body **light columns
of the core** turned out to be a different, simpler thing: the effect that carries
them (`G_Fx_Button_005`, per the game's own `[fxcol]` trace) was **never created**
in the editor at all, and the columns that *did* exist were drawn by
`SDLWorldQuadRenderer`, which the editor had left with `window_ == null`.

`SDLWorldQuadRenderer` was built at `Initialize` with no window (the Qt editor
claims none then) and, unlike the tile map and the UI, it had **no `setWindow`** —
so `pipelineFor` bailed on `!window_` and returned a null pipeline for every group.
`Draw` then skipped every group silently. Commit `623d7f6f` fixed it for world-quad
alone; the same fix has now been applied to **every** renderer, so none is left
with a null (or default-format) pipeline. That is what made the columns appear.

So the editor's remaining difference from the game is **not** here: it is the
game-logic channels the editor skips on purpose (`isUnderEditor()` gates:
`Universe::Quant` → no `triggerQuant`, `SourceBase::quant` → no source activation
or waiting effects, `SourceZone::apply` → no damage/abnormal state, `UnitActing`
→ no weapon quant, and `Environment::logicQuant` → time stands still unless the
time-flow toggle is on). Effects started by those channels are absent in the
editor by design; effects tied to a unit's `permanentEffects` load and show.

### `EngineViewport::drawFrame` vs `GameShell::Show` / `CGeneralView::graphQuant`

The frame order matches the original editor's `graphQuant` (Fill → BeginScene →
`environment->graphQuant` → `ATTRCAMERA_CLEARZBUFFER` → `terScene->Draw` →
`drawGrid` → `drawPostEffects` → `cameraManager->showEditor` → `universe()->graphQuant`
→ aux → `afterQuant` → EndScene/Flush). Deliberate differences and gaps:

- The port calls `editorZPlane()` + `camera_->SetFrustum()` by hand instead of
  `cameraManager->SetFrustumEditor(surMapOptions.zFarInfinite)`. `SetFrustumEditor`
  also has the `zFarInfinite → 12000` cap the port dropped, and resets
  `frustumClip_`. Worth switching to the engine call.
- **`cameraManager->quant()` is never called.** The game runs it every frame; it
  drives `CameraCoordinate::check()` and the camera matrix. The port sets the matrix
  in `applyCamera()` instead, so the camera moves — but no child-camera/coordinate
  bookkeeping happens. Whether that matters for the effects is open.
- The port calls `environment->graphQuant` (which now draws the sky cubemap via
  crossplatform's `EnvironmentTime::Draw()`) — matching the original, which called
  `environmentTime()->Draw()` explicitly before it.
- `UI_Dispatcher::quant` and `gb_RenderDevice->selectRenderWindow(renderWindow_)`
  around the frame are not mirrored (the port selects the window once at init).

## Engine-side changes made for the editor (outside `SurMap5Qt/`)

The Qt editor branch is cut from `b7660ed4`. Bringing the editor up changed engine
code **outside** `SurMap5Qt/` too, because the editor drives the real game code
paths (`new Universe`, world load, `loadAllLibraries`, the SDL renderers) in a
context those paths never saw before: no `GameShell`, no SDL-owned window at
`Initialize`, no MFC `EditorVisual`, a frozen `.3dxG` cache with no source
`.3dx`, and `.prm` data the game tolerates but a direct `Universe` construction
does not.

This is the register of those changes — what, where, why — so they are not
mistaken for unrelated engine edits. Reproduce the list with:

```
git diff --name-only b7660ed4..HEAD -- . ':(exclude)SurMap5Qt'
```

Items marked **[TEMP]** are diagnostics or band-aids added during bring-up and
are candidates for removal once the problem they measure is fixed.

### Game / Units / Util (the simulation)

- **`Game/GameContext.cpp` — a real `editorVisual()`.** The non-editor build
  `xassert`ed and returned `*reinterpret_cast<EditorVisual::Interface*>(0)`. The
  Qt editor runs `UnitBase::showEditor` → `hide(HIDE_BY_EDITOR, ...)` and the
  anchor/source editor drawing through `EditorVisual::isVisible`, so it needs a
  live implementation. Added an engine-side `EditorVisualImpl`: everything
  visible, the draw helpers no-ops (the editor overlay loops that would call
  them — source/anchor labels, selection radius — do not run yet). SurMap5's own
  `SurMap5/EditorVisualImpl.cpp` is MFC-bound (`CMainFrame`, `SurMapOptions`).
  Extend it when the editor's View filters land.
- **`UserInterface/UI_LogicGame.cpp` — guard `disableDirectControl()`.** It
  dereferenced `gameShell`, which the map editor never creates (`createRuntime` /
  `WinMain` do). `Universe::setActivePlayer` calls it on world load. Added
  `if(!gameShell) return;` (no-op in the game).
- **`Util/Serialization/StringTableBase.h` + `Units/UnitAttribute.cpp` —
  normalized `editorGroupName()`.** The lookup used the raw
  `typeid(*type_).name()` spelling ("class X"), but the factory registers
  *normalized* names, so every group missed ("No translation for such class
  name!") and the Units library editor crashed. Now normalize through
  `normalizeTypeName(...)` and pass `true` (silent) so an unregistered type falls
  back to the normalized name instead of asserting.
- **`Units/Parameters.cpp` + `Util/FormulaString.cpp` — cycle/stack guards
  [TEMP].** `ParameterValue::value()`'s `state_==CALCULATING` check only catches
  direct self-reference (P1→P1); a P1→P2→P1 cycle recurses until
  `STATUS_STACK_OVERFLOW`. Added a thread-local depth counter (cap 256) that
  returns 0 at the cap, plus a "first 64 entries" log naming the looping `.prm`
  parameter. `FormulaString.cpp` got the same guard at depth 512 for the
  expression parser. These are a band-aid for cyclic `.prm` data — fix the data
  at the source (the log names it) and remove both.
- **`Units/UnitAttribute.cpp` — `VISTA_LOG_LIBRARIES` [TEMP].** `loadAllLibraries`
  prints each library before loading it when `VISTA_LOG_LIBRARIES=1`, used by the
  editor bring-up to find which singleton load crashes.
- **`Platform/WindowsAPI.h` — `MoveFile` shim.** Added `MoveFileA`/`MoveFile`
  (`rename(2)`) for the editor's WorldList rename/resave path; the caller checks
  destination existence itself, matching `rename`'s overwrite semantics.
- **`Game/Universe.cpp` — stage `fprintf` logs [TEMP].** `[ctor]` / `setActivePlayer`
  progress lines, added while bringing the editor's `Universe` construction up.
  Console-only; delete when the editor is stable.

### Render

- **`Render/3dx/Lib3dx.cpp` — `exported_ = true`.** The port runs against the
  shipped frozen cache (`CacheData/Models/*.3dxG`); the source `.3dx` files are
  not bundled. `LoadCache`'s file-time validation (`meshTime_`/`meshSize_` vs the
  missing source) rejected every good cached model, so `cLib3dx` now treats the
  cache as exported — the same fix `cTexLibrary` already uses.
- **`Render/3dx/Node3DX.cpp` — `lods[iLOD]` bounds guards.** A model whose cache
  says `is_lod=true` but carries fewer than three lods (or none) selects iLOD 0..2
  in `Update()` and indexed out of range in `Draw()` / `DrawShadowAndZbuffer()`.
  Report via `VisError` and return instead of the invalid-parameter fast-fail.
  Marked "possibly delete when the `.3dxG` LOD-count mismatch is understood".
- **`Render/src/NParticle.cpp` — `emitterDepthTest(camera)`.** The emitters' depth
  test comes from the camera **pass**, not the material: D3D's
  `Camera::DrawObjectNoZ` (`SCENENODE_OBJECT_NOZ` and the two `..._GRASS`
  siblings) turned `D3DRS_ZENABLE` off for the whole node, so an emitter asking
  for one of those passes (`EMITTER_DRAW_AFTER_ALL`, the grass modes) drew with no
  depth test. The port had it hardcoded `true`, depth-testing those sprites
  against terrain/units and dropping them. Applied to `cEmitterColumnLight`,
  `cEmitterInt`, `cEmitterSpline`, `cEmitterZ`.
- **`Render/src/cCamera.cpp` — quad batch bracket.** `DrawSortObject` brackets the
  sorted pass with `beginQuadBatch()`/`endQuadBatch()` (see the render device), so
  the few hundred unit lights and effects share one world-quad render pass instead
  of one each.

### Renderer (SDL GPU)

- **`Render/SDLRenderDevice.cpp/.h` — `cRenderWindow` and multi-window.** The D3D
  implementation (`Render/D3D/RenderDevice.cpp`) is compiled nowhere, and the
  editor creates one `cRenderWindow` per viewport, so the class lives here now:
  `CalcSize` via the shim's `GetClientRect`, `ChangeSize` →
  `RecalculateDeviceSize`, dtor → `DeleteRenderWindow`. `createRenderWindow(hwnd)`
  wraps the viewport's HWND in a foreign `SDL_Window`
  (`SDL_CreateWindowWithProperties`, with `SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER`
  / X11 window number / Cocoa window pointer) and claims it on the device;
  `selectRenderWindow` / `currentRenderWindow` / `setGlobalRenderWindow` pick which
  swapchain `BeginScene`/`EndScene` use; `activeWindow()` maps the active
  `cRenderWindow` to its `SDL_Window`. The game's window is the implicit global
  one registered by `Initialize`.
- **`Render/SDLRenderDevice.cpp` — null-window and active-window paths.**
  `Initialize` tolerates `window_ == null` (the Qt editor creates no SDL window of
  its own, so the device is created without claiming a window; the first
  `createRenderWindow()` supplies the swapchain target — before, a null window was
  fatal). `BeginScene` acquires the swapchain from `activeWindow()` and sets
  `xScr/yScr` from the acquired drawable size (the engine's `xScr/yScr` are the
  window size the Camera's 4:3 math uses). `ensureCapture` and
  `CreateTexture(TEXTURE_RENDER32)` query the format from `activeWindow()`. `Done`
  destroys foreign editor windows first, nulls the bindings, deletes the
  `cRenderWindow`s.
- **`Render/SDLRenderDevice.cpp/.h` — `beginQuadBatch`/`endQuadBatch`.** On SDL GPU
  a world-quad flush is a whole render pass (a full-target load/store per light or
  effect — several hundred a frame on a busy map). A walk pass that records many
  back to back brackets itself and they collapse into one pass. The quads never
  write depth, so deferring the flush only moves where the pass opens.
- **`Render/SDLRenderDevice.cpp/.h` — `DrawLine(const Vect3f&, ...)` implemented.**
  It records into the new world-line renderer; `EndScene` replays it before the
  UI. See `Documents/Render-PORTING.md` #17 (no longer a no-op).
- **`Render/SDLWorldLineRenderer.{cpp,h}` + `Render/SDLShaders/worldline.{vert,frag}.hlsl`
  + `Render/CMakeLists.txt` — new world-line pipeline.** The editor's terrain grid
  (`EngineViewport::drawGrid`) draws through `DrawLine`. It is the SDL stand-in for
  D3D's `FlushLine3D`: `PT_LINELIST`, `SetWorldMaterial(ALPHA_BLEND, MatXf::ID)`,
  world-space vertices × the camera's view-projection, LESS-EQUAL z-test, no
  z-write, `SRC_ALPHA`/`1-SRC_ALPHA` blend. One pipeline, one vertex layout
  (`sVertexXYZD`), one uniform (MVP), replayed in one pass at `EndScene` before the
  UI. `Render/CMakeLists.txt` adds the `worldline` shader module and the source.
- **Null-window tolerance across the renderers.** When the Qt editor had no SDL
  window, every renderer was constructed with `window_ == null`. Two mechanisms
  cover it, and between them **every** renderer now builds its pipelines in the
  editor:
  - The plain-single-pipeline renderers (`SDLCloudShadowRenderer`,
    `SDLEnvironmentEarthRenderer`, `SDLGrassRenderer`, `SDLPostEffectRenderer`,
    `SDLObject3dxRenderer`) build their shaders from the device alone:
    `SDL_GetGPUSwapchainTextureFormat` tolerates a null window (returns the device
    default), so only `device_` gates. Their window-format pipelines are built
    lazily and dropped/rebuilt when the window arrives (below).
  - `SDLWorldQuadRenderer` (the effects), `SDLWaterRenderer` and
    `SDLMinimapRenderer` need the **real** swapchain format, so they keep their
    window gate and get a `setWindow` instead.
- **`setWindow` on every renderer.** `cSDLRenderDevice::createRenderWindow` hands
  the viewport's foreign window to **all twelve** renderers (`ui`, `tileMap`,
  `worldQuad`, `object3dx`, `water`, `grass`, `cloudShadow`, `environmentEarth`,
  `postEffect`, `worldLine`, `blobs`, `minimap`), not just the first three. Each
  `setWindow` releases the pipelines built against the old (null) format and lets
  them rebuild; the shaders and samplers, which are window-independent, are kept.
  This is what restored the light columns: before, only the tile map and the UI
  got the window, so `SDLWorldQuadRenderer` stayed with a null `window_` and
  `pipelineFor` returned null for every effect group, silently.
  Two renderers build their pipelines **only** in the constructor, with no lazy
  path, so their `setWindow` *re-runs* the build (releasing the samplers and the
  flat stand-in texture first, or they would leak): `SDLWaterRenderer`
  (`createPipelines` → fill/line/reflect/cube/ice) and `SDLMinimapRenderer`
  (`createPipelines` → map/symbol/lines). `SDLWorldLineRenderer::setWindow` also
  clears `shadersTried_` beside `pipelineReady_`, since `ensurePipeline` keys off
  it and would otherwise keep returning the stale `false`.
- **`Render/SDLBlobsRenderer.cpp` — shaders need no window.** Its `createShaders`
  was the last one still gated on `window_`; the shaders read no window (only the
  swapchain-format query in `createPipelines` does), so it builds from the device
  alone now.
- **`Render/SDLUIRenderer.cpp/.h` — `DrawDebugTriangle` [TEMP].** Screen-space
  diagnostic triangle used while validating the native SDL swapchain host.
- **`Render/SDLTileMapRenderer.cpp` — terrain bring-up logs [TEMP].** A first-draw
  `target/depth/pipeline/indices/camera/size` line and a `BeginGPURenderPass`
  failure line.
- **`Render/SDLObject3dxRenderer.{cpp,h}` — null-window gate + replay diagnostics
  [TEMP].** The null-window gate is what let the models rasterize at all; the
  `[dbgreplay]` periodic `draws`/`nullPipeline`/`replayed` counters and `drawCount()`
  separate "nothing recorded" from "recorded but the pipeline was rejected".
- **`Render/SDLWorldQuadRenderer.{cpp,h}` — null-window tolerance + the FX debug
  layer [TEMP].** `forceNoDepth`/`forceNoFog`/`forceNoSoft`/`forceNoPremul`/`forceFlat`,
  `debugWireParticles`, and `cleanFx_`/`cleanKeep_` with the `VISTA_FX_ON` bits. See
  "Effect render filters — diagnostic mode" above.

### Build, packaging and CI

- **`CMakeLists.txt`** — `option(BUILD_EDITOR ...)` + `add_subdirectory(SurMap5Qt)`.
  The old MSVC tool projects (`SurMap5`, `ModelViewer`, `VistaEditor`, …) were
  never migrated to CMake and stay in place as the reference implementation.
- **`Render/CMakeLists.txt`** — the `worldline` shader module and
  `SDLWorldLineRenderer.cpp` in `RENDER_SOURCES` (part of the `BUILD_EDITOR` work
  but useful to the engine independently).
- **`Documents/Build-PORTING.md`** — "The editor (SurMap5Qt)" section: build with
  `-DBUILD_EDITOR=ON`, Qt 6 ≥ 6.4 (Widgets), per-platform Qt install, the CI
  editor jobs.
- **`.github/workflows/{windows,linux,macos}.yaml`** — `qt-editor` added to the
  push/PR triggers, plus an `editor` job per platform: configure
  `-DBUILD_EDITOR=ON`, build `SurMap5Qt`, stage the runtime (Windows:
  `windeployqt` + the whole directory so `platforms/qwindows.dll` ships; macOS:
  reuse the Game job's shadercross cache), upload the artifact. The
  `--selftest=SmokeTest` step is disabled (commented) — it needs a GPU/display and
  staged content the runners do not provide yet.
- **`.gitignore`** — editor build/run artifacts (`build-qt-check`, `iniFile.cfg`,
  `Worlds`, helper `*.ps1`/`*.cmd`, screenshots and `*.log`/`*.err`/`*.out`),
  `.vs`/`out`/`*.slnx`, the local `.local/` build/run helpers, and the
  XLibs/STLPort debug leftovers MSVC and the git tools unpack into the tree.

### Permanent vs. temporary

The **permanent** engine changes are the ones that make a code path the editor
legitimately runs safe: `EditorVisual`, the `GameShell` guard, the normalized
`editorGroupName` lookups, `MoveFile`, `exported_ = true` for the frozen cache,
the `lods[iLOD]` guards, `emitterDepthTest`, and the whole SDL render device /
world-line / null-window body of work.

Everything tagged **[TEMP]** is a diagnostic (`VISTA_LOG_LIBRARIES`,
`VISTA_FX_CLEAN`/`VISTA_FX_ON`, `[dbgreplay]`, the `fprintf` stage logs, the
`--selftest`/debug triangle) or a band-aid for bad `.prm` data (the
`Parameters.cpp`/`FormulaString.cpp` cycle guards) and should be removed once the
underlying problem is fixed.
