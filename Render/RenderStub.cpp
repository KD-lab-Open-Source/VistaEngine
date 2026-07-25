// Render-backend stub for the cross-platform build.
//
// The real renderer is the D3D9 backend (Render/D3D/*.cpp), which drives
// IDirect3DDevice9 / IDirect3D9 directly and is therefore Windows-only (gated in
// Render/CMakeLists.txt). Off-Windows the fake d3d9.h only makes the render
// headers parse, not the device-creation call sites, so we provide no-op
// definitions for the render-backend symbols the rest of the engine references.
// The real cross-platform renderer is the SDL GPU backend (Track B); until then
// the game links and runs with a silent renderer.
//
// Only the symbols actually referenced by the compiled (non-WIN32) sources are
// stubbed here — see the undefined-symbol set at the final Game link.
#include "StdAfxRD.h"
#include "VisGeneric.h"
#include "SDLRenderDevice.h"
#include "D3DRender.h"
#include "src/WinVideo.h"

// ---------------------------------------------------------------------------
// Globals (defined in D3DRender.cpp / RenderDevice.cpp on Windows).
// ---------------------------------------------------------------------------
RENDER_API cInterfaceRenderDevice* gb_RenderDevice = 0;
RENDER_API cD3DRender*             gb_RenderDevice3D = 0;

RENDER_API SAMPLER_DATA sampler_wrap_point;
RENDER_API SAMPLER_DATA sampler_wrap_linear;
RENDER_API SAMPLER_DATA sampler_wrap_anisotropic;
RENDER_API SAMPLER_DATA sampler_clamp_point;
RENDER_API SAMPLER_DATA sampler_clamp_linear;
RENDER_API SAMPLER_DATA sampler_clamp_anisotropic;

// The six samplers the engine names when it calls SetSamplerData/SetSamplerDataVirtual.
// cD3DRender::InitSamplerConstants fills them on Windows; without this they stay zeroed
// off it, and a caller asking for sampler_wrap_linear (the selection frame's tiled centre)
// would be asking for address mode 0, which is not a mode at all.
static void initSamplerConstants()
{
	memset(&sampler_clamp_linear, 0, sizeof(SAMPLER_DATA));
	sampler_clamp_linear.addressu = DX_TADDRESS_CLAMP;
	sampler_clamp_linear.addressv = DX_TADDRESS_CLAMP;
	sampler_clamp_linear.addressw = DX_TADDRESS_CLAMP;
	sampler_clamp_linear.minfilter = DX_TEXF_LINEAR;
	sampler_clamp_linear.magfilter = DX_TEXF_LINEAR;
	sampler_clamp_linear.mipfilter = DX_TEXF_LINEAR;
	sampler_clamp_linear.bordercolor = 0;

	sampler_wrap_linear = sampler_clamp_linear;
	sampler_wrap_linear.addressu = DX_TADDRESS_WRAP;
	sampler_wrap_linear.addressv = DX_TADDRESS_WRAP;
	sampler_wrap_linear.addressw = DX_TADDRESS_WRAP;

	sampler_wrap_point = sampler_wrap_linear;
	sampler_wrap_point.minfilter = DX_TEXF_POINT;
	sampler_wrap_point.magfilter = DX_TEXF_POINT;
	sampler_wrap_point.mipfilter = DX_TEXF_POINT;

	sampler_clamp_point = sampler_wrap_point;
	sampler_clamp_point.addressu = DX_TADDRESS_CLAMP;
	sampler_clamp_point.addressv = DX_TADDRESS_CLAMP;
	sampler_clamp_point.addressw = DX_TADDRESS_CLAMP;

	// cD3DRender rounds these to linear until SetAnisotropic(n) raises them, and nothing
	// off-Windows raises them yet.
	sampler_wrap_anisotropic = sampler_wrap_linear;
	sampler_clamp_anisotropic = sampler_clamp_linear;
}

RENDER_API cInterfaceRenderDevice* CreateIRenderDevice(bool multiThread)
{
	// Half 1 of the real CreateIRenderDevice (RenderDevice.cpp): construct the
	// backend-agnostic cVisGeneric so gb_VisGeneric is non-null. Its constructor
	// is pure file-IO/config (no GPU device), and VisGeneric.cpp is compiled into
	// this library off-Windows, so this links and runs today.
	//
	// Half 2: the cross-platform render device is the SDL GPU backend
	// (cSDLRenderDevice), replacing the Windows-only cD3DRender.
	initSamplerConstants();
	gb_VisGeneric = new cVisGeneric(multiThread);
	return gb_RenderDevice = new cSDLRenderDevice;
}

// ---------------------------------------------------------------------------
// ManagedResource (key function ~ManagedResource emits its vtable + typeinfo;
// referenced by cScene in the compiled Scene.cpp).
// ---------------------------------------------------------------------------
ManagedResource::ManagedResource() {}
ManagedResource::~ManagedResource() {}

// ---------------------------------------------------------------------------
// sPtrVertexBuffer / sPtrIndexBuffer — lifetime routes through the interface
// (mirrors the Windows D3DRender.cpp bodies), so the SDL backend owns the GPU
// buffers behind the slot and releases them on Destroy/dtor.
//
// The gb_RenderDevice guard matters for handles with *static storage duration*:
// Runtime::done() releases the device with RELEASE(gb_RenderDevice), which nulls
// the global, so by the time __cxa_finalize runs these dtors at exit the device is
// already gone. cSDLRenderDevice::
// Done() has by then released every GPU buffer and cleared vbGpu_/ibGpu_, so there
// is nothing left to route -- skipping leaks only the tiny sSlot heap node, which
// the process exit reclaims anyway. Without the guard the null-device virtual call
// crashes in this dtor.
// ---------------------------------------------------------------------------
void sPtrVertexBuffer::Destroy()
{
	if(ptr && gb_RenderDevice)
		gb_RenderDevice->DeleteVertexBuffer(*this);
	ptr = 0;
}
void sPtrVertexBuffer::CopyAddRef(const sPtrVertexBuffer& from)
{
	xassert(ptr == 0);
	ptr = from.ptr;
	if(ptr) ptr->init++;
}

sPtrIndexBuffer::~sPtrIndexBuffer()
{
	if(ptr && gb_RenderDevice)
		gb_RenderDevice->DeleteIndexBuffer(*this);
	ptr = 0;
}
void sPtrIndexBuffer::CopyAddRef(const sPtrIndexBuffer& from)
{
	xassert(ptr == 0);
	ptr = from.ptr;
	if(ptr) ptr->init++;
}

// ---------------------------------------------------------------------------
// Vertex-format declaration registration. On Windows this queues (decl,elements)
// pairs for CreateVertexDeclaration at device init; off-Windows there is no D3D
// object, so we point the declaration at its immortal static element table (from
// the BEGIN_VERTEX_DECLARATION macro) so the SDL backend can read the layout.
// The vertex::declaration statics themselves are defined by VertexDeclaration.cpp.
// ---------------------------------------------------------------------------
void cD3DRender::RegisterVertexDeclaration(LPDIRECT3DVERTEXDECLARATION9& declaration,
                                           D3DVERTEXELEMENT9* elements)
{
	IDirect3DVertexDeclaration9* d = new IDirect3DVertexDeclaration9();
	d->elements = elements;
	unsigned int n = 0;
	while(elements[n].Stream != 0xFF) ++n;   // count up to the D3DDECL_END() terminator
	d->elementCount = n;
	declaration = d;
}

// ---------------------------------------------------------------------------
// DrawStrip has no implementation, deliberately. Its Set() is an inline in
// Render/D3D/VertexBuffer.h that writes straight into a locked cVertexBuffer, and there is
// no such buffer here: a body would leave `buf` null and `pointer` uninitialised, so the
// first Set() would write through a garbage pointer. Every caller takes
// SDLWorldQuadRenderer's triangle route instead (cUnkLight::Draw,
// CircleManager::Layer::drawSpline, Lighting::OneLight::Draw), and a new one now fails at
// link time rather than corrupting the heap at run time.
//
// PoolManager / Pool / VertexPool / IndexPool (the D3D9 tilemap's page allocator), DrawType
// and cTileMapRender were stubbed here for the same reason and are not stubbed any more:
// nothing left in the build references them. They live on in Render/D3D/, which is compiled
// nowhere.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Render free functions (D3D/*.cpp): logging, format/size queries, debug stats.
// ---------------------------------------------------------------------------
unsigned int ColorByNormalRGBA(Vect3f n);   // declared inline near its callers
unsigned int ColorByNormalRGBA(Vect3f /*n*/) { return 0xffffffff; }
int   RDWriteLog(HRESULT /*err*/, const char* /*exp*/, const char* /*file*/, int /*line*/) { return 0; }
int   GetTextureFormatSize(D3DFORMAT /*f*/) { return 0; }
Vect2i GetSize(IDirect3DSurface9* /*pTexture*/) { return Vect2i(0, 0); }
void  ShowGraphicsStatistic() {}

// ---------------------------------------------------------------------------
// sVideoWrite (src/WinVideo.cpp): Win32 Video-for-Windows AVI writer.
// ---------------------------------------------------------------------------
sVideoWrite::sVideoWrite(bool use_rgb_format_)
	: use_rgb_format(use_rgb_format_), stream_is_open(false),
	  pf(0), psSmall(0), bitmapsize(0) {}
sVideoWrite::~sVideoWrite() {}
bool sVideoWrite::Open(const char* /*file_name*/, int /*sizex*/, int /*sizey*/, int /*frame_rate*/) { return false; }
void sVideoWrite::Close() {}
bool sVideoWrite::WriteFrame() { return false; }

// ---------------------------------------------------------------------------
// cVertexBufferInternal / cQuadBufferInternal
// ---------------------------------------------------------------------------
BYTE* cVertexBufferInternal::Lock(int /*minvertex*/) { return 0; }
void  cVertexBufferInternal::Unlock(int /*num_write_vertex*/) {}
void  cVertexBufferInternal::DrawPrimitive(PRIMITIVETYPE /*Type*/, UINT /*Count*/) {}

void  cQuadBufferInternal::BeginDraw() {}
void  cQuadBufferInternal::EndDraw() {}
void* cQuadBufferInternal::Get() { return 0; }
void  cQuadBufferInternal::SetMatrix(const MatXf& /*m*/) {}

// cSkinVertex (ctor / GetWeight / GetDeclaration / declaration[] / Register) is
// defined by VertexDeclaration.cpp, which is now compiled off-Windows too.

// ---------------------------------------------------------------------------
// cOcclusionQuery
// ---------------------------------------------------------------------------
cOcclusionQuery::cOcclusionQuery() : pQuery(0), draw(false), testedCount_(0) {}
cOcclusionQuery::~cOcclusionQuery() {}
bool cOcclusionQuery::Init() { return false; }
void cOcclusionQuery::Done() {}
void cOcclusionQuery::Test(const Vect3f& /*pos*/) {}
void cOcclusionQuery::Test(const Vect3f* /*point*/, int /*numPoints*/) {}
int  cOcclusionQuery::VisibleCount() { return 0; }
bool cOcclusionQuery::IsVisible() { return true; }
void cOcclusionQuery::Begin() {}
void cOcclusionQuery::End() {}

// ---------------------------------------------------------------------------
// cD3DRender — the helper (non-virtual) methods still named by compiled code.
// Every call reaches them through the permanently null gb_RenderDevice3D, so none of
// these bodies ever runs; they exist so those call sites link. Only the methods the
// linker actually asks for are here — the rest of cD3DRender's helpers went with the
// pools above.
// ---------------------------------------------------------------------------
Mat4f cD3DRender::shadowMatBias() const { return Mat4f(); }
void  cD3DRender::SetAnisotropic(int /*level*/) {}
int   cD3DRender::GetAnisotropic() { return 0; }
int   cD3DRender::GetMaxAnisotropicLevels() { return 0; }
void  cD3DRender::SetBlendStateAlphaRef(eBlendMode /*blend*/) {}
void  cD3DRender::SetRenderTarget(cTexture* /*target*/, IDirect3DSurface9* /*pZBuffer*/) {}
void  cD3DRender::RestoreRenderTarget() {}
void  cD3DRender::FlushPrimitive3DWorld() {}
bool  cD3DRender::ReinitOcclusion() { return false; }
bool  cD3DRender::PossibilityOcclusion() { return false; }
void  cD3DRender::RestoreShader() {}
void  cD3DRender::BuildNormalMap(cTexture* /*Texture*/, Vect3f* /*normals*/) {}
void  cD3DRender::SetSamplerDataReal(DWORD /*stage*/, SAMPLER_DATA& /*data*/) {}
