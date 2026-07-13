#include "StdAfxRD.h"
#include "UnkLight.h"
#include "D3DRender.h"
#include "cCamera.h"
#ifndef _WIN32
#include "Render/SDLRenderDevice.h"        // the light sprite, reached via drawWorldQuads
#include "Render/SDLWorldQuadRenderer.h"
#endif

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
	Color4c Diffuse(GetDiffuse().a*GetDiffuse().r*255,
					GetDiffuse().a*GetDiffuse().g*255,
					GetDiffuse().a*GetDiffuse().b*255,255);

#ifdef _WIN32
	DrawStrip strip;
	gb_RenderDevice3D->SetWorldMaterial(ALPHA_ADDBLENDALPHA,MatXf::ID,0,GetTexture());//???

	cVertexBuffer<sVertexXYZDT1>* buf=gb_RenderDevice->GetBufferXYZDT1();
	sVertexXYZDT1 *v=buf->Lock(4);
#else
	// The billboard is four corners written as two opposite edges -- 0,1 then 2,3 -- which
	// is exactly what the world-quad renderer's index pattern covers, so the strip becomes
	// one of its quads and the corner maths below is unchanged.
	SDLWorldQuadRenderer* buf = sdlWorldQuadRenderer();
	if(!buf)
		return;
	buf->SetCamera(camera);
	buf->SetMaterial(ALPHA_ADDBLENDALPHA, GetTexture());
	buf->BeginDraw();
	sVertexXYZDT1* v = buf->Get();
#endif
	Vect3f sx=GetRadius()*camera->GetWorldI(),sy=GetRadius()*camera->GetWorldJ();
	v[0].pos=GetGlobalMatrix().trans()+sx+sy; v[0].u1()=0, v[0].v1()=0;
	v[1].pos=GetGlobalMatrix().trans()+sx-sy; v[1].u1()=0, v[1].v1()=1;
	v[2].pos=GetGlobalMatrix().trans()-sx+sy; v[2].u1()=1, v[2].v1()=0;
	v[3].pos=GetGlobalMatrix().trans()-sx-sy; v[3].u1()=1, v[3].v1()=1;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=Diffuse;
#ifdef _WIN32
	buf->Unlock(4);

	buf->DrawPrimitive(PT_TRIANGLESTRIP,2);
#else
	buf->EndDraw();
	// D3D drew as DrawPrimitive went; open the pass here, where the sprite sits in the
	// sorted pass. cUnkLight::PreDraw attaches it to SCENENODE_OBJECTSORT.
	if(cSDLRenderDevice* dev = sdlRenderDevice())
		dev->drawWorldQuads();
#endif
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
