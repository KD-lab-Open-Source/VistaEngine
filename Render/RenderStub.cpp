// Where the renderer is created, and the seam the D3D9 retirement left behind.
//
// CreateIRenderDevice() below is the engine's one entry point into the renderer: it builds
// gb_VisGeneric and the SDL GPU device every platform now draws through. The D3D9 backend it
// replaced is retired -- Render/D3D/*.cpp is compiled nowhere, nothing assigns
// gb_RenderDevice3D, so that pointer is null for the life of the process, on Windows too.
// Every D3D name below resolves through the fake d3d9.h; there is no Direct3D in this build.
//
// The rest of the file is what retirement could not reach. Portable engine code still speaks
// D3D9 nouns -- cTexture::CalcTextureSize asks GetTextureFormatSize(D3DFORMAT),
// GrassMap::BuildGrass packs normals with ColorByNormalRGBA, Camera::SetRenderTarget takes an
// IDirect3DSurface9* -- and it still owns classes whose only implementation was D3D9:
// cOcclusionQuery (cObject3dx's silhouette test, the lens flare), sVideoWrite (GameShell's
// movie recorder), cVertexBufferInternal / cQuadBufferInternal (the dynamic buffer family,
// Documents/Render-PORTING.md #15). The call sites are real and have to link; these bodies are
// what they land on.
//
// Two different kinds of body, and the difference matters. The cD3DRender:: helpers are
// unreachable -- every call goes through the null gb_RenderDevice3D, so nothing runs them.
// The rest DO run, and their return values are consumed: GrassMap::BuildGrass writes
// ColorByNormalRGBA's result into every bush (so each one gets a white normal), cOcclusionQuery
// answers IsVisible() = true, GetTextureFormatSize reports 0 bits per pixel. Those are answers
// the game acts on, not silence. Before treating one as inert, check what reads it.
//
// Only the symbols the linker actually asks for are defined here. When a feature above is
// ported -- or the dead D3D9 reference sources go -- delete what goes with it; the way to find
// out what is still owed is to remove a body and relink.
#include "StdAfxRD.h"
#include "VisGeneric.h"
#include "SDLRenderDevice.h"
#include "D3DRender.h"
#include "src/WinVideo.h"

// ---------------------------------------------------------------------------
// Globals. cD3DRender's constructor used to define and own these; it no longer runs
// anywhere, so gb_RenderDevice3D keeps the null it is initialised with here.
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
// cD3DRender::InitSamplerConstants used to fill them at device init; without this they stay
// zeroed, and a caller asking for sampler_wrap_linear (the selection frame's tiled centre)
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

	// cD3DRender rounded these to linear until SetAnisotropic(n) raised them, and nothing
	// raises them now: cVisGeneric::SetAnisotropic forwards to the null gb_RenderDevice3D.
	sampler_wrap_anisotropic = sampler_wrap_linear;
	sampler_clamp_anisotropic = sampler_clamp_linear;
}

RENDER_API cInterfaceRenderDevice* CreateIRenderDevice(bool multiThread)
{
	// Half 1, unchanged from the original (RenderDevice.cpp): construct the backend-agnostic
	// cVisGeneric so gb_VisGeneric is non-null. Its constructor is pure file-IO/config and
	// touches no GPU device.
	//
	// Half 2 is where the backends part: cSDLRenderDevice, not cD3DRender.
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
// (the shape the D3DRender.cpp bodies had), so the SDL backend owns the GPU
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
// Vertex-format declaration registration. The D3D9 original queued (decl,elements)
// pairs for CreateVertexDeclaration at device init; there is no D3D object to create
// now, so the declaration points at its immortal static element table (from the
// BEGIN_VERTEX_DECLARATION macro) and the SDL backend reads the layout straight off it.
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

// cSkinVertex (ctor / GetWeight / GetDeclaration / declaration[] / Register) is not stubbed:
// VertexDeclaration.cpp is real, portable, and in the build.

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
