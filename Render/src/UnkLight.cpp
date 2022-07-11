#include "StdAfxRD.h"
#include "UnkLight.h"
#include "D3DRender.h"
#include "cCamera.h"

cUnkLight::cUnkLight() : cUnkObj(KIND_LIGHT)
{
	Direction.set(0,0,-0.995f);
	CurrentTime=0;
	TimeLife=1000;
	blendingMode = ALPHA_ADDBLEND;
}

cUnkLight::~cUnkLight()
{
}

void cUnkLight::Animate(float dt)
{
	if(CurrentTime==0) dt=0.1f;
	CurrentTime+=dt;
}

void cUnkLight::SetAnimationPeriod(float Period)
{
	TimeLife = Period;
}

void cUnkLight::PreDraw(Camera* camera)
{
	if(!Key.empty()){
		int nKey = round( floor( Key.size()*fmodf( CurrentTime/TimeLife, 0.999f ) ) );
		xassert( 0<=nKey && nKey<Key.size() );
		SetRadius(max(Key[nKey].radius,1.0f));
		Color4f d(Key[nKey].diffuse.r/255.f, Key[nKey].diffuse.g/255.f, Key[nKey].diffuse.b/255.f, Key[nKey].diffuse.a/255.f);
		SetDiffuse(d);
	}

	if( getAttribute(ATTRLIGHT_SPHERICAL_SPRITE) && !getAttribute(ATTRLIGHT_IGNORE) )
		if( camera->TestVisible(GetGlobalMatrix().trans(),GetRadius()) )
			camera->Attach(SCENENODE_OBJECTSORT,this); // спрайты всегда выводятся последними
}

void cUnkLight::Draw(Camera* camera)
{
	DrawStrip strip;
	gb_RenderDevice3D->SetWorldMaterial(ALPHA_ADDBLENDALPHA,MatXf::ID,0,GetTexture());//???
	Color4c Diffuse(GetDiffuse().a*GetDiffuse().r*255,
					GetDiffuse().a*GetDiffuse().g*255,
					GetDiffuse().a*GetDiffuse().b*255,255);

	cVertexBuffer<sVertexXYZDT1>* buf=gb_RenderDevice->GetBufferXYZDT1();
	sVertexXYZDT1 *v=buf->Lock(4);
	Vect3f sx=GetRadius()*camera->GetWorldI(),sy=GetRadius()*camera->GetWorldJ();
	v[0].pos=GetGlobalMatrix().trans()+sx+sy; v[0].u1()=0, v[0].v1()=0; 
	v[1].pos=GetGlobalMatrix().trans()+sx-sy; v[1].u1()=0, v[1].v1()=1;
	v[2].pos=GetGlobalMatrix().trans()-sx+sy; v[2].u1()=1, v[2].v1()=0;
	v[3].pos=GetGlobalMatrix().trans()-sx-sy; v[3].u1()=1, v[3].v1()=1;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=Diffuse;
	buf->Unlock(4);

	buf->DrawPrimitive(PT_TRIANGLESTRIP,2);
}

void cUnkLight::SetDirection(const Vect3f& direction)
{
	Direction = direction;
	Direction.normalize(0.995f);
}

void cUnkLight::SetAnimKeys(sLightKey *AnimKeys,int size)
{
	Key.resize(size);
	for(int i=0;i<size;i++)
		Key[i]=AnimKeys[i];
}
