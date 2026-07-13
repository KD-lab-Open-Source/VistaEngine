#include "stdafx.h"
#include "CloudShadow.h"
#include "Terra/VMAP.H"
#include "Serialization/ResourceSelector.h"
#include "Render/D3D/D3DRender.h"
#include "Render/SDLRenderDevice.h"
#include "Render/SDLCloudShadowRenderer.h"
#include "XMath/SafeMath.h"
#include "SkyObject.h"
#include "Render/src/TileMap.h"
#include "Render/src/Scene.h"

static const int size_vb = 4;
static const int size_ib = 2;

cCloudShadow::cCloudShadow() : BaseGraphObject(0)
{
	setAttribute(ATTRCAMERA_SHADOW|ATTRUNKOBJ_IGNORE_NORMALCAMERA);
	texture1 = 0;
	tex1_name = "Scripts\\Resource\\Textures\\cloud_shadow.tga";
	SetTexture(tex1_name);
	uv1.set(0,0);
	uv2.set(0,0);
	du=vMap.H_SIZE/2048;
	dv=vMap.V_SIZE/2048;
	rotate_angle=0;

	gb_RenderDevice->CreateVertexBuffer(earth_vb, size_vb,VType::declaration);
	gb_RenderDevice->CreateIndexBuffer(earth_ib, size_ib);

	{
		VType* vx=(VType*)gb_RenderDevice->LockVertexBuffer(earth_vb);
		Vect2i size((int)vMap.H_SIZE, (int)vMap.V_SIZE);
		vx[0].pos.set(0,0, 0);
		vx[1].pos.set(0,size.y, 0);
		vx[2].pos.set(size.x,size.y, 0);
		vx[3].pos.set(size.x,0, 0);

		const int gc = 255;
		Color4c color_diffuse(gc, gc, gc, 255);
		for(int i=0;i<size_vb;i++)
			vx[i].diffuse = color_diffuse;
		gb_RenderDevice->UnlockVertexBuffer(earth_vb);

		SetTexels();
	}

	{
		sPolygon* pt=gb_RenderDevice->LockIndexBuffer(earth_ib);
		pt[0].set(0,1,2);
		pt[1].set(2,3,0);
		gb_RenderDevice->UnlockIndexBuffer(earth_ib);
	}

	color=128;

}
cCloudShadow::~cCloudShadow()
{
	RELEASE(texture1);
}

//void cCloudShadow::SetColor(const Color4c& color)
//{
//	cD3DRender* rd=gb_RenderDevice3D;
//	VType* vx=(VType*)rd->LockVertexBuffer(earth_vb);
//	for(int i=0;i<size_vb;i++)
//		vx[i].diffuse = color;
//	rd->UnlockVertexBuffer(earth_vb);
//}

void cCloudShadow::PreDraw(Camera* camera)
{
	camera->Attach(SCENENODE_OBJECTFIRST,this);
}

// Ported to SDL GPU. SDLCloudShadowRenderer takes what this pushed into VSCloudShadow /
// PSCloudShadow, and cSDLRenderDevice::drawCloudShadow opens the pass right here, where the
// scene walk reached us. Everything above -- the quad, the two scrolling texture coordinate
// sets, the animation -- was always portable and never stopped running.
//
// The camera is the planar LIGHT camera (cScene gives it ATTRCAMERA_SHADOW), so this draws
// into the terrain lightmap, not into the view. The terrain and the grass both add the
// lightmap to their own light, so they pick the clouds up with no change of their own. See
// SDLCloudShadowRenderer.h.
void cCloudShadow::Draw(Camera* camera)
{
	if(!camera->getAttribute(ATTRCAMERA_SHADOW))
		return;

	SDLCloudShadowRenderer* renderer = sdlCloudShadowRenderer();
	cSDLRenderDevice* device = sdlRenderDevice();
	if(!renderer || !device || !texture1)
		return;
	cTileMap* tileMap = scene_ ? scene_->GetTileMap() : 0;
	if(!tileMap)
		return;

	SDLCloudShadowRenderer::State state;
	state.texture = texture1;

	// tfactor, exactly as the original builds it: the tile map's diffuse flattened to its own
	// grey average (the clouds darken, they do not tint), scaled by the world's serialized
	// cloud intensity, and then by how high the sun stands. -sunDirection.z goes to zero as
	// the sun reaches the horizon, so the shadows fade out at dusk and are gone at night.
	Color4f tfactor = tileMap->GetDiffuse();
	tfactor.r =
	tfactor.g =
	tfactor.b = (tfactor.r + tfactor.g + tfactor.b) / 3;
	tfactor *= color/255.0f;
	tfactor *= -scene_->GetSunDirection().z;
	tfactor.r = clamp(tfactor.r, 0.0f, 1.0f);
	tfactor.g = clamp(tfactor.g, 0.0f, 1.0f);
	tfactor.b = clamp(tfactor.b, 0.0f, 1.0f);
	tfactor.a = tfactor.r;
	state.tfactor = tfactor;

	renderer->SetState(state, camera);
	renderer->DrawIndexedPrimitive(earth_vb, earth_ib, size_ib);
	device->drawCloudShadow();
}


void cCloudShadow::Animate(float dt)
{
	static float k = 5e-3f;
	dt*=k;

	static Vect2f d1(-0.001f,-0.001f);
	static Vect2f d2(-0.002f,+0.0001f);
	Mat2f rotate(rotate_angle*M_PI/180);
	
	uv1+=rotate*(d1*dt);
	uv2+=rotate*(d2*dt);

	uv1.x=cycle(uv1.x,1);
	uv1.y=cycle(uv1.y,1);
	uv2.x=cycle(uv2.x,1);
	uv2.y=cycle(uv2.y,1);

	SetTexels();
}

void cCloudShadow::SetTexels()
{
	VType* vx=(VType*)gb_RenderDevice->LockVertexBuffer(earth_vb);

	vx[0].GetTexel1().set(uv1.x, uv1.y);
	vx[1].GetTexel1().set(uv1.x, uv1.y+dv);
	vx[2].GetTexel1().set(uv1.x+du, uv1.y+dv);
	vx[3].GetTexel1().set(uv1.x+du ,uv1.y);
	
	vx[0].GetTexel2().set(uv2.x, uv2.y);
	vx[1].GetTexel2().set(uv2.x, uv2.y+dv);
	vx[2].GetTexel2().set(uv2.x+du, uv2.y+dv);
	vx[3].GetTexel2().set(uv2.x+du, uv2.y);

	gb_RenderDevice->UnlockVertexBuffer(earth_vb);
}

void cCloudShadow::SetTexture(const string& tex1)
{
	RELEASE(texture1);
	//texture1 = GetTexLibrary()->GetElement(tex1.c_str());
	texture1 = GetTexLibrary()->GetElement3D(tex1.c_str());
}
void cCloudShadow::serialize(Archive& ar)
{
	static ResourceSelector::Options textureOptions("*.tga", "Resource\\TerrainData\\Textures");
	ar.serialize(ResourceSelector(tex1_name, textureOptions), "cloudShadowTexture1", "Текстура теней облаков");
	ar.serialize(color, "cloudShadowAlpha", "Интенсивность теней облаков (0-255)");
	ar.serialize(rotate_angle, "rotate_angle", "Угол вращения движения облаков (0-360)");
	
	if (ar.isInput())
		SetTexture(tex1_name);
}

//void cCloudShadow::serializeColor(Archive& ar)
//{
//	ar.serialize(color, "cloudShadowColor", "Цвет теней облаков");
//	if (ar.isInput())
//		SetColor(color);
//}
