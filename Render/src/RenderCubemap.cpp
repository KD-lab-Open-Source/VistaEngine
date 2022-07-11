#include "StdAfxRd.h"
#include "scene.h"
#include "cCamera.h"

#include "RenderCubemap.h"


cRenderCubemap::cRenderCubemap()
{
	pTexture=NULL;
	pZBuffer=NULL;
	pSceneBefore=gb_VisGeneric->CreateScene();
	for(int i=0;i<num_camera;i++)
	{
		pCamera[i]=pSceneBefore->CreateCamera();
		pCamera[i]->SetAttr(ATTRCAMERA_PERSPECTIVE);
	}
	
	sColor4c fone_color(162,202,251,0);
	SetFoneColor(fone_color);
	cur_draw_phase=-1;
}

cRenderCubemap::~cRenderCubemap()
{
	RELEASE(pZBuffer);
	RELEASE(pSceneBefore);
	for(int i=0;i<num_camera;i++)
	{
		RELEASE(pCamera[i]);
	}

	xassert(pTexture && pTexture->GetRef()==1);
	RELEASE(pTexture);
}

void cRenderCubemap::DeleteDefaultResource()
{
	if(pTexture)
	{
		RELEASE(pTexture->BitMap[0]);
	}
	RELEASE(pZBuffer);
	cur_draw_phase=-1;
}

void cRenderCubemap::RestoreDefaultResource()
{
	DeleteDefaultResource();
	if(gb_RenderDevice3D->CreateCubeTexture(pTexture))
	{
		xassert(0 && "Low videomemory. Cannot create cubemap.");
		return;
	}
/*
	RDCALL(gb_RenderDevice3D->lpD3DDevice->CreateDepthStencilSurface(linear_size, linear_size, 
		D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &pZBuffer, NULL));
*/
	IDirect3DCubeTexture9 * pCube=(IDirect3DCubeTexture9*)GETIDirect3DTexture(pTexture->BitMap[0]);
	for(int i=0;i<num_camera;i++)
	{
		IDirect3DSurface9 *pCubeMapSurface=NULL;
		RDCALL(pCube->GetCubeMapSurface((D3DCUBEMAP_FACES)i,0,&pCubeMapSurface));
		pCamera[i]->SetRenderTarget(pCubeMapSurface,pZBuffer);
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
	pTexture->SetNumberMipMap(1);
	pTexture->SetAttribute(TEXTURE_RENDER32);
	RestoreDefaultResource();

	float far_zplane=20000.0f;
	for(int i=0;i<num_camera;i++)
	{
		pCamera[i]->SetFrustum(							// устанавливается пирамида видимости
			&Vect2f(0.5f,0.5f),							// центр камеры
			&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),		// видимая область камеры
			//&Vect2f(1.f,1.f),							// фокус камеры
			&Vect2f(0.5f,0.5f),							// фокус камеры
			&Vect2f(30.0f,far_zplane)
			);
	}

	MatXf m=MatXf::ID;
	m.rot().set(Vect3f(0,1,0),M_PI/2);
	m.rot()=Mat3f(Vect3f(0,0,1),M_PI)*m.rot();
	m.rot().invXform(camera_pos,m.trans());
	pCamera[D3DCUBEMAP_FACE_POSITIVE_X]->SetPosition(m);
	m.rot().set(Vect3f(0,1,0),-M_PI/2);
	m.rot()=Mat3f(Vect3f(0,0,1),M_PI)*m.rot();
	m.rot().invXform(camera_pos,m.trans());
	pCamera[D3DCUBEMAP_FACE_NEGATIVE_X]->SetPosition(m);

	m.rot().set(Vect3f(1,0,0),-M_PI/2);
	m.rot()=Mat3f(Vect3f(0,0,1),M_PI)*m.rot();
	m.rot().invXform(camera_pos,m.trans());
	pCamera[D3DCUBEMAP_FACE_POSITIVE_Y]->SetPosition(m);
	m.rot().set(Vect3f(1,0,0),M_PI/2);
	m.rot()=Mat3f(Vect3f(0,0,1),M_PI)*m.rot();
	m.rot().invXform(camera_pos,m.trans());
	pCamera[D3DCUBEMAP_FACE_NEGATIVE_Y]->SetPosition(m);

	m.rot()=Mat3f::ID;
	m.rot()=Mat3f(Vect3f(0,0,1),M_PI);
	m.rot().invXform(camera_pos,m.trans());
	pCamera[D3DCUBEMAP_FACE_POSITIVE_Z]->SetPosition(m);
	m.rot().set(Vect3f(1,0,0),M_PI/2);
	m.rot().invXform(camera_pos,m.trans());
	pCamera[D3DCUBEMAP_FACE_NEGATIVE_Z]->SetPosition(m);
}

void cRenderCubemap::Animate(float dt)
{
	pSceneBefore->SetDeltaTime(dt);
}

void cRenderCubemap::DrawOne(int i)
{
	pCamera[i]->ClearAttribute(ATTRCAMERA_NOCLEARTARGET);
	pCamera[i]->ClearAttribute(ATTRCAMERA_WRITE_ALPHA);
	pCamera[i]->GetScene()=pSceneBefore;
	pSceneBefore->Draw(pCamera[i]);
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
	RDCALL(D3DXSaveTextureToFile(file_name,D3DXIFF_DDS,GETIDirect3DTexture(pTexture->BitMap[0]),NULL));
}

extern bool LoadTGA(const char* filename,int& dx,int& dy,unsigned char*& buf,
			 int& byte_per_pixel);
bool Load1DTexture(const char* filename,vector<sColor4c>& colors)
{
	int dx,dy;
	unsigned char* buf;
	int byte_per_pixel;
	bool ret=LoadTGA(filename,dx,dy,buf,byte_per_pixel);
	if(byte_per_pixel!=3 || !ret)
	{
		xassert_s(0,filename);
		delete[] buf;
		return false;
	}

	BYTE* ptr=buf;
	colors.resize(dx);
	for(int i=0;i<dx;i++)
	{
		sColor4c c(ptr[2],ptr[1],ptr[0]);
		colors[i]=c;
		ptr+=3;
	}

	delete[] buf;
	return true;
}

void cRenderCubemap::SetFoneColor(sColor4c fone_color)
{
	fone_color.a=0;
	for(int i=0;i<num_camera;i++)
	{
		pCamera[i]->SetFoneColor(fone_color);
	}
}
