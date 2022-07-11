#include "StdAfxRD.h"
#include "cZPlane.h"
#include "Texture.h"
#include "cCamera.h"
#include "D3DRender.h"

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
	cD3DRender* rd=gb_RenderDevice3D;
	cVertexBuffer<sVertexXYZDT1>* buf=gb_RenderDevice->GetBufferXYZDT1();

	sVertexXYZDT1* vertex=buf->Lock(4);
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
	buf->Unlock(4);

	rd->SetSamplerData(0,sampler_clamp_point);
	rd->SetNoMaterial(ALPHA_NONE,GetGlobalMatrix(),0,GetTexture());
	
	int zEnable = rd->GetRenderState(D3DRS_ZENABLE);
	rd->SetRenderState(D3DRS_ZENABLE, FALSE);

	buf->DrawPrimitive(PT_TRIANGLESTRIP,2);

	rd->SetRenderState(D3DRS_ZENABLE, zEnable);
}

void cPlane::SetUV(float _umin,float _vmin,float _umax,float _vmax)
{
	umin=_umin;
	vmin=_vmin;
	umax=_umax;
	vmax=_vmax;
}
