# Renderer porting register — what D3D9 did that SDL GPU does not yet

The D3D9 backend is gone; SDL GPU is the renderer on every platform. Most of the engine's
rendering came across, but the features below did **not**: they existed only in the D3D9
branches, and those branches were deleted rather than carried as dead code.

Nothing here is lost. **`ca9aa43` is the last commit with the D3D9 implementations intact** —
read the original alongside the port:

```sh
git show ca9aa43:Render/src/Grass.cpp          # the D3D9 draw path for any file below
git show ca9aa43 --stat                        # the tree as it was
```

The original HLSL/asm shader sources are still in the repo under `Render/shader/**/*.vsl`,
`*.psl` — they were kept deliberately. **Read them before reimplementing an effect.**

Every site is marked in the source with a greppable tag:

```sh
grep -rn "TODO(sdl-port)" --include=*.cpp --include=*.h .
```

---

## Features with no SDL path

| # | Feature | Where it died | What the original did |
|---|---------|---------------|-----------------------|
| 1 | **Grass** | `Render/src/Grass.cpp` — `GrassMap::Draw` / `DrawGrass` are empty | `VSGrass`/`PSGrass` + per-card `PSGrassShadow` variants, alpha-tested wind-animated blades out of dynamic VBs. Everything but the draw (tile grid, blade generation, sort, buffers) still runs. Shaders: `Render/shader/Grass/`. |
| 2 | **Distance fog** | `cSDLRenderDevice::SetGlobalFog` is a no-op; `Environment::graphQuant` no longer sets it | D3D fixed-function global fog, colour + range driven by the time of day. Affects every world shader. |
| 3 | **The perimeter field dome** | `VistaRender/Field.cpp` — `FieldDispatcher::Draw` | The game's signature effect: an additive `sVertexXYZDT2` tile strip over the water, sampling the reflection texture. |
| 4 | **Lava + ice terrain materials** | `cTileMap::setMaterial` (deleted); `Water/ice.cpp` — `cTemperature::Draw` | `ShaderSceneWaterLava` / `ShaderSceneWaterIce` over the placement-zone materials. |
| 5 | **Cloud shadows** | `Water/CloudShadow.cpp` — `cCloudShadow::Draw` | `VSCloudShadow`/`PSCloudShadow`, a scrolling shadow layer modulated by sun elevation. |
| 6 | **Post-effect stack** | `Environment::drawPostEffects` | `PEManager`, screen flash, lens flare, the under-water effect, `CChaos`. Needs render-to-texture + fullscreen passes. |
| 7 | **`cEnvironmentEarth`** | `Water/Water.cpp` | The ground plane drawn under the water out to the horizon. |
| 8 | **`cFogCircleEX`** | `Water/SkyObject.cpp` | Fixed-function horizon ring shaded from `D3DRS_TEXTUREFACTOR`. |
| 9 | **Sky cubemap** | `EnvironmentTime::Draw` | A cube render target. Note: it had **no consumer** even on D3D — see the note in `MEMORY`/history before spending time on it. |
| 10 | **Terrain lightmap alpha / fog of war** | `Render/src/FogOfWar.cpp` — `FogOfWar::Draw`; `VistaRender/FieldOfView.cpp` | An alpha-only quad into the lightmap's alpha channel. Blocked on a colour-write mask (#14). |
| 11 | **Mirage camera** | `cScene::AddMirageCamera` (deleted) | Heat-haze render target, composited over the frame. |
| 12 | **Float Z-buffer camera** | `cScene::AddFloatZBufferCamera`, `Camera::ClearFloatZBuffer`, `Camera::DrawToZBuffer` (deleted) | A float depth target for depth-of-field / soft particles. |
| 13 | **Water depth prepass** | `cWater::DrawToZBuffer` | Feeds #12. |
| 14 | **Render-target debug viewers** | `Camera::DrawShadowDebug`, `TempDrawShadow` (deleted) | The `Option_ShowRenderTextureDBG` overlays that blit the shadow/reflection/float maps to the corner of the screen. Dev tooling, but genuinely useful — worth restoring early. |

## Device capabilities the above need first

| # | Capability | Where |
|---|------------|-------|
| 15 | **Dynamic vertex/quad buffers** — `GetBufferXYZD`, `GetQuadBufferXYZDT1`, … all return `nullptr` | `cSDLRenderDevice`. ~25 files called them. `SDLWorldQuadRenderer` covers the quad cases that were ported; the rest (grass, leaves, field, lens flare) need it or an equivalent. |
| 16 | **Colour-write masks** (alpha-only / RGB-only passes) | Needs a pipeline variant per mask in the SDL renderers. Blocks #10. |
| 17 | **3D debug primitives** — `DrawLine`, `DrawPoint`, `drawCircle`, `DrawBound`, `FlushPrimitive3D` | `cSDLRenderDevice`, all no-ops. Used by `cTileMap::DrawLines` and the debug overlays. |
| 18 | **`DrawSprite2` / `DrawSpriteScale` / `DrawSpriteSolid`** | `cSDLRenderDevice`, all no-ops. Only `CChaos` (#6) uses them. |
| 19 | **Two-pass Z prepass for fading objects** | `cSimply3dx::Draw` — the `IsDraw2Pass()` path. Without it a fading object blends with itself. |
| 20 | **Occlusion queries** | `cOcclusionQuery` — stubbed to "always visible". |
| 21 | **Screenshots** (`SetScreenShot`), **gamma** (`SetGamma`) | `cSDLRenderDevice`, no-ops. |
| 22 | **Fullscreen mode** | `PlatformWindow::create` always makes a windowed window; `IsFullScreen()` returns false. `OPTION_FULL_SCREEN` is ignored. |

## The `_WIN32` guards that are still there on purpose

Every `#ifdef _WIN32` in the rendering code is gone, and so is the one in the in-place
serializer (the engine is 64-bit on every platform now, and reconstructs the 32-bit cache
images by hand rather than relocating them — see `Util/Serialization/InPlaceArchive.h`).

What remains are genuine OS-API guards, and deleting them would be wrong:

- **`Render/3dx/Saver.h`** — Win32 `_open`/`_read` vs POSIX `open`/`read`.
- **`Render/src/ftrender.cpp`** — `NormalizePath` (backslash → POSIX path, plus case
  resolution on case-sensitive volumes) exists only off-Windows, and is not wanted on Windows.
- **`Render/src/FileImage.cpp`** — the Video-for-Windows AVI reader.
- **`Game/Runtime.cpp`** — `_beginthread` for the threaded mission load; elsewhere the load
  is synchronous. A threading port, not a rendering one.

## Not the renderer, but still Windows-only

These block a genuinely single-platform-behaviour build and are tracked here so they aren't
forgotten, but they are separate from the D3D9 retirement:

- **Audio** — DirectSound (`Sound/`); off-Windows is a silent stub (`SoundStub.cpp`).
- **Networking** — DirectPlay 8 (`Network/`); off-Windows is a no-op stub (`NetCenterStub.cpp`).
- **Joystick** — DirectInput 8 (`Util/joystick.cpp`).
- **Video** — Bink (`Game/PlayBink.cpp`) and Video-for-Windows AVI (`Render/src/FileImage.cpp`).
  The main-menu backdrop is a Bink video.
- **Editors** — `kdw`, `TriggerEditor`, `ShaderToH2`, and the MSVC-only tool projects
  (`SurMap5`, `ModelViewer`, `VistaEditor`, …), which still reference the deleted D3D9 backend.
