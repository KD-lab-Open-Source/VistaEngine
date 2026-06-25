#pragma once
// Direct3D 9 — minimal stub for non-Windows builds.
// All D3D source files are excluded from non-Windows CMake targets.
// This stub only exists to satisfy includes in headers that aren't yet guarded.
#include "../WindowsAPI.h"

typedef struct IDirect3D9             IDirect3D9;
typedef struct IDirect3DDevice9       IDirect3DDevice9;
typedef struct IDirect3DTexture9      IDirect3DTexture9;
typedef struct IDirect3DSurface9      IDirect3DSurface9;
typedef struct IDirect3DVertexBuffer9 IDirect3DVertexBuffer9;
typedef struct IDirect3DIndexBuffer9  IDirect3DIndexBuffer9;
typedef struct IDirect3DVertexDeclaration9 IDirect3DVertexDeclaration9;
typedef struct IDirect3DVertexShader9 IDirect3DVertexShader9;
typedef struct IDirect3DPixelShader9  IDirect3DPixelShader9;
typedef struct IDirect3DCubeTexture9  IDirect3DCubeTexture9;
typedef struct IDirect3DStateBlock9   IDirect3DStateBlock9;
typedef struct IDirect3DQuery9        IDirect3DQuery9;

typedef IDirect3D9*              LPDIRECT3D9;
typedef IDirect3DDevice9*        LPDIRECT3DDEVICE9;
typedef IDirect3DTexture9*       LPDIRECT3DTEXTURE9;
typedef IDirect3DSurface9*       LPDIRECT3DSURFACE9;
typedef IDirect3DVertexBuffer9*  LPDIRECT3DVERTEXBUFFER9;
typedef IDirect3DIndexBuffer9*   LPDIRECT3DINDEXBUFFER9;
typedef IDirect3DVertexDeclaration9* LPDIRECT3DVERTEXDECLARATION9;
typedef IDirect3DVertexShader9*  LPDIRECT3DVERTEXSHADER9;
typedef IDirect3DPixelShader9*   LPDIRECT3DPIXELSHADER9;
typedef IDirect3DCubeTexture9*   LPDIRECT3DCUBETEXTURE9;
typedef IDirect3DQuery9*         LPDIRECT3DQUERY9;

typedef DWORD D3DCOLOR;
typedef DWORD D3DFORMAT;
typedef DWORD D3DPOOL;
typedef DWORD D3DRESOURCETYPE;
typedef DWORD D3DPRIMITIVETYPE;
typedef DWORD D3DCULL;

struct D3DVERTEXELEMENT9 { WORD Stream, Offset; BYTE Type, Method, Usage, UsageIndex; };
struct D3DDISPLAYMODE    { UINT Width, Height, RefreshRate; D3DFORMAT Format; };
struct D3DVIEWPORT9      { DWORD X, Y, Width, Height; float MinZ, MaxZ; };
struct D3DXMATRIX        { float m[4][4]; };
struct D3DRECT           { LONG x1, y1, x2, y2; };
struct D3DBOX            { UINT Left, Top, Right, Bottom, Front, Back; };
struct D3DLOCKED_RECT    { INT Pitch; void* pBits; };
struct D3DLOCKED_BOX     { INT RowPitch, SlicePitch; void* pBits; };
struct D3DSURFACE_DESC   { D3DFORMAT Format; D3DRESOURCETYPE Type; DWORD Usage; D3DPOOL Pool;
                           UINT Width, Height; };
struct D3DCAPS9          { DWORD DevCaps; DWORD MaxTextureWidth, MaxTextureHeight; };

#define D3DDECL_END() { 0xFF, 0, 0, 0, 0, 0 }
#define D3DCLEAR_TARGET  0x1
#define D3DCLEAR_ZBUFFER 0x2
#define D3DPT_TRIANGLELIST 4
#define D3DPT_TRIANGLESTRIP 5
