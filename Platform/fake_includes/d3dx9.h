#pragma once
#include "d3d9.h"
// D3DX9 utilities — stub for non-Windows builds
struct D3DXVECTOR2 { float x, y; };
struct D3DXVECTOR3 { float x, y, z; };
struct D3DXVECTOR4 { float x, y, z, w; };
struct D3DXCOLOR   { float r, g, b, a; };

// D3DX helper functions invoked from inline render-header code. Variadic
// template stubs so any argument list the headers pass compiles; the real
// implementations live in the excluded renderer .cpp / future render stub LIB.
template<class... A> HRESULT D3DXCreateTextureFromFileInMemoryEx(A...) { return 0; }
template<class... A> HRESULT D3DXCreateCubeTextureFromFileInMemory(A...) { return 0; }
template<class... A> HRESULT D3DXOptimizeVertices(A...) { return 0; }
template<class... A> HRESULT D3DXOptimizeFaces(A...)    { return 0; }
template<class... A> HRESULT D3DXLoadSurfaceFromMemory(A...)  { return 0; }
template<class... A> HRESULT D3DXLoadSurfaceFromSurface(A...) { return 0; }
template<class... A> HRESULT D3DXSaveSurfaceToFile(A...) { return 0; }
template<class... A> HRESULT D3DXSaveTextureToFile(A...) { return 0; }
template<class... A> const char* D3DXGetPixelShaderProfile(A...)  { return ""; }
template<class... A> const char* D3DXGetVertexShaderProfile(A...) { return ""; }

// D3DXIMAGE_FILEFORMAT / D3DX filter flags.
#define D3DXIFF_BMP 0
#define D3DXIFF_DDS 4
#define D3DXIFF_PNG 3
#define D3DX_FILTER_POINT    (1 << 0)
#define D3DX_FILTER_TRIANGLE (3 << 0)
