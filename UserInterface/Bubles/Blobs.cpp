#include "stdafx.h"
#include "Blobs.h"
#include "Render/src/FileImage.h"
#include "Render/src/Texture.h"
#include "Render/SDLRenderDevice.h"
#include "Render/SDLBlobsRenderer.h"

// Ported to SDL GPU. SDLBlobsRenderer takes what this pushed into the device's quad buffer
// and PSBlobsShader, and cSDLRenderDevice::drawBlobs runs both of its passes -- the cells
// into the metaball field, then field + frame -> the swapchain. See SDLBlobsRenderer.h.
//
// Everything below the draw calls was always portable and never stopped running: the cell
// list, the falloff texture, the settings.

cBlobs::cBlobs()
{
	Texture = NULL;
}

cBlobs::~cBlobs()
{
	RELEASE(Texture);
}

// The frame the composite refracts. D3D9 rendered the scene to the back buffer and
// StretchRect'ed a copy out of it each frame; SDL GPU cannot sample the swapchain, so the
// scene walk is routed into the capture target instead and the composite reads that. It
// has to be armed before anything draws -- hence a call of its own, at the top of the
// frame, rather than inside BeginDraw (which runs after the scene).
void cBlobs::BeginFrame()
{
	cSDLRenderDevice* device = sdlRenderDevice();
	if(!device)
		return;
	device->armSceneCapture();
}

void cBlobs::Init(int /*width*/, int /*height*/)
{
	// The field target was cTexLibrary::CreateRenderTexture(width, height, TEXTURE_RENDER32)
	// here. SDLBlobsRenderer owns it now and sizes it to the target it composites onto,
	// which is the same screen size every caller passed.
}

void cBlobs::BeginDraw()
{
	if(SDLBlobsRenderer* renderer = sdlBlobsRenderer())
		renderer->BeginCells();
	points.clear();
}

void cBlobs::Draw(int x, int y, int plane, float phase)
{
	points.push_back(Vect3f(x, y, phase));
}

void cBlobs::EndDraw()
{
	SDLBlobsRenderer* renderer = sdlBlobsRenderer();
	if(!renderer || !Texture)
		return;

	// One additive sprite per cell, white scaled by the cell's phase -- the original drew
	// them in cBlobsSetting::color_ only on the no-PS2.0 path, which is not ported (the
	// composite tints the field itself). See SDLBlobsRenderer.h.
	for(vector<Vect3f>::iterator it = points.begin(), ite = points.end(); it != ite; ++it)
		renderer->AddCell(it->x, it->y, it->z);

	renderer->EndCells(Texture);
}

void cBlobs::DrawBlobsShader(float phase, const cBlobsSetting& setting)
{
	SDLBlobsRenderer* renderer = sdlBlobsRenderer();
	cSDLRenderDevice* device = sdlRenderDevice();
	if(!renderer || !device)
		return;

	renderer->recordComposite(phase, setting.color_, setting.specularColor_);
	device->drawBlobs();
}

void cBlobs::CreateBlobsTexture(int size)
{
	Texture = new cTexture;
	Texture->setMipmapNumber(5);
	Texture->setAttribute(TEXTURE_32);
	Texture->setAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_NODDS);

	Texture->BitMap.resize(1);
	Texture->BitMap[0]=0;
	Texture->SetWidth(size);
	Texture->SetHeight(size);

	Color4c* data=new Color4c[size*size];

	float s2=size/2.0f;
	float is2=1/s2;
	for(int y=0;y<size;y++)
	for(int x=0;x<size;x++)
	{
		float xx=(x-s2)*is2;
		float yy=(y-s2)*is2;
		float s = sqrt(xx*xx + yy*yy);
		if(s > 1)
			s=0;
		else
		{
//			s=(1+cos((1-s)*M_PI))*0.35f + xm_random_generator.frand()*1/255.0f;
			s=0.2 * (1 + cos(s*M_PI)) * 0.5f;
//			s= 0.2 * (1 - sqrt(1-sqr(s-1)));
//			if(s > 0.9)
//				s=0.9f;
		}
		BYTE c=round(255*s);
		data[x+y*size].set(c,c,c,c);
	}

	cFileImageData fid(size,size,data);

	// Creating a texture is the render *interface*'s job, not the D3D9 device's:
	// gb_RenderDevice3D is null now that the backend is retired (Render/RenderStub.cpp),
	// and this was a null dereference.
	if(gb_RenderDevice->CreateTexture(Texture,&fid,-1,-1))
	{
		delete Texture;
		Texture = 0;
	}

	// `data` is an array, and it was freed twice on the failure path above.
	delete[] data;
}
