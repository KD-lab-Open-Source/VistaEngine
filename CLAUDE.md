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

Legacy Windows-only codebase. MSVC project files are the source of truth until CMake is in place. No CMake files exist yet.
