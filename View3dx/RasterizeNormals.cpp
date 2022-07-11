#include "stdafx.h"
#include "RasterizeNormals.h"
#include "..\Render\inc\IVisD3D.h"
#include "..\Render\inc\VertexFormat.h"
bool SaveTga(const char* filename,int width,int height,unsigned char* buf,int byte_per_pixel);
class vsRasterize:public cVertexShader
{
public:
	void RestoreShader()
	{
		LoadShader("RasterizeNormals\\vs.vsl");
	}

	void Select()
	{
		gb_RenderDevice3D->SetVertexShader(shader->GetVertexShader());
	}
};
class psRasterize:public cPixelShader
{
public:
	void RestoreShader()
	{
		LoadShader("RasterizeNormals\\ps.psl");
	}

	void Select()
	{
		gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
	}
};

RasterizeNormals::RasterizeNormals()
{
	pRenderTarget=NULL;
	pDestTexture=NULL;
}

RasterizeNormals::~RasterizeNormals()
{
	RELEASE(pRenderTarget);
	RELEASE(pDestTexture);
}

void RasterizeNormals::Init(int dx_,int dy_,cObject3dx* obj, int nmaterial)
{
	dx=dx_;
	dy=dy_;
	xassert(!pRenderTarget);
	pRenderTarget=CreateRenderTexture(dx,dy,true);
	pDestTexture=CreateRenderTexture(dx,dy,false);
	Render(obj,nmaterial);

	{
		HRESULT hr;
		IDirect3DSurface9* pSrc=NULL;
		IDirect3DSurface9* pDst=NULL;

		hr=pRenderTarget->BitMap[0]->GetSurfaceLevel(0,&pSrc);
		xassert(SUCCEEDED(hr));
		hr=pDestTexture->BitMap[0]->GetSurfaceLevel(0,&pDst);
		xassert(SUCCEEDED(hr));
		hr=gb_RenderDevice3D->lpD3DDevice->GetRenderTargetData(pSrc,pDst);
		xassert(SUCCEEDED(hr));
		RELEASE(pSrc);
		RELEASE(pDst);
	}

	int k=0;

	if(false)
	{
		HRESULT hr;
		IDirect3DSurface9* pDst=NULL;
		hr=pDestTexture->BitMap[0]->GetSurfaceLevel(0,&pDst);
		xassert(SUCCEEDED(hr));
		sColor4c* buf=new sColor4c[dx*dy];
		D3DLOCKED_RECT lock;
		hr=pDst->LockRect(&lock,NULL,D3DLOCK_READONLY);
		xassert(SUCCEEDED(hr));

		for(int y=0;y<dy;y++)
		{
			Vect4f* in=(Vect4f*)(y*lock.Pitch+(BYTE*)lock.pBits);
			for(int x=0;x<dx;x++,in++)
			{
				buf[x+dx*y].set(clamp(in->x,0,1)*255,clamp(in->y,0,1)*255,clamp(in->z,0,1)*255,255);
			}
		}

		SaveTga("out.tga",dx,dy,(unsigned char*)buf,4);
		pDst->UnlockRect();
		RELEASE(pDst);
		delete buf;
	}

	{
		HRESULT hr;
		IDirect3DSurface9* pDst=NULL;
		hr=pDestTexture->BitMap[0]->GetSurfaceLevel(0,&pDst);
		xassert(SUCCEEDED(hr));
		Vect3f* buf=new Vect3f[dx*dy];
		D3DLOCKED_RECT lock;
		hr=pDst->LockRect(&lock,NULL,D3DLOCK_READONLY);
		xassert(SUCCEEDED(hr));

		for(int y=0;y<dy;y++)
		{
			Vect4f* in=(Vect4f*)(y*lock.Pitch+(BYTE*)lock.pBits);
			for(int x=0;x<dx;x++,in++)
			{
				buf[x+dx*y].set(in->x,in->y,in->z);
			}
		}

		pDst->UnlockRect();
		RELEASE(pDst);

		cTexture* pTexture = GetTexLibrary()->CreateNormalMap(dx,dy);
		pTexture->BuildNormalMap(buf);
		pTexture->SaveDDS("out.dds");
		RELEASE(pTexture);
		delete buf;
	}
}

void RasterizeNormals::GetMap(vector<Vect3f>& nmap)
{
	nmap.resize(dx*dy);
	HRESULT hr;
	IDirect3DSurface9* pDst=NULL;
	hr=pDestTexture->BitMap[0]->GetSurfaceLevel(0,&pDst);
	xassert(SUCCEEDED(hr));
	Vect3f* buf=new Vect3f[dx*dy];
	D3DLOCKED_RECT lock;
	hr=pDst->LockRect(&lock,NULL,D3DLOCK_READONLY);
	xassert(SUCCEEDED(hr));

	for(int y=0;y<dy;y++)
	{
		Vect4f* in=(Vect4f*)(y*lock.Pitch+(BYTE*)lock.pBits);
		for(int x=0;x<dx;x++,in++)
		{
			nmap[x+dx*y].set(in->x,in->y,in->z);
		}
	}

	pDst->UnlockRect();
	RELEASE(pDst);
}

cTexture* RasterizeNormals::CreateRenderTexture(int width,int height,bool render)
{
	cTexture *Texture=new cTexture("");
	Texture->New(1);
	Texture->SetTimePerFrame(0);	
	Texture->SetWidth(width);
	Texture->SetHeight(height);
	Texture->SetNumberMipMap(1);
	Texture->SetAttribute(TEXTURE_RENDER32|TEXTURE_D3DPOOL_DEFAULT);

	IDirect3DTexture9* lpTexture=NULL;
	HRESULT hr=gb_RenderDevice3D->lpD3DDevice->CreateTexture(width,height,1,render?D3DUSAGE_RENDERTARGET:0,
		D3DFMT_A32B32G32R32F,render?D3DPOOL_DEFAULT:D3DPOOL_SYSTEMMEM,&lpTexture,NULL);
	if(FAILED(hr))
	{
		RELEASE(Texture);
		VISASSERT(0 && "Cannot create texture");
	}

	Texture->BitMap[0]=lpTexture;

	return Texture;
}


void RasterizeNormals::Render(cObject3dx* obj, int nmaterial)
{
	gb_RenderDevice3D->BeginScene();
	vector<Vect3f> norm;
	vector<Vect2f> uv;
	vector<Mat3f> mats;
	TriangleInfo all;
	obj->GetTriangleInfo(all,TIF_UV|TIF_NORMALS);
	uv.swap(all.uv);
	norm.swap(all.normals);

	obj->GetAllLocalMatrix(mats);
	vector<sPolygon> triangles;
	obj->GetTrianglesByMaterial(triangles,nmaterial);

	cVertexBuffer<sVertexDot3> buf;
	buf.Create(sizeof(sVertexDot3)*4096);
	gb_RenderDevice3D->SetRenderTarget(pRenderTarget,NULL);
	gb_RenderDevice3D->lpD3DDevice->Clear(0,NULL,D3DCLEAR_TARGET,D3DCOLOR_ARGB(0,0,0,255),0,0);

	vsRasterize vsshader;
	psRasterize psshader;
	vsshader.Restore();
	psshader.Restore();

	vsshader.Select();
	psshader.Select();

	sVertexDot3* vertex=buf.Lock();
	int ntriangle=0;
	vector<sPolygon>::iterator it;
	FOR_EACH(triangles,it)
	{
		int idx=ntriangle*3;
		for(int i=0;i<3;i++,idx++)
		{
			int triangle_idx= (*it)[i];
			sVertexDot3& v=vertex[idx];
			v.pos.x = uv[triangle_idx].x;
			v.pos.y = uv[triangle_idx].y;
			v.pos.z = 0.5f;
			Vect3f n=norm[triangle_idx];
			n.normalize();
			v.n = n;
			v.GetTexel().set(0,0);
			Mat3f mat=mats[triangle_idx];
			v.S=mat[0];
			v.T=mat[1];
			v.SxT=mat[2];
		}

		ntriangle++;
		if( ntriangle*3>=buf.GetSize()-4)
		{
			buf.Unlock(ntriangle*3);
			buf.DrawPrimitive(PT_TRIANGLELIST,ntriangle);
			vertex=buf.Lock();
			ntriangle=0;
		}
	}

	buf.Unlock(ntriangle*3);
	if(ntriangle)
		buf.DrawPrimitive(PT_TRIANGLELIST,ntriangle);

	gb_RenderDevice3D->RestoreRenderTarget();
	gb_RenderDevice3D->EndScene();
/*
   n=(S,T,SxT)*x
*/
}
