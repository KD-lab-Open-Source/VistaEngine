#include "StdAfx.h"
#include "blobs.h"
#include "IVisD3D.h"
#include "..\Render\src\FileImage.h"

cBlobs::cBlobs()
{
	pRenderTarget=NULL;
	pBlobsShader=new PSBlobsShader;
	pBlobsShader->Restore();

	for(int i=0;i<num_planes;i++)
		planeTextures[i]=NULL;
}

cBlobs::~cBlobs()
{
	delete pBlobsShader;
	RELEASE(pRenderTarget);
	RELEASE(Texture);
	for(int i=0;i<num_planes;i++)
		RELEASE(planeTextures[i]);
}

void cBlobs::Init(int width,int height)
{
	pRenderTarget=GetTexLibrary()->CreateRenderTexture(width,height,TEXTURE_RENDER32);
}

void cBlobs::BeginDraw()
{
	if(gb_RenderDevice3D->IsPS20())
	{
		gb_RenderDevice3D->SetRenderTarget(pRenderTarget,NULL);
		RDCALL(gb_RenderDevice3D->lpD3DDevice->Clear(0,NULL,D3DCLEAR_TARGET,0,1,0));
	}
	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
	points.clear();
}

void cBlobs::Draw(int x,int y,int plane, float phase)
{
	points.push_back(Vect3f(x, y, phase));
}

void cBlobs::EndDraw(const cBlobsSetting& setting)
{
	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
	int dx=Texture->GetWidth(),dy=Texture->GetHeight();

	gb_RenderDevice3D->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,Texture);
	gb_RenderDevice3D->SetBlendStateAlphaRef(ALPHA_ADDBLEND);
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_point);

	cQuadBuffer<sVertexXYZWDT1>* quad=gb_RenderDevice3D->GetQuadBufferXYZWDT1();
	quad->BeginDraw();
	sColor4c color;
	if(gb_RenderDevice3D->IsPS20())
		color.set(255,255,255);
	else
		color = setting.color_;
	int dx2=dx/2,dy2=dy/2;
	float w=0.99f;
	for(vector<Vect3f>::iterator it=points.begin(),ite=points.end();it!=ite;++it)
	{
		Vect3f& p=*it;
		sVertexXYZWDT1 *v=quad->Get();
		v[0].x=p.x-dx2-0.5f; v[0].y=p.y-dy2-0.5f; v[0].z=v[0].w=w; v[0].u1()=0; v[0].v1()=0;v[0].diffuse = color; v[0].diffuse *= p.z;
		v[1].x=p.x-dx2-0.5f; v[1].y=p.y+dy2-0.5f; v[1].z=v[1].w=w; v[1].u1()=0; v[1].v1()=1;v[1].diffuse = color; v[1].diffuse *= p.z;
		v[2].x=p.x+dx2-0.5f; v[2].y=p.y-dy2-0.5f; v[2].z=v[2].w=w; v[2].u1()=1; v[2].v1()=0;v[2].diffuse = color; v[2].diffuse *= p.z;
		v[3].x=p.x+dx2-0.5f; v[3].y=p.y+dy2-0.5f; v[3].z=v[3].w=w; v[3].u1()=1; v[3].v1()=1;v[3].diffuse = color; v[3].diffuse *= p.z;
	}
	quad->EndDraw();

	gb_RenderDevice3D->RestoreRenderTarget();
}

void cBlobs::DrawBlobsSimply(int x,int y)
{
	gb_RenderDevice->DrawSprite(x,y,pRenderTarget->GetWidth(),pRenderTarget->GetHeight(),
						0,0,1,1,pRenderTarget,sColor4c(255,255,255,255),0,ALPHA_NONE);
}

void cBlobs::DrawBlobsShader(int x,int y, float phase, cTexture* texture, const cBlobsSetting& setting)
{
	//Не забыть выключить биоинейную интерполяцию наверно, для скорости потом.
	gb_RenderDevice3D->SetBlendStateAlphaRef(ALPHA_NONE);
	gb_RenderDevice3D->SetVertexShader(NULL);
	

	D3DXVECTOR4 def_color(setting.color_.r, setting.color_.g, setting.color_.b, setting.color_.a);
	D3DXVECTOR4 spec_color(setting.specularColor_.r, setting.specularColor_.g, setting.specularColor_.b, 1.0f);
	pBlobsShader->Select(pRenderTarget, def_color, spec_color, phase);
	//for(int i=0;i<num_planes;i++)
	//	gb_RenderDevice3D->SetTexture(i+1,planeTextures[i]);
	gb_RenderDevice3D->SetTexture(1,texture);
	gb_RenderDevice3D->DrawQuad(x,y,pRenderTarget->GetWidth(),pRenderTarget->GetHeight(),
		0,0,1,1,sColor4c(255,255,255,255));
}

//void cBlobs::SetTexture(const char* name)
//{
//	planeTextures[0] = GetTexLibrary()->GetElement2D(name);
//}
void cBlobs::SetTexture(cTexture* texture)
{
	planeTextures[0] = texture;
}

void cBlobs::CreateBlobsTexture(int size)
{
	Texture = new cTexture;
	Texture->SetNumberMipMap(5);
	Texture->SetAttribute(TEXTURE_32);
	Texture->SetAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_NODDS);

	Texture->BitMap.resize(1);
	Texture->BitMap[0]=0;
	Texture->SetWidth(size);
	Texture->SetHeight(size);

	sColor4c* data=new sColor4c[size*size];

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

	if(gb_RenderDevice3D->CreateTexture(Texture,&fid,-1,-1))
	{
		delete data;
		delete Texture;
		Texture = 0;
	}

	delete data;
}