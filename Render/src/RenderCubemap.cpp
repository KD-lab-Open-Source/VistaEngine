#include "StdAfxRd.h"
#include "RenderCubemap.h"
#include "D3DRender.h"
#include "scene.h"
#include "cCamera.h"
#include "VisGeneric.h"

cRenderCubemap::cRenderCubemap()
{
	pTexture=0;
	pZBuffer=0;
	pSceneBefore=gb_VisGeneric->CreateScene();
	for(int i=0;i<num_camera;i++)
	{
		camera[i]=pSceneBefore->CreateCamera();
		camera[i]->setAttribute(ATTRCAMERA_PERSPECTIVE);
	}
	
	Color4c fone_color(162,202,251,0);
	SetFoneColor(fone_color);
	cur_draw_phase=-1;
}

cRenderCubemap::~cRenderCubemap()
{
	RELEASE(pZBuffer);
	RELEASE(pSceneBefore);
	for(int i=0;i<num_camera;i++)
	{
		RELEASE(camera[i]);
	}

	xassert(pTexture && pTexture->GetRef()==1);
	RELEASE(pTexture);
}

void cRenderCubemap::deleteManagedResource()
{
	if(pTexture)
	{
		RELEASE(pTexture->BitMap[0]);
	}
	RELEASE(pZBuffer);
	cur_draw_phase=-1;
}

void cRenderCubemap::restoreManagedResource()
{
	deleteManagedResource();
	if(gb_RenderDevice3D->CreateCubeTexture(pTexture))
	{
		xassert(0 && "Low videomemory. Cannot create cubemap.");
		return;
	}
/*
	RDCALL(gb_RenderDevice3D->lpD3DDevice->CreateDepthStencilSurface(linear_size, linear_size, 
		D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &pZBuffer, 0));
*/
	IDirect3DCubeTexture9 * pCube=(IDirect3DCubeTexture9*)pTexture->BitMap[0];
	for(int i=0;i<num_camera;i++)
	{
		IDirect3DSurface9 *pCubeMapSurface=0;
		RDCALL(pCube->GetCubeMapSurface((D3DCUBEMAP_FACES)i,0,&pCubeMapSurface));
		camera[i]->SetRenderTarget(pCubeMapSurface,pZBuffer);
		pCubeMapSurface->Release();
	}
}

void cRenderCubemap::Init(int linear_size_,Vect3f camera_pos)
{
	linear_size=linear_size_;
	pTexture=new cTexture;
	pTexture->New(1);
	pTexture->SetTimePerFrame(0);	
	pTexture->SetWidth(linear_size);
	pTexture->SetHeight(linear_size);
	pTexture->setMipmapNumber(1);
	pTexture->setAttribute(TEXTURE_RENDER32);
	restoreManagedResource();

	float far_zplane=20000.0f;
	for(int i=0;i<num_camera;i++)
	{
		camera[i]->SetFrustum(							// устанавливается пирамида видимости
			&Vect2f(0.5f,0.5f),							// центр камеры
			&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),		// видимая область камеры
			//&Vect2f(1.f,1.f),							// фокус камеры
			&Vect2f(0.5f,0.5f),							// фокус камеры
			&Vect2f(30.0f,far_zplane)
			);
	}

	MatXf m=MatXf::ID;
	m.rot().set(Vect3f(0,1,0),M_PI_2);
	m.rot()=Mat3f(Vect3f(0,0,1),M_PI)*m.rot();
	m.rot().invXform(camera_pos,m.trans());
	camera[D3DCUBEMAP_FACE_POSITIVE_X]->SetPosition(m);
	m.rot().set(Vect3f(0,1,0),-M_PI_2);
	m.rot()=Mat3f(Vect3f(0,0,1),M_PI)*m.rot();
	m.rot().invXform(camera_pos,m.trans());
	camera[D3DCUBEMAP_FACE_NEGATIVE_X]->SetPosition(m);

	m.rot().set(Vect3f(1,0,0),-M_PI_2);
	m.rot()=Mat3f(Vect3f(0,0,1),M_PI)*m.rot();
	m.rot().invXform(camera_pos,m.trans());
	camera[D3DCUBEMAP_FACE_POSITIVE_Y]->SetPosition(m);
	m.rot().set(Vect3f(1,0,0),M_PI_2);
	m.rot()=Mat3f(Vect3f(0,0,1),M_PI)*m.rot();
	m.rot().invXform(camera_pos,m.trans());
	camera[D3DCUBEMAP_FACE_NEGATIVE_Y]->SetPosition(m);

	m.rot()=Mat3f::ID;
	m.rot()=Mat3f(Vect3f(0,0,1),M_PI);
	m.rot().invXform(camera_pos,m.trans());
	camera[D3DCUBEMAP_FACE_POSITIVE_Z]->SetPosition(m);
	m.rot().set(Vect3f(1,0,0),M_PI_2);
	m.rot().invXform(camera_pos,m.trans());
	camera[D3DCUBEMAP_FACE_NEGATIVE_Z]->SetPosition(m);
}

void cRenderCubemap::Animate(float dt)
{
	pSceneBefore->SetDeltaTime(dt);
}

void cRenderCubemap::DrawOne(int i)
{
	camera[i]->clearAttribute(ATTRCAMERA_NOCLEARTARGET);
	camera[i]->clearAttribute(ATTRCAMERA_WRITE_ALPHA);
	camera[i]->setScene(pSceneBefore);
	pSceneBefore->Draw(camera[i]);
}

void cRenderCubemap::Draw()
{
	if(cur_draw_phase==-1)
	{
		for(int i=0;i<num_camera;i++)
			DrawOne(i);
	}else
	{
		DrawOne(cur_draw_phase);
	}

	cur_draw_phase=(cur_draw_phase+1)%num_camera;
	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
}

void cRenderCubemap::Save(const char* file_name)
{
	RDCALL(D3DXSaveTextureToFile(file_name,D3DXIFF_DDS, pTexture->BitMap[0],0));
}

void cRenderCubemap::SetFoneColor(Color4c fone_color)
{
	fone_color.a=0;
	for(int i=0;i<num_camera;i++)
	{
		camera[i]->SetFoneColor(fone_color);
	}
}
