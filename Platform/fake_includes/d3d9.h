#pragma once
// Direct3D 9 — stub for non-Windows builds.
//
// The D3D9 renderer .cpp are excluded from the cross-platform CMake targets and
// will be replaced by the SDL GPU backend (Track B). This stub exists so the
// render *headers* (Render/D3D/D3DRender.h, Render/shader/Shaders.h, ...) PARSE
// for the upper layers that include them (UserInterface, Game, ...). Those
// layers compile against these declarations; the actual render symbols are
// provided by the eventual Render stub LIB at link time.
//
// Only what the inline code in the render headers touches is modelled here.
#include "../WindowsAPI.h"

// ─── COM-ish base ─────────────────────────────────────────────────────────────
// Every D3D9 interface inherits IUnknown's Release/AddRef/QueryInterface; model
// them as variadic-template no-ops so inline calls (and the RELEASE macro)
// compile. Per-interface methods used by inline code are added on the concrete
// types below.
struct IDirect3DUnknown9 {
    template<class... A> ULONG   Release(A...)        { return 0; }
    template<class... A> ULONG   AddRef(A...)         { return 0; }
    template<class... A> HRESULT QueryInterface(A...) { return 0; }
};

// ─── Texture interfaces ──────────────────────────────────────────────────────
// As in real D3D9, the concrete texture types derive from a common base so a
// IDirect3DTexture9*/CubeTexture9* binds to a IDirect3DBaseTexture9* parameter.
struct IDirect3DBaseTexture9   : IDirect3DUnknown9 {};
struct IDirect3DTexture9       : IDirect3DBaseTexture9 {
    template<class... A> HRESULT GetSurfaceLevel(A...) { return 0; }
    template<class... A> HRESULT LockRect(A...)        { return 0; }
    template<class... A> HRESULT UnlockRect(A...)      { return 0; }
    template<class... A> HRESULT GetLevelDesc(A...)    { return 0; }
    template<class... A> DWORD   GetLevelCount(A...)   { return 0; }
    template<class... A> HRESULT GenerateMipSubLevels(A...) { return 0; }
};
struct IDirect3DCubeTexture9   : IDirect3DBaseTexture9 {
    template<class... A> HRESULT GetCubeMapSurface(A...) { return 0; }
    template<class... A> HRESULT GetLevelDesc(A...)      { return 0; }
};
struct IDirect3DVolumeTexture9 : IDirect3DBaseTexture9 {
    template<class... A> HRESULT LockBox(A...)             { return 0; }
    template<class... A> HRESULT UnlockBox(A...)           { return 0; }
    template<class... A> HRESULT GenerateMipSubLevels(A...){ return 0; }
};

struct IDirect3DSurface9 : IDirect3DUnknown9 {
    template<class... A> HRESULT LockRect(A...)   { return 0; }
    template<class... A> HRESULT UnlockRect(A...) { return 0; }
    template<class... A> HRESULT GetDesc(A...)    { return 0; }
};
struct IDirect3DVertexBuffer9 : IDirect3DUnknown9 {
    template<class... A> HRESULT Lock(A...)   { return 0; }
    template<class... A> HRESULT Unlock(A...) { return 0; }
};
struct IDirect3DIndexBuffer9 : IDirect3DUnknown9 {
    template<class... A> HRESULT Lock(A...)   { return 0; }
    template<class... A> HRESULT Unlock(A...) { return 0; }
};
struct IDirect3DVertexDeclaration9  : IDirect3DUnknown9 {};
struct IDirect3DVertexShader9       : IDirect3DUnknown9 {};
struct IDirect3DPixelShader9        : IDirect3DUnknown9 {};
struct IDirect3DStateBlock9         : IDirect3DUnknown9 {};
struct IDirect3DQuery9              : IDirect3DUnknown9 {
    template<class... A> HRESULT Issue(A...)   { return 0; }
    template<class... A> HRESULT GetData(A...) { return 0; }
};
struct IDirect3D9                   : IDirect3DUnknown9 {};

// ─── Device ──────────────────────────────────────────────────────────────────
// The render .cpp call these directly; declared as variadic templates returning
// HRESULT so any argument list compiles (the real implementations are the D3D
// backend, excluded here / future SDL GPU backend). The method list is the full
// set of IDirect3DDevice9 members called across the engine sources.
struct IDirect3DDevice9 : IDirect3DUnknown9 {
    template<class... A> HRESULT BeginScene(A...)               { return 0; }
    template<class... A> HRESULT Clear(A...)                    { return 0; }
    template<class... A> HRESULT CreateCubeTexture(A...)        { return 0; }
    template<class... A> HRESULT CreateDepthStencilSurface(A...){ return 0; }
    template<class... A> HRESULT CreateIndexBuffer(A...)        { return 0; }
    template<class... A> HRESULT CreateOffscreenPlainSurface(A...){ return 0; }
    template<class... A> HRESULT CreatePixelShader(A...)        { return 0; }
    template<class... A> HRESULT CreateQuery(A...)              { return 0; }
    template<class... A> HRESULT CreateRenderTarget(A...)       { return 0; }
    template<class... A> HRESULT CreateTexture(A...)            { return 0; }
    template<class... A> HRESULT CreateVertexBuffer(A...)       { return 0; }
    template<class... A> HRESULT CreateVertexDeclaration(A...)  { return 0; }
    template<class... A> HRESULT CreateVertexShader(A...)       { return 0; }
    template<class... A> HRESULT CreateVolumeTexture(A...)      { return 0; }
    template<class... A> HRESULT DrawIndexedPrimitive(A...)     { return 0; }
    template<class... A> HRESULT DrawPrimitive(A...)            { return 0; }
    template<class... A> HRESULT DrawPrimitiveUP(A...)          { return 0; }
    template<class... A> HRESULT EndScene(A...)                 { return 0; }
    template<class... A> HRESULT EvictManagedResources(A...)    { return 0; }
    template<class... A> UINT    GetAvailableTextureMem(A...)   { return 0; }
    template<class... A> HRESULT GetDepthStencilSurface(A...)   { return 0; }
    template<class... A> HRESULT GetDeviceCaps(A...)            { return 0; }
    template<class... A> HRESULT GetDisplayMode(A...)           { return 0; }
    template<class... A> DWORD   GetRenderState(A...)           { return 0; }
    template<class... A> HRESULT GetRenderTarget(A...)          { return 0; }
    template<class... A> HRESULT GetRenderTargetData(A...)      { return 0; }
    template<class... A> HRESULT GetFrontBufferData(A...)       { return 0; }
    template<class... A> HRESULT GetSamplerState(A...)          { return 0; }
    template<class... A> HRESULT GetTextureStageState(A...)     { return 0; }
    template<class... A> HRESULT GetViewport(A...)              { return 0; }
    template<class... A> HRESULT LightEnable(A...)              { return 0; }
    template<class... A> HRESULT Present(A...)                  { return 0; }
    template<class... A> HRESULT Reset(A...)                    { return 0; }
    template<class... A> HRESULT SetDepthStencilSurface(A...)   { return 0; }
    template<class... A> HRESULT SetGammaRamp(A...)             { return 0; }
    template<class... A> HRESULT SetIndices(A...)               { return 0; }
    template<class... A> HRESULT SetLight(A...)                 { return 0; }
    template<class... A> HRESULT SetPixelShader(A...)           { return 0; }
    template<class... A> HRESULT SetPixelShaderConstantF(A...)  { return 0; }
    template<class... A> HRESULT SetRenderState(A...)           { return 0; }
    template<class... A> HRESULT SetRenderTarget(A...)          { return 0; }
    template<class... A> HRESULT SetSamplerState(A...)          { return 0; }
    template<class... A> HRESULT SetStreamSource(A...)          { return 0; }
    template<class... A> HRESULT SetTexture(A...)               { return 0; }
    template<class... A> HRESULT SetTextureStageState(A...)     { return 0; }
    template<class... A> HRESULT SetTransform(A...)             { return 0; }
    template<class... A> HRESULT SetVertexDeclaration(A...)     { return 0; }
    template<class... A> HRESULT SetVertexShader(A...)          { return 0; }
    template<class... A> HRESULT SetVertexShaderConstantF(A...) { return 0; }
    template<class... A> HRESULT SetViewport(A...)              { return 0; }
    template<class... A> HRESULT StretchRect(A...)              { return 0; }
    template<class... A> HRESULT TestCooperativeLevel(A...)     { return 0; }
};

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
typedef DWORD D3DMULTISAMPLE_TYPE;
typedef DWORD D3DRENDERSTATETYPE;
typedef DWORD D3DTEXTURESTAGESTATETYPE;
typedef DWORD D3DSAMPLERSTATETYPE;
typedef DWORD D3DTRANSFORMSTATETYPE;

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
struct D3DCAPS9          { DWORD DevCaps; DWORD MaxTextureWidth, MaxTextureHeight;
                           DWORD VertexShaderVersion, PixelShaderVersion;
                           DWORD TextureOpCaps; };
typedef DWORD D3DCUBEMAP_FACES;
struct D3DPRESENT_PARAMETERS { UINT BackBufferWidth, BackBufferHeight;
                               D3DFORMAT BackBufferFormat; UINT BackBufferCount;
                               BOOL Windowed; D3DFORMAT AutoDepthStencilFormat; };

// Shader version encodings (vs_M_m / ps_M_m), as in d3d9types.h.
#define D3DVS_VERSION(major,minor) (0xFFFE0000 | ((major) << 8) | (minor))
#define D3DPS_VERSION(major,minor) (0xFFFF0000 | ((major) << 8) | (minor))

#define D3DDECL_END() { 0xFF, 0, 0, 0, 0, 0 }
#define D3DCLEAR_TARGET  0x1
#define D3DCLEAR_ZBUFFER 0x2
#define D3DPT_TRIANGLELIST 4
#define D3DPT_TRIANGLESTRIP 5

// ─── Enumerated constants (subset used by inline render code) ─────────────────
#define D3DX_DEFAULT     ((UINT)-1)
#define D3DPOOL_MANAGED  1

// D3DFORMAT values (real d3d9types.h numbering; FourCC for the DXT compressed
// formats). The engine assigns/compares these, so the actual values matter.
#ifndef MAKEFOURCC
#define MAKEFOURCC(a,b,c,d) ((DWORD)(BYTE)(a)|((DWORD)(BYTE)(b)<<8)|((DWORD)(BYTE)(c)<<16)|((DWORD)(BYTE)(d)<<24))
#endif
#define D3DFMT_UNKNOWN        0
#define D3DFMT_R8G8B8         20
#define D3DFMT_A8R8G8B8       21
#define D3DFMT_X8R8G8B8       22
#define D3DFMT_R5G6B5         23
#define D3DFMT_X1R5G5B5       24
#define D3DFMT_A1R5G5B5       25
#define D3DFMT_A4R4G4B4       26
#define D3DFMT_R3G3B2         27
#define D3DFMT_A8             28
#define D3DFMT_A8R3G3B2       29
#define D3DFMT_X4R4G4B4       30
#define D3DFMT_A2B10G10R10    31
#define D3DFMT_A8B8G8R8       32
#define D3DFMT_X8B8G8R8       33
#define D3DFMT_G16R16         34
#define D3DFMT_A2R10G10B10    35
#define D3DFMT_A16B16G16R16   36
#define D3DFMT_A8P8           40
#define D3DFMT_P8             41
#define D3DFMT_L8             50
#define D3DFMT_A8L8           51
#define D3DFMT_A4L4           52
#define D3DFMT_V8U8           60
#define D3DFMT_L6V5U5         61
#define D3DFMT_X8L8V8U8       62
#define D3DFMT_Q8W8V8U8       63
#define D3DFMT_V16U16         64
#define D3DFMT_A2W10V10U10    67
#define D3DFMT_D16_LOCKABLE   70
#define D3DFMT_D32            71
#define D3DFMT_D15S1          73
#define D3DFMT_D24S8          75
#define D3DFMT_D24X8          77
#define D3DFMT_D24X4S4        79
#define D3DFMT_D16            80
#define D3DFMT_L16            81
#define D3DFMT_D32F_LOCKABLE  82
#define D3DFMT_D24FS8         83
#define D3DFMT_CxV8U8         117
#define D3DFMT_INDEX16        101
#define D3DFMT_INDEX32        102
#define D3DFMT_Q16W16V16U16   110
#define D3DFMT_R16F           111
#define D3DFMT_G16R16F        112
#define D3DFMT_A16B16G16R16F  113
#define D3DFMT_R32F           114
#define D3DFMT_G32R32F        115
#define D3DFMT_A32B32G32R32F  116
#define D3DFMT_DXT1           MAKEFOURCC('D','X','T','1')
#define D3DFMT_DXT2           MAKEFOURCC('D','X','T','2')
#define D3DFMT_DXT3           MAKEFOURCC('D','X','T','3')
#define D3DFMT_DXT4           MAKEFOURCC('D','X','T','4')
#define D3DFMT_DXT5           MAKEFOURCC('D','X','T','5')

// D3DRENDERSTATETYPE values (real d3d9types.h values).
#define D3DRS_ZWRITEENABLE      14
#define D3DRS_CULLMODE          22
#define D3DRS_ZFUNC             23
#define D3DRS_ALPHAREF          24
#define D3DRS_COLORWRITEENABLE  168

// D3DCMPFUNC / D3DTEXTUREFILTERTYPE.
#define D3DCMP_EQUAL        3
#define D3DCMP_LESSEQUAL    4
#define D3DCMP_GREATER      5
#define D3DCMP_GREATEREQUAL 7
#define D3DCMP_ALWAYS       8
#define D3DSTENCILOP_KEEP   1
#define D3DSTENCILOP_INCR   7
#define D3DRS_STENCILFAIL   53
#define D3DRS_STENCILZFAIL  54
#define D3DTEXF_LINEAR   2

// D3DRENDERSTATETYPE values (real d3d9types.h).
#define D3DRS_ZENABLE             7
#define D3DRS_ALPHATESTENABLE     15
#define D3DRS_SRCBLEND            19
#define D3DRS_DESTBLEND           20
#define D3DRS_ALPHAFUNC          25
#define D3DRS_ALPHABLENDENABLE    27
#define D3DRS_FOGENABLE          28
#define D3DRS_STENCILENABLE      52
#define D3DRS_BLENDOP           171
#define D3DRS_SLOPESCALEDEPTHBIAS 175

// D3DBLEND / D3DBLENDOP.
#define D3DBLEND_ONE          2
#define D3DBLEND_SRCALPHA     5
#define D3DBLEND_INVSRCALPHA  6
#define D3DBLEND_DESTALPHA    7
#define D3DBLEND_INVDESTALPHA 8
#define D3DBLENDOP_ADD     1

// Fog render states / stencil ops / lock flags.
#define D3DRS_FOGCOLOR        34
#define D3DRS_FOGTABLEMODE    35
#define D3DRS_FOGSTART        36
#define D3DRS_FOGEND          37
#define D3DRS_FOGVERTEXMODE  140
#define D3DRS_STENCILPASS     55
#define D3DRS_STENCILFUNC     56
#define D3DRS_STENCILREF      57
#define D3DSTENCILOP_REPLACE   3
#define D3DLOCK_READONLY    0x10
#define D3DTS_TEXTURE0       16
#define D3DTS_TEXTURE1       17
#define D3DTTFF_DISABLE       0
#define D3DTTFF_COUNT2        2

// D3DCULL / D3DPOOL / D3DMULTISAMPLE_TYPE.
#define D3DCULL_NONE       1
#define D3DCULL_CCW        3
#define D3DPOOL_SYSTEMMEM  2
#define D3DMULTISAMPLE_NONE 0

// D3DTEXTURESTAGESTATETYPE (D3DTSS_) and texture-argument flags.
#define D3DTSS_COLOROP        1
#define D3DTSS_COLORARG1      2
#define D3DTSS_COLORARG2      3
#define D3DTSS_ALPHAOP        4
#define D3DTSS_ALPHAARG1      5
#define D3DTSS_ALPHAARG2      6
#define D3DTSS_BUMPENVMAT00   7
#define D3DTSS_BUMPENVMAT01   8
#define D3DTSS_BUMPENVMAT10   9
#define D3DTSS_BUMPENVMAT11   10
#define D3DTSS_TEXCOORDINDEX  11
#define D3DTSS_TEXTURETRANSFORMFLAGS 24
#define D3DTA_TEXTURE  2
#define D3DTA_CURRENT  1
#define D3DTA_DIFFUSE  0
#define D3DTOP_DISABLE    1
#define D3DTOP_SELECTARG1 2
#define D3DTOP_SELECTARG2 3
#define D3DTOP_BUMPENVMAP 22
#define D3DTEXOPCAPS_BUMPENVMAP 0x00200000

// D3DCUBEMAP_FACES enumerators.
#define D3DCUBEMAP_FACE_POSITIVE_X 0
#define D3DCUBEMAP_FACE_NEGATIVE_X 1
#define D3DCUBEMAP_FACE_POSITIVE_Y 2
#define D3DCUBEMAP_FACE_NEGATIVE_Y 3
#define D3DCUBEMAP_FACE_POSITIVE_Z 4
#define D3DCUBEMAP_FACE_NEGATIVE_Z 5

// Color write mask bits.
#define D3DCOLORWRITEENABLE_RED   (1L<<0)
#define D3DCOLORWRITEENABLE_GREEN (1L<<1)
#define D3DCOLORWRITEENABLE_BLUE  (1L<<2)
#define D3DCOLORWRITEENABLE_ALPHA (1L<<3)
