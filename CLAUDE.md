# VistaEngine

Old Windows RTS game (Perimeter 2). Goal: make it cross-platform (Windows, Linux, macOS).

## Cross-platform migration targets

- **Build system:** CMake (replacing MSVC `.sln`/`.vcproj`)
- **Windowing/input:** SDL 3
- **GPU:** SDL GPU API with HLSL shaders, cross-compiled to SPIR-V (Vulkan) and MSL (Metal) via DirectXShaderCompiler
- **Platforms:** Windows, Linux, macOS

## C++ standard

C++20. The original codebase was compiled with old Visual C++ (mixed C and C++). All new and migrated code targets C++20.

## Branching strategy

- `crossplatform` — long-lived integration branch for the cross-platform port
- Feature branches cut from `crossplatform`, one per meaningful change, merged back via PR

## Current state

CMake is the source of truth and builds the game on macOS. The MSVC `.sln`/`.vcproj` files
are stale — they still reference the deleted D3D9 backend, and the tool projects (SurMap5,
ModelViewer, VistaEditor, …) have not been migrated.

**The renderer is SDL GPU on every platform, including Windows. There is no D3D9 backend.**
The `#ifdef _WIN32` guards that used to select between the two are gone; a `_WIN32` guard in
this tree now means a genuine OS-API difference (see below), never a rendering one.

The engine is **64-bit everywhere**. The shipped `.3dxG` / ContentBin caches are raw 32-bit
memory images, so they are reconstructed field-by-field rather than relocated in place
(`Util/Serialization/InPlaceArchive.h`, `Render/3dx/Static3DX.cpp`).

### What is not ported yet

**`Render/PORTING.md` is the register** — read it before touching the renderer. It lists the
features that existed only in the D3D9 branches and were deleted with them (grass, distance
fog, the perimeter field dome, lava/ice terrain, cloud shadows, the post-effect stack, …),
what each one did, and what has to exist first. Sites are tagged `TODO(sdl-port)`.

Two conventions matter:
- **`ca9aa43` is the last commit with the D3D9 implementations intact.** Read the original
  next to any port: `git show ca9aa43:Render/src/Grass.cpp`.
- The original shader sources (`Render/shader/**/*.vsl`, `*.psl`) are kept **on purpose**.
  Read the original before reimplementing an effect; don't guess with a procedural stand-in.

Dead D3D9 source still compiles but never runs: `Render/shader/shaders.cpp`,
`ShaderStorage.cpp`, `Render/src/{CChaos,RenderCubemap,LensFlare}.cpp`,
`VistaRender/postEffects.cpp`. It is kept as the reference implementation for the ports above.

Audio (DirectSound), networking (DirectPlay), joystick (DirectInput) and video (Bink/AVI) are
still Windows-only, stubbed elsewhere. Those are separate from the D3D9 retirement.
