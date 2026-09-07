# SurMap5Qt UI porting register

The MFC editor (SurMap5/) is the reference; this file tracks what the Qt port
(SurMap5Qt/) still has as a stub or is missing entirely. Status values:

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
| New World | OnFileNew | OK | `newWorld()` |
| Open World | OnFileOpen | OK | `openWorld()` |
| Save / Save As | OnFileSave/Saveas | OK | `fileSave()/fileSaveAs()` |
| Save MiniMap to World | OnFileSaveminimaptoworld | OK | `fileSaveMiniMapToWorld()` |
| Save Without terTool Color | OnFileSavewithouttertoolcolor | MISSING | vMap.save variant; engine `saveWorld` flag |
| Resave All Worlds | OnFileResaveWorlds | OK | `fileResaveWorlds()` |
| Resave All Triggers | OnFileResaveTriggers | MISSING | iterate Scripts\Content\Triggers, `triggerChain.save()` |
| Save VoiceFile Durations | OnFileSaveVoiceFileDurations | MISSING | `VoiceAttribute::loadVoiceFileDuration` then save |
| Update quick start worlds list | OnFileUpdateQuickStartWorldsList | MISSING | rebuild `Scripts\Content\QuickStartWorlds` list |
| Merge | OnFileMerge | STUB | `fileMerge()` TODO; needs `universe()->mergeWorld` |
| Run World / Run Main Menu | OnFileRunWorld/RunMenu | OK | spawns Game.exe |
| Export VistaEngine | OnFileExportVistaEngine | STUB | `fileExportVistaEngine()` TODO |
| Import/Export Text Excel | OnFileImportTextFromExcel / ExportTextToExcel | STUB | two slots TODO |
| Export/Import World | OnFileExportImportWorld | OK | `fileExImWorld()` + ExImWorldDialog |
| Properties | OnFileProperties | OK | WorldPropertiesDialog |
| Statistics | OnFileStatistics | OK | TexturesStatisticsDialog |
| Exit | ID_APP_EXIT | OK | |

### Edit
| Command | Original handler | Port | Notes |
|---|---|---|---|
| Undo/Redo | OnEditUndo/Redo | OK | `editUndo()/editRedo()` |
| Map Scenario | OnEditMap | STUB | `editMapScenario()` TODO; original edits MapSerializer via kdw |
| Game Scenario | OnEditGameScenario | STUB | TODO |
| Map Preset | OnEditPreset | MISSING | Edit menu "Карта пресет" — attribute preset editor |
| Preferences | OnEditPreferences | MISSING | full options dialog (see Preferences section) |
| Save Camera As Default | OnEditSaveCameraAsDefault | OK | `editSaveCameraAsDefault()` |
| PlayPMO | OnEditPlaypmo | MISSING | debug replay (PMO) |
| Rebuild World | OnEditRebuildworld | OK | |
| Update Surface | OnEditUpdateSurface | OK | |
| Change Total World Param | OnEditChangetotalworldheight | OK | ChangeTotalWorldHeightDialog |
| Rolling Border | OnEditRollingborder | OK | BorderRollingDialog |
| Triggers | OnEditTriggers | STUB | `editTriggers()` picks file, editor TODO |
| Units | OnEditUnits | STUB | Library editor (kdw) |
| Objects (editors?) | OnEditObjects | MISSING | |
| User Interface | OnEditUserInterface | STUB | launches external UIEditor.exe |
| Effects | OnEditEffects | STUB | |
| Sounds | OnEditSounds | STUB | |
| TerTools | OnEditTertools | STUB | |
| Cursors | OnEditCursors | STUB | |
| Heads | OnEditHeads | STUB | |
| Sound Tracks | OnEditSoundTracks | STUB | |
| Reels | OnEditReels | STUB | (original is a no-op too → N/A-ish) |
| Command Color | OnEditCommandColor | STUB | |
| UITextSprites | OnEditUITextSprites | STUB | |

### View
| Command | Original handler | Port | Notes |
|---|---|---|---|
| Sources | OnViewSources | STUB | original flips `surMapOptions.showSources_` + `Environment::flag_ViewWaves`; visibility read by EditorVisual::isVisible |
| Cameras | OnViewCameras | STUB | same pattern (`showCameras_`) |
| Geosurface | OnViewGeosurface | STUB | original is commented out (no-op) → N/A |
| Show Grid | OnViewShowGrid | OK | `viewToggleGrid` → EngineViewport gridVisible_ |
| Camera Borders | OnViewShowCameraBorders | STUB | `showCameraBorders_` + GeneralView::drawGrid |
| Path Finding | OnViewPathFinding | STUB | `showPathFinding_` + EditorVisual aux unit |
| Path Finding - Select Ref Unit | OnViewPathFindingReferenceUnit | STUB | kdw TreeSelectorDialog |
| Time Flow | OnViewEnableTimeFlow | OK | `viewToggleTimeFlow` (state only) |
| Hide Models | OnViewHideModels | STUB | `hideWorldModels_` → EditorVisual::isVisible |
| Animation | OnViewAnimation | OK | `actToggleAnimation_` |
| Time Slider (dock) | (CTimeSliderDlg in filtersBar) | PARTIAL | modal dialog port only; original is a modeless child of the filters bar |
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
| Editable Tools Tree | OnViewExtendedmodetreelbar (debug) | STUB | property only |
| Save Tools Tree | OnDebugSaveconfig | OK | `saveState()` |
| Save config | OnDebugSaveconfig | STUB | should also persist view/panels state |
| Edit ZipConfig | OnDebugEditZipConfig | STUB | read-only text view of Scripts\Content\ZipConfig (no save) |
| Edit debugPrm | OnDebugEditDebugPrm | STUB | read-only text view of Debug.dat (no save) |
| Edit AuxAttribute | OnDebugEditAuxAttribute | MISSING | kdw library editor |
| Edit RigidBodyPrm | OnDebugEditRigidBodyPrm | MISSING | |
| Edit Toolzer | OnDebugEditToolzer | MISSING | |
| Edit ExplodeTable | OnDebugEditExplodeTable | MISSING | |
| Edit SourcesLibrary | OnDebugEditSourcesLibrary | MISSING | |
| UI Sprite Lib | OnDebugUISpriteLib | MISSING | |
| Show Palette Texture | OnDebugShowpalettetexture | OK | |
| Show Mipmap | OnDebugShowmipmap | STUB | no engine effect |

## Panels

| Panel | Original | Port | Notes |
|---|---|---|---|
| Tools tree | CToolsTreeWindow / CToolsTreeCtrl | STUB | ToolsTreePanel shows only the 4 transform tools + a static tree; original built a real per-tool tree from FactorySelector<CSurToolBase>, persisted in ToolsTreeCtrl::serialize, with popup Create/Delete/Properties |
| Objects Manager | CObjectsManagerWindow | PARTIAL | tabs + objectList() now show data; **drag&drop, selection sync (selectObject/worldObjectSelected), rename/delete via popup, context menu** not wired |
| Properties | propertiesBar_ hosting the current CSurToolBase dialog | MISSING | propertiesDock_ exists with NO widget; tools' per-tool property pages not ported |
| Minimap | CMiniMapWindow | PARTIAL | MiniMapPanel (see its own notes) |
| Gradients | CGradientsWindow | PARTIAL | GradientsPanel exists |
| Camera control | (camera controls) | PARTIAL | CameraControlPanel exists |
| Time slider | CTimeSliderDlg modeless in filtersBar | PARTIAL | only modal dialog; not embedded |
| Wave dialog | CWaveDlg floating | STUB | WaveDialog: create/remove/apply all say "needs environment->fixedWaves()" — but Universe now runs with an Environment; wire fixedWaves |
| Camera dialog | CCameraDlg (IDD_DLG_CAMERA) | STUB | CameraDialog create/delete/play say "needs cameraManager" — cameraManager now runs; wire splines + mouse CREATE_POINTS mode |

## Tools (SurTool*, the map-editing tools)

The original registered ~36 tool types through `FactorySelector<CSurToolBase>`
(SurMap5/SurTool*.h/.cpp) and stored the per-world tool tree in
`ToolsTreeCtrl::serialize`. The Qt port has only the 4 transform tools
(Select/Move/Rotate/Scale) and they act on **no selection** (the tools'
selection state is empty — SurMap5Qt/src/tools are "Phase 3b" stubs that
store poses but nothing selects world objects yet).

Not yet ported (original class → what it does):
- CSurTool3DM / CSurToolEnvironment — place a .3dx model / environment object on the map.
- CSurToolKind — edit surface kind (terrain type) by brush.
- CSurToolColorPic — paint a texture ("Color picture") on the terrain.
- CSurToolToolzer — raise/lower/flatten terrain with a brush (toolzer).
- CSurToolMiniDetaile / CSurToolMiniDetaileFolder — mini-details placement.
- CSurToolRoad — road drawing.
- CSurToolWater / CSurToolWaves / CSurToolWindStatic — water level / waves / wind zones.
- CSurToolLighting — lighting (sun) editing.
- CSurToolSource — extraction/resource source placement.
- CSurToolUnit / CSurToolUnitFolder / CSurToolPlayerFolder — place units / assign player.
- CSurToolPathEditor — path (waypoint) editing.
- CSurToolZone... / CSurToolSpecFilter — zone/spec-filter editing.
- CSurToolSelect — real marquee that selects world objects (the port's
  SelectTool only rubber-bands; nothing is selected).

Dependency note: every real placement tool needs (a) the tool property page
in the properties dock (was a kdw dialog — must become a Qt form), and (b) a
pick-from-scene / click-to-place path through EngineViewport that ends in
`worldPlayer()->buildUnit(...)` / vMap mutations. The transform tools also
need object selection before Move/Rotate/Scale do anything.

## Camera / Environment editing

- **CameraDialog**: original CCameraDlg switched CGeneralView into
  mouseMode CREATE_POINTS / SELECT_POINTS / EDIT_POINTS and edited
  `cameraManager->splines()` points on the map. Port's dialog buttons are all
  stubs. Now that cameraManager exists after a world load, this can be wired
  (list splines, add points by clicking the map, set time/cycling).
- **WaveDialog**: original CWaveDlg added/removed fixed waves via
  `environment->fixedWaves()` and applied them. Port's three buttons are
  stubs. Universe's Environment is alive now; wire fixedWaves.
- **TimeSlider**: original was a modeless child of the filters bar that
  nudged `environment->environmentTime()->...` (time of day + time flow).
  Port has only a modal dialog that stores a float. To actually change the
  scene light, drive `environment->environmentTime()` / sun from the dialog.

## Editor state & persistence

- SurMapOptions (SurMap5/SurMapOptions.h) — showSources_/showCameras_/
  hideWorldModels_/showPathFinding_/enableGrid_/gridSpacing_/gridColor_/
  cameraBorder*_/last_dirs_/dlgBarState. The port kept gridVisible_ inside
  EngineViewport and has no equivalent struct. A Qt `SurMapOptions`
  (QSettings-backed) would let the View filters and the View menu checks
  persist and actually drive rendering (via EditorVisual::isVisible ports).
- `EditorVisual::isVisible` (SurMap5/EditorVisualImpl.cpp) — the per-class
  visibility hook the renderer asks. Not ported; without it the View toggles
  (Sources/Cameras/Hide Models) cannot affect the 3D view.
- Tools tree persistence (ToolsTreeCtrl::serialize reads/writes
  "Scripts\TreeControlSetups\..." files). Port saves only QSettings state.

## External-tool editors (kdw-based library editors)

Units, Effects, Sounds, UI Message Types, UI Messages, UI Show Mode Sprites,
Sound Tracks, Heads, TerTools, Cursors, Command Colors, Text Images, Terrain
Type Name — all open a `kdw::LibraryEditorDialog` over a LibraryWrapper
singleton (editLibrary()). kdw is not built on any platform now
(Util/kdwStub.cpp), so these need a Qt re-implementation of
LibraryEditorDialog: a tree of library elements + a serialized property form
(Serializer). This is the single biggest missing chunk (a generic
"library editor" widget), and most of the Libraries menu rides on it.

## Priority proposal

1. **Properties dock content + tool property pages** (currently an empty
   dock) — start with the transform tools' minimal pages (position/rotation/
   scale readout), then per-tool forms as tools land.
2. **Real object selection in the 3D view** (pick unit/environment/source
   under cursor, sync with Objects Manager selection). Unlocks every SurTool
   port and the Objects Manager rename/delete.
3. **EditorVisual::isVisible + SurMapOptions** so the View menu toggles
   (Sources, Cameras, Hide Models, Grid persists) actually drive rendering.
4. **Objects Manager interaction**: context menu delete/rename for real
   (UnitTreeObject/SourceTreeObject select+kill), drag&drop to move objects.
5. **CameraDialog wiring** (cameraManager splines are loaded; add/select/
   edit points on the map).
6. **Terrain brushes** (SurToolToolzer: raise/lower/level; SurToolKind:
   surface type; SurToolColorPic: texture paint) — the core terrain editing
   loop that makes the tool tree meaningful.
7. **Placement tools**: SurToolUnit (pick from AttributeLibrary),
   SurTool3DM/Environment (file-pick a model), SurToolSource.
8. **Wave dialog wiring** (environment->fixedWaves()).
9. **Qt Library editor** (replaces kdw::LibraryEditorDialog) → promotes the
   whole Libraries menu.
10. **Time slider as modeless filters-bar child** driving environment time.
11. Smaller MISSING items: Save Without terTool Color, Resave All Triggers,
    Save VoiceFile Durations, Update quick start list, Map Preset,
    Preferences, PlayPMO, camera borders overlay, extended tree-bar mode.
