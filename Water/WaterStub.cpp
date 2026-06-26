// Placeholder translation unit for the non-Windows build.
//
// The entire Water module (water surface, circles, clouds, sky, foliage,
// coast sprites, ice, ...) is a visual/render module: nearly every source
// includes the D3D9 render headers (Render/D3D/D3DRender.h, shaders.h) and
// calls into the IDirect3DDevice9 interface. Like the renderer itself, it is
// excluded from the cross-platform build for now and will be restored on top
// of the eventual SDL GPU backend (Track B) / Render stub LIB.
//
// This file exists only so the Water static library has at least one object
// on non-Windows platforms.
