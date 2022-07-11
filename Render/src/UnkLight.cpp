#include "StdAfxRD.h"
#include "UnkLight.h"

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
void cUnkLight::SetAnimation(float Period,float StartPhase,float FinishPhase,bool recursive)
{
	TimeLife = Period;
}
void cUnkLight::PreDraw(cCamera *DrawNode)
{
	if( !Key.empty() ) 
	{
		int nKey = round( floor( Key.size()*fmodf( CurrentTime/TimeLife, 0.999f ) ) );
		VISASSERT( 0<=nKey && nKey<Key.size() );
		GetRadius() = max(Key[nKey].radius,1.0f);
		GetRealRadius() = max(Key[nKey].radius,1.0f);
		sColor4f d(Key[nKey].diffuse.r/255.f, Key[nKey].diffuse.g/255.f, Key[nKey].diffuse.b/255.f, Key[nKey].diffuse.a/255.f);
		SetDiffuse(d);
	}

	if( GetAttribute(ATTRLIGHT_SPHERICAL_SPRITE) && !GetAttribute(ATTRLIGHT_IGNORE) )
		if( DrawNode->TestVisible(GetGlobalMatrix().trans(),GetRadius()) )
			DrawNode->Attach(SCENENODE_OBJECTSORT,this); // спрайты всегда выводятся последними
}
void cUnkLight::Draw(cCamera *DrawNode)
{
	DrawStrip strip;
	gb_RenderDevice3D->SetWorldMaterial(ALPHA_ADDBLENDALPHA,MatXf::ID,0,GetTexture());//???
	sColor4c Diffuse(GetDiffuse().a*GetDiffuse().r*255,
					GetDiffuse().a*GetDiffuse().g*255,
					GetDiffuse().a*GetDiffuse().b*255,255);

	cVertexBuffer<sVertexXYZDT1>* buf=gb_RenderDevice->GetBufferXYZDT1();
	sVertexXYZDT1 *v=buf->Lock(4);
	Vect3f sx=GetRealRadius()*DrawNode->GetWorldI(),sy=GetRealRadius()*DrawNode->GetWorldJ();
	v[0].pos=GetGlobalMatrix().trans()+sx+sy; v[0].u1()=0, v[0].v1()=0; 
	v[1].pos=GetGlobalMatrix().trans()+sx-sy; v[1].u1()=0, v[1].v1()=1;
	v[2].pos=GetGlobalMatrix().trans()-sx+sy; v[2].u1()=1, v[2].v1()=0;
	v[3].pos=GetGlobalMatrix().trans()-sx-sy; v[3].u1()=1, v[3].v1()=1;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=Diffuse;
	buf->Unlock(4);

	buf->DrawPrimitive(PT_TRIANGLESTRIP,2);
}
const MatXf& cUnkLight::GetPosition() const
{
	return GlobalMatrix;//странно это GetLocalMatrix();
}
void cUnkLight::SetPosition(const MatXf& Matrix)
{
	MTAccess();
	//GetPos()=Matrix.trans();
	GlobalMatrix = Matrix;
}
void cUnkLight::SetDirection(const Vect3f& direction)
{
	Direction=direction;
	Direction.Normalize(0.995f);
}
const Vect3f& cUnkLight::GetDirection() const
{
	return Direction;
}
void cUnkLight::SetAnimKeys(sLightKey *AnimKeys,int size)
{
	Key.resize(size);
	for(int i=0;i<size;i++)
		Key[i]=AnimKeys[i];
}
