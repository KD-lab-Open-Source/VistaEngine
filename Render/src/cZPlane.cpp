#include "StdAfxRD.h"
#include "cZPlane.h"
cPlane::cPlane()
:cUnkObj(KIND_NULL)
{
	umin=vmin=0;
	umax=vmax=1;
}

Vect3f cPlane::GetCenterObject()
{
	return GetGlobalMatrix().trans()+size*0.5f;
}

void cPlane::PreDraw(cCamera *DrawNode)
{
	if(GetAttribute(ATTRUNKOBJ_IGNORE)==0)
	{
		if(GetTexture() && GetTexture()->IsAlpha())
			DrawNode->Attach(SCENENODE_OBJECTSORT,this);
		else
			DrawNode->Attach(SCENENODE_OBJECT,this);
	}
}

void cPlane::Draw(cCamera *DrawNode)
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

	buf->DrawPrimitive(PT_TRIANGLESTRIP,2);
}

void cPlane::SetUV(float _umin,float _vmin,float _umax,float _vmax)
{
	umin=_umin;
	vmin=_vmin;
	umax=_umax;
	vmax=_vmax;
}
