#include "StdAfxRD.h"
#include "cZPlane.h"
#include "Texture.h"
#include "cCamera.h"
#include "D3DRender.h"
#include "Render/SDLWorldQuadRenderer.h"   // the plane is drawn by SDLWorldQuadRenderer,
#include "Render/SDLRenderDevice.h"        // reached via cSDLRenderDevice::drawWorldQuads

cPlane::cPlane()
:cUnkObj(KIND_NULL)
{
	umin=vmin=0;
	umax=vmax=1;
}

void cPlane::PreDraw(Camera* camera)
{
	if(getAttribute(ATTRUNKOBJ_IGNORE)==0)
	{
		if(GetTexture() && GetTexture()->isAlpha())
			camera->Attach(SCENENODE_OBJECTSORT,this);
		else
			camera->Attach(SCENENODE_OBJECT,this);
	}
}

void cPlane::Draw(Camera* camera)
{
#ifdef _WIN32
	cD3DRender* rd=gb_RenderDevice3D;
	cVertexBuffer<sVertexXYZDT1>* buf=gb_RenderDevice->GetBufferXYZDT1();

	sVertexXYZDT1* vertex=buf->Lock(4);
#else
	// The device hands out no vertex buffer off Windows -- SDL GPU draws only inside a render
	// pass, which is a renderer's business -- and this is a world-space textured quad, so it
	// belongs to the world-quad renderer, which answers to the quad buffer's BeginDraw/Get/
	// EndDraw. Get's four vertices are the same two opposite edges D3D's strip pairs, so its
	// two triangles cover the same quad.
	//
	// SetMaterial stands in for the SetNoMaterial below (the texture and the object's world
	// matrix) and for D3DRS_ZENABLE FALSE, the depth test off. The clamp_point sampler does
	// not survive: the renderer keeps one linear sampler, so the footprint's texels are
	// filtered rather than blocky.
	SDLWorldQuadRenderer* buf = sdlWorldQuadRenderer();
	cSDLRenderDevice* dev = sdlRenderDevice();
	if(!buf || !dev)
		return;
	buf->SetCamera(camera);
	buf->SetMaterial(ALPHA_NONE, GetTexture(), false, GetGlobalMatrix());
	buf->BeginDraw();
	sVertexXYZDT1* vertex = buf->Get();
#endif
	vertex[0].pos.set(0,0,0);
	vertex[0].diffuse.RGBA()=0xFFFFFFFF;
	vertex[0].uv[0]=umin;
	vertex[0].uv[1]=vmin;

	vertex[1].pos.set(0,size.y,0);
	vertex[1].diffuse.RGBA()=0xFFFFFFFF;
	vertex[1].uv[0]=umin;
	vertex[1].uv[1]=vmax;

	vertex[2].pos.set(size.x,0,0);
	vertex[2].diffuse.RGBA()=0xFFFFFFFF;
	vertex[2].uv[0]=umax;
	vertex[2].uv[1]=vmin;

	vertex[3].pos.set(size.x,size.y,0);
	vertex[3].diffuse.RGBA()=0xFFFFFFFF;
	vertex[3].uv[0]=umax;
	vertex[3].uv[1]=vmax;

#ifdef _WIN32
	buf->Unlock(4);

	rd->SetSamplerData(0,sampler_clamp_point);
	rd->SetNoMaterial(ALPHA_NONE,GetGlobalMatrix(),0,GetTexture());

	int zEnable = rd->GetRenderState(D3DRS_ZENABLE);
	rd->SetRenderState(D3DRS_ZENABLE, FALSE);

	buf->DrawPrimitive(PT_TRIANGLESTRIP,2);

	rd->SetRenderState(D3DRS_ZENABLE, zEnable);
#else
	buf->EndDraw();
	dev->drawWorldQuads();
#endif
}

void cPlane::SetUV(float _umin,float _vmin,float _umax,float _vmax)
{
	umin=_umin;
	vmin=_vmin;
	umax=_umax;
	vmax=_vmax;
}
