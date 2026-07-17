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

## Ported since the retirement

| # | Feature | Now lives in |
|---|---------|--------------|
| 1 | **Grass** | `Render/SDLGrassRenderer.{h,cpp}` + `Render/SDLShaders/grass.{vert,frag}.hlsl`, driven from `GrassMap::Draw`/`DrawGrass` as the original drove `VSGrass`/`PSGrass`. Everything but the draw (tile grid, blade generation, sort, buffers) had always been portable. The alpha test (`D3DRS_ALPHAREF 100`) has no SDL GPU equivalent and became a `clip()` in the fragment shader. |
| 2 | **Distance fog** | `cSDLRenderDevice::SetGlobalFog` + `fogPlane()`, and a fog term at the end of every world fragment shader: terrain, objects, grass, water, and the world quads (particles, coast foam, wave sources, light columns). Driven from `Environment::graphQuant`, as before. The world-quad renderer has two rules, since its groups are not all occluders: additive/subtractive ones fade to nothing (the original's `FIX_FOG_ADD_BLEND`), the rest fade toward the fog colour. Three cameras keep fog OFF and must stay that way — the sky (`cSkyObj::Draw`), the lightmap (`CameraPlanarLight::DrawScene`) and the 2D pass (`Camera::Set2DRenderState`). The under-water post-effect's fog override, once blocked on #6, is back too (#6a). |
| 3 | **The perimeter field dome** | No renderer of its own: it is a `SetWorldMaterial` caller with `sVertexXYZDT2` geometry, so it goes through **`SDLWorldQuadRenderer`'s triangle route**, which already had the blend, the two textures and the `COLOR_ADD` operation. Added there: `DrawIndexedPrimitive` (the dome's tile grid shares vertices, so neither a strip nor a list expresses it) and the `ZREFLECTION` variant in `worldtri.{vert,frag}.hlsl` — the height clip against the water's A8L8 map that stops the dome showing through the hills. Also restored `tilemap_inv_size`, a D3D-only global `cTileMap`'s constructor sets. |
| 5 | **Cloud shadows** | `Render/SDLCloudShadowRenderer.{h,cpp}` + `Render/SDLShaders/cloudshadow.{vert,frag}.hlsl`, driven from `cCloudShadow::Draw`. Note this is a **lightmap** effect, not a view one: `cCloudShadow` carries `ATTRCAMERA_SHADOW`, which is the planar *light* camera's attribute, so its world-sized quad overwrites the terrain lightmap (`ALPHA_NONE`, `sortIndex -1`) before `drawLights()` blends over it. The shader is centred on 0.5 — the lightmap's neutral — so the terrain and the grass pick the clouds up through the lightmap term they already sample. Neither of their shaders changed. |
| 6a | **Post effects: monochrome + under-water** (the reachable half of the stack — pause and the campaign triggers use monochrome; the under-water effect is on by default and carries the fog override) | `Render/SDLPostEffectRenderer.{h,cpp}` + `Render/SDLShaders/{posteffect.vert,copy.frag,monochrome.frag,underwater.frag}.hlsl`, recorded from the (still live) `PostEffectManager` effects in `VistaRender/postEffects.cpp` and composited by `cSDLRenderDevice::drawPostEffects`. **The frame is turned around**: D3D9 `StretchRect`ed the back buffer per effect, but SDL GPU cannot sample the swapchain — so when `Environment::graphQuant` knows an effect will draw (`PostEffectManager::anyEffectWillDraw`), it arms a scene-capture target (`armSceneCapture`) that stands in for the screen for the whole scene walk, and the chain composites capture → swapchain before the UI pass. Zero cost on the (nearly all) frames with no active effect. The under-water fog override (`PostEffectUnderWater::setFog`) is back in `graphQuant`, closing the gap left by #2. |
| 6b | **Lens flare** (`isEnabled` in 26 of 28 shipped worlds, daytime) | `Render/src/LensFlare.cpp` draws again: the 2D sprite chain through the UI batch (`DrawSprite`, which now honours `ALPHA_ADDBLENDALPHA` — `SDLUIRenderer` grew a per-run blend), the world-space glow billboard through `SDLWorldQuadRenderer`'s quad route. Two divergences, both deliberate: SDL GPU has **no occlusion queries** (#20), so the D3D 25-point ring test became a CPU test — sun on screen and in front of the camera, edge-faded — which gives up occlusion by terrain/objects; and the sprites ride the UI batch, which replays *after* the post-effect composite, so monochrome does not grey the flare as it did on D3D9. |
| 10 | **Fog of war** | Another **lightmap** effect, and the reason that map has an alpha channel at all. The lightmap packs two unrelated things: the light sources live in RGB, the fog of war in **alpha**. `FogOfWar::Draw` writes the alpha with one world-sized `ALPHA_NONE` quad through `SDLWorldQuadRenderer`'s quad route (no renderer of its own), masked to alpha with the new `SetColorWriteMask` (#16); `CameraPlanarLight::drawLights` now masks the other way, to RGB, so the light quads cannot punch holes of clear visibility through the fog. The consumer is one line restored at the end of `tilemap.frag.hlsl` and `grass.frag.hlsl` — `ot.rgb = lerp(ot.rgb, vFogOfWar, lightmap.a)`, after the shadow and before the distance fog, where the originals put it — gated on `LightMapParams.y` and fed from `cSDLRenderDevice::SetFogOfWar`, which `cScene::Draw` drives as it drove `cD3DRender::SetFogOfWar`. The coverage map (`FogOfWar::UpdateTexture`, the sight circles, `FogOfWarMap`) was always portable and always running. **Caveat, and it is a big one: no shipped world switches this on.** `is_fog_of_war` is gated out of `MissionDescription::serialize` by `GlobalAttributes::enableFogOfWar`, which was false when these worlds were saved, so the key is absent from every `.spg` and the text archive leaves it at `false` — and `Universe.cpp`'s `if(environment->fogOfWar())` gates the *simulation* on the same object. Fog of war is therefore off in all of retail P2, on D3D9 as much as here. This restores the capability, not a visible fix; to see it, set `is_fog_of_war` on a world (the editor exposes it). Verified by forcing that flag: shroud, sight circles, and grass darkening with the ground under it. |
| 10b | **`FieldOfViewMap`** | The fog of war's sibling: the *other* quad into the terrain lightmap, and the reason the collision in #10 was worth spelling out. The sight sectors live in the lightmap's **RGB** (a per-player colour tint), the fog of war in its **alpha**. `FieldOfViewMap::Draw` ports the original `ALPHA_BLEND` quad through `SDLWorldQuadRenderer`'s quad route, but masked to **RGB** with `SetColorWriteMask` (#16) — the same recipe `CameraPlanarLight::drawLights` uses. That resolves the collision the D3D9 version left implicit: its alpha (`128 + (color.a*visibility >> 9)`, see `updateTexture`) would otherwise land in the channel the fog of war now owns and read as coverage. Masking it away costs nothing — `FogOfWar::Draw` (sortIndex 10) overwrote that alpha after us (sortIndex 0) when fog was on, and nothing samples it when fog is off, so the alpha write was never meaningfully consumed even on D3D9. The tint is packed centred on 128 = 0.5 = the lightmap's neutral, so untraced ground contributes nothing and the terrain/grass pick a traced cell up through the `light += 2*(lightmap.rgb - 0.5)` term they already sample — no shader change. `updateTexture()` is rewired at the end of `Environment::graphQuant` where the original drove it, reading `sdlRenderDevice()->planarTransform()` (the D3D `gb_RenderDevice3D` is null off-Windows). **Same caveat as #10: no shipped content switches it on.** `showSightSector` defaults `false` on the unit attribute and no `.spg`/attribute sets it, so `IronLegion`/`IronBuilding` never call `add()` — field of view is off in all of retail P2, on D3D9 as much as here. Verified by forcing the flag: per-unit sight discs tinted into the ground, decaying as units move, under the normal lighting. |
| 12 | **Float Z-buffer camera → the scene-depth snapshot** (its one live consumer: the soft-particle fade — `OPTION_SOFT_SMOKE` drives the emitters' `softSmoke` key, and the coast sprites always asked for it) | The original existed because D3D9 could not sample its own depth buffer: a child camera re-rendered terrain/objects/grass depth into a float texture (`ZBuffer/*.vsl` store pre-divide clip z), and `standart.psl`'s `FLOAT_ZBUFFER` faded a billboard by `saturate((zb − zpos.z) * 0.1)`. SDL GPU samples depth directly, so `cSDLRenderDevice::snapshotSceneDepth` instead **copies the frame's depth buffer once per frame** at the opaque→transparent boundary (first soft group in `drawWorldQuads` pays; the water reflection and other offscreen targets draw with the fade off), and `worldquad.frag.hlsl` converts both the sample and the fragment's own hardware depth back into **the original's clip-z units** through the camera's projection constants (`clipZOf`; note `Camera::Update` leaves `matProj._44` at 1, so all four constants matter) — the `0.1` falloff carries over verbatim. No second scene walk, no depth-output shader variants. One divergence: the snapshot holds the whole opaque scene, so option value 1 ("terrain only") behaves as 2 ("objects and terrain"). The other consumer of the float map, DOF, is still #6. |

## Features with no SDL path

| # | Feature | Where it died | What the original did |
|---|---------|---------------|-----------------------|
| 4 | **Lava + ice terrain materials** | `cTileMap::setMaterial` (deleted); `Water/ice.cpp` — `cTemperature::Draw` | `ShaderSceneWaterLava` / `ShaderSceneWaterIce` over the placement-zone materials. |
| 6 | **Post-effect stack, the remainder** (monochrome, under-water and the lens flare are ported — #6a/#6b above; the capture + fullscreen-pass infrastructure they needed now exists) | `VistaRender/postEffects.cpp` — the effects `PostEffectManager::createEffect` declines to create | **DOF** — reachable on the top graphics preset. Its depth source now exists — #12's scene-depth snapshot is exactly the full-screen float map `PostEffectDOF::redraw` sampled — so what is left is the effect itself (the blur + combine passes) and a decision on #13 below. Note `cVisGeneric::SetEnableDOF` still gates on `gb_RenderDevice3D->IsPS20()`, which is permanently false. **Bloom + the screen flash** — the manager masks bloom out of creation (`enabledEffects_ &= ~(1 << PE_BLOOM)`, there since the init commit), so the nuke flash was invisible on shipped D3D9 too; kept masked on purpose — porting it would invent behaviour retail never showed. **Mirage** — an inverted-logic bug in `PostEffectMirage::createTextures` kept it from ever drawing on D3D9, and it needs the mirage camera (#11). **Colour-dodge** — zero uses in shipped content. Check reachability before porting any of these. |
| 7 | **`cEnvironmentEarth`** | `Water/Water.cpp` | The ground plane drawn under the water out to the horizon. |
| 8 | **`cFogCircleEX`** | `Water/SkyObject.cpp` | Fixed-function horizon ring shaded from `D3DRS_TEXTUREFACTOR`. |
| 9 | **Sky cubemap** | `EnvironmentTime::Draw` | A cube render target. Note: it had **no consumer** even on D3D — see the note in `MEMORY`/history before spending time on it. |
| 11 | **Mirage camera** | `cScene::AddMirageCamera` (deleted) | Heat-haze render target, composited over the frame. |
| 13 | **Water depth prepass** | `cWater::DrawToZBuffer` | Stamped the water surface into #12's float map so depth effects saw the water, not the terrain under it. **Dead at `ca9aa43` already**: its one call site (`cWater::Draw`, Water.cpp:278) is commented out, so retail D3D9 shipped soft particles and DOF fading against the bottom, not the surface. If DOF (#6) is ever ported, decide then whether to resurrect this (a depth-only water pass into #12's snapshot) or keep retail behaviour. |
| 14 | **Render-target debug viewers** | `Camera::DrawShadowDebug`, `TempDrawShadow` (deleted) | The `Option_ShowRenderTextureDBG` overlays that blit the shadow/reflection/float maps to the corner of the screen. Dev tooling, but genuinely useful — worth restoring early. |
| 23 | **The KD-lab logo splash** (`cBlobs`) | `UserInterface/Bubles/Blobs.cpp`; `ReelManager::showLogoModal`, now guarded off | The fish swimming under a screen full of metaballs. The *scene* half would run today (`cScene`/`cObject3dx` have an SDL path); the effect half is D3D9 through and through — cells drawn with the `cQuadBuffer` family (#15), composited by `PSBlobsShader` out of the retired shader system, plus a render target, an `IsPS20()` capability check and a `SetVertexShader`, none of which the portable interface has. The loop also `StretchRect`s the back buffer into a texture; the SDL way is to point the camera at a render target — the scene-capture machinery #6a added (`cSDLRenderDevice::armSceneCapture`) is exactly that. **Latent crash until the video port**: it is reached only through `ActionShowLogoReel`, which `DisableVideo` used to keep from ever firing. |
| 24 | **`cChaos`** — the seething void drawn under and around the map when a world's `Outside` is `ENVIRONMENT_CHAOS` | `Render/src/CChaos.cpp` — `cChaos::Draw` returns early | The dual-bump-EMBM disc mesh (`Chaos/chaos.{vsl,psl}`), plus the `sBumpTile::SetVertexZ` hole-sinking in the terrain. **No shipped P2 world uses `ENVIRONMENT_CHAOS`** — it is a Perimeter-1 leftover, reachable only from the editors. Like the sky cubemap (#9): check for a consumer before spending time here. Its `DrawSprite2` path (#18) was dead even at `ca9aa43`. |

## Device capabilities the above need first

| # | Capability | Where |
|---|------------|-------|
| 15 | **Dynamic vertex/quad buffers** — `GetBufferXYZD`, `GetQuadBufferXYZDT1`, … all return `nullptr` | `cSDLRenderDevice`. ~25 files called them. **Check what a feature really uses before believing it is blocked on this — the register has been wrong three times.** Grass draws from *static* `sPtr` buffers the device already implements; the field dome had its own dynamic VB and now goes through `SDLWorldQuadRenderer`'s triangle route; the lens flare's screen quads ride the UI batch and its glow the world-quad route (#6b). What is genuinely left: `cLeaves`, and the colour-dodge's rotating quad if that effect is ever ported. |
| ~~16~~ | ~~**Colour-write masks**~~ — **done**, in `SDLWorldQuadRenderer` | `SetColorWriteMask(eColorWriteMask)`, folded into the pipeline key: SDL GPU only consults `color_write_mask` when the pipeline sets `enable_color_write_mask`, so "write everything" stays the zeroed default. It is a *render state*, not part of the material — set around a run of groups and restored, as `D3DRS_COLORWRITEENABLE` was. Only the two lightmap callers use it (#10). No other renderer has one, and nothing else has asked. |
| 17 | **3D debug primitives** — `DrawLine`, `DrawPoint`, `drawCircle`, `DrawBound`, `FlushPrimitive3D` | `cSDLRenderDevice`, all no-ops. Used by `cTileMap::DrawLines` and the debug overlays. |
| 18 | **`DrawSprite2` / `DrawSpriteScale` / `DrawSpriteSolid`** | `cSDLRenderDevice`, all no-ops. Only `CChaos` (#24) uses them — and only on a path that was already dead at `ca9aa43`. |
| 19 | **Two-pass Z prepass for fading objects** | `cSimply3dx::Draw` — the `IsDraw2Pass()` path. Without it a fading object blends with itself. |
| 20 | **Occlusion queries** | `cOcclusionQuery` — stubbed (`Init()` false, `VisibleCount()` 0). SDL GPU exposes none, so a real port means depth readback or a counting pass. The one caller that mattered, the lens flare, no longer waits on it: #6b replaced the ring test with a CPU visibility check. |
| 21 | **Screenshots** (`SetScreenShot`), **gamma** (`SetGamma`) | `cSDLRenderDevice`, no-ops. |
| 22 | **Fullscreen mode** | `PlatformWindow::create` always makes a windowed window; `IsFullScreen()` returns false. `OPTION_FULL_SCREEN` is ignored. |

## The dead D3D9 source is kept on purpose — do not "tidy" it away

`Render/shader/shaders.cpp`, `Render/shader/ShaderStorage.cpp`, `Render/src/CChaos.cpp` and
`Render/src/RenderCubemap.cpp` still compile, and still contain their full D3D9
implementations. **This is deliberate**: they are the reference for the ports above, and
`Environment` / `SkyObject` still construct and configure them (the settings come out of
world data). Nothing in them runs.

`VistaRender/postEffects.cpp` and `Render/src/LensFlare.cpp` are now **partly live**: the
manager, monochrome, under-water and the flare run on SDL (#6a/#6b), while the unported
effect classes in postEffects.cpp (bloom, DOF, mirage, colour-dodge) remain D3D9 reference
bodies that `createEffect` declines to construct.

They are inert because `gb_RenderDevice3D` is permanently null and the draw entry points return
early. If you add a new entry point into any of them, guard it the same way — a bare
`gb_RenderDevice3D->` or a `static_cast<cD3DRender*>(gb_RenderDevice)` is a null dereference or
a bad cast now, not a Windows-only path.

Getting them out of the build means unpicking `Environment`'s `PostEffectManager` / `Flash` /
`cChaos` plumbing and `SkyObject`'s cubemap, while keeping the serialization intact so world
data still loads. Do that when the features are ported, not before.

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
