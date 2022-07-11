#include "StdAfxRD.h"

#include "Scene.h"
#include "TileMap.h"

#include "Font.h"

//#include "Silicon.h"
#include "FontInternal.h"

inline void xFormPoint(D3DMATRIX &m,const Vect3f &in,Vect4f &out)
{
	out.x=m._11*in.x+m._21*in.y+m._31*in.z+m._41;
	out.y=m._12*in.x+m._22*in.y+m._32*in.z+m._42;
	out.z=m._13*in.x+m._23*in.y+m._33*in.z+m._43;
	out.w=m._14*in.x+m._24*in.y+m._34*in.z+m._44;
	if(out.w!=0) out.w=1/out.w; else out.w=1e-30f;
	out.x*=out.w; out.y*=out.w; out.z*=out.w;
}

void cD3DRender::SetRenderTarget(cTexture* target,LPDIRECT3DSURFACE9 pZBuffer)
{
	LPDIRECT3DSURFACE9 lpDDSurface=0;
	RDCALL((target->GetDDSurface(0))->GetSurfaceLevel(0,&lpDDSurface));
	SetRenderTarget(lpDDSurface,pZBuffer);

	lpDDSurface->Release();
}

void cD3DRender::SetRenderTarget(IDirect3DSurface9* target,IDirect3DSurface9* pZBuffer)
{
	for( int nPasses=0; nPasses<nSupportTexture; nPasses++ ) 
	{
		lpD3DDevice->SetTexture( nPasses, CurrentTexture[nPasses]=0 );
	}

	LPDIRECT3DSURFACE9 lpDDSurface=0;
	RDCALL(lpD3DDevice->SetRenderTarget(0,target));
	RDCALL(lpD3DDevice->SetDepthStencilSurface(pZBuffer));

	if(!pZBuffer)
	{
		SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
		SetRenderState( D3DRS_ZWRITEENABLE, FALSE ); 
	}else
	{
		SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
		SetRenderState( D3DRS_ZWRITEENABLE, TRUE); 
	}

}

void cD3DRender::SetRenderTarget1(cTexture* target1)
{
	if(!(DeviceCaps.NumSimultaneousRTs>=2))
		return;
	if(target1)
	{
		LPDIRECT3DSURFACE9 lpDDSurface=0;
		RDCALL((target1->GetDDSurface(0))->GetSurfaceLevel(0,&lpDDSurface));
		RDCALL(lpD3DDevice->SetRenderTarget(1,lpDDSurface));
		lpDDSurface->Release();
	}else
	{
		RDCALL(lpD3DDevice->SetRenderTarget(1,NULL));
	}
}


void cD3DRender::RestoreRenderTarget()
{
	RDCALL(lpD3DDevice->SetRenderTarget(0,lpBackBuffer));
	if(DeviceCaps.NumSimultaneousRTs>=2)
		RDCALL(lpD3DDevice->SetRenderTarget(1,NULL));
	RDCALL(lpD3DDevice->SetDepthStencilSurface(lpZBuffer));
	SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	SetRenderState( D3DRS_ZWRITEENABLE, TRUE ); 
}

void cD3DRender::SetDrawNode(cCamera *pDrawNode)
{
	if((DrawNode=pDrawNode)==0||lpD3DDevice==0) return;

	if(DrawNode->GetRenderSurface() || DrawNode->GetRenderTarget())
	{
		LPDIRECT3DSURFACE9 pZBuffer=DrawNode->GetZBuffer();
		if(DrawNode->GetRenderSurface())
			SetRenderTarget(DrawNode->GetRenderSurface(),pZBuffer);
		else
			SetRenderTarget(DrawNode->GetRenderTarget(),pZBuffer);

		DWORD color=0;
		if(pDrawNode->GetAttribute(ATTRCAMERA_SHADOWMAP))
		{
			color=D3DCOLOR_RGBA(0,0,0,255);//test TSM
			//color=D3DCOLOR_RGBA(255,255,255,255);//ƒл€ Radeon 9700
		}else
		{
			color=pDrawNode->GetFoneColor().RGBA();
		}
		if(pDrawNode->GetAttribute(ATTRCAMERA_REFLECTION))
		{
			color&=~0xff000000;
		}

		if(!pDrawNode->GetAttribute(ATTRCAMERA_NOCLEARTARGET))
		{
			if(!pZBuffer)
			{
				RDCALL(lpD3DDevice->Clear(0,NULL,D3DCLEAR_TARGET,color,1,0));
			}else
			{
				RDCALL(lpD3DDevice->Clear(0,NULL,D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 
					pDrawNode->GetAttribute(ATTRCAMERA_ZINVERT)?0:1, 0));
			}
		}
	}
	else
	{
		RestoreRenderTarget();
	}

	if(pDrawNode->GetScene()->GetTileMap())
	{
		cTileMap* pTileMap=pDrawNode->GetScene()->GetTileMap();
		int x=pTileMap->GetTerra()->SizeX();
		int y=pTileMap->GetTerra()->SizeY();
		planar_node_size=*(D3DXVECTOR4*)&pDrawNode->GetScene()->GetPlanarNodeParam();
		planar_node_size.x=-planar_node_size.x*planar_node_size.z;
		planar_node_size.y=-planar_node_size.y*planar_node_size.w;
	}

	SetDrawTransform(pDrawNode);

	{//Ќемного не к месту, зато быстро по скорости, дл€ отражений.
		SetTexture(5,pDrawNode->GetZTexture());
		SetSamplerData(5,sampler_clamp_linear);
	}
}

void cD3DRender::SetDrawTransform(class cCamera *pDrawNode)
{
	DrawNode=pDrawNode;
	RDCALL(lpD3DDevice->SetTransform(D3DTS_PROJECTION,(D3DXMATRIX*)&pDrawNode->matProj));
	RDCALL(lpD3DDevice->SetTransform(D3DTS_VIEW,(D3DXMATRIX*)&pDrawNode->matView));

	Vect2f rendersize=pDrawNode->GetRenderSize();
	xassert(pDrawNode->vp.X>=0 && pDrawNode->vp.X<=rendersize.x);
	xassert(pDrawNode->vp.Y>=0 && pDrawNode->vp.Y<=rendersize.y);
	xassert(pDrawNode->vp.X+pDrawNode->vp.Width>=0 && pDrawNode->vp.X+pDrawNode->vp.Width<=rendersize.x);
	xassert(pDrawNode->vp.Y+pDrawNode->vp.Height>=0 && pDrawNode->vp.Y+pDrawNode->vp.Height<=rendersize.y);

	RDCALL(lpD3DDevice->SetViewport((D3DVIEWPORT9*)&pDrawNode->vp));
	if(pDrawNode->GetAttribute(ATTRCAMERA_REFLECTION)==0)
		SetRenderState(D3DRS_CULLMODE,CurrentCullMode=D3DCULL_CW);	// пр€мое изображение
	else
		SetRenderState(D3DRS_CULLMODE,CurrentCullMode=D3DCULL_CCW);	// отраженное изображение
}

void cD3DRender::DrawBound(const MatXf &Matrix,Vect3f &min,Vect3f &max,bool wireframe,sColor4c diffuse)
{ 
	VISASSERT(DrawNode);
	DWORD d3d_wireframe=GetRenderState(D3DRS_FILLMODE);
	DWORD d3d_zwrite=GetRenderState(D3DRS_FILLMODE);
	SetRenderState( RS_ZWRITEENABLE, FALSE );
	if(wireframe)
		SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);
	SetNoMaterial(ALPHA_BLEND,Matrix);

	cQuadBuffer<sVertexXYZDT1>* buf=GetQuadBufferXYZDT1();
	sVertexXYZDT1 p,*v;
	p.diffuse=diffuse;
	p.GetTexel().set(0,0);
	buf->BeginDraw(Matrix);

	v=buf->Get();//bottom
	p.pos.set(min.x,min.y,min.z);v[0]=p;
	p.pos.set(max.x,min.y,min.z);v[1]=p;
	p.pos.set(min.x,max.y,min.z);v[2]=p;
	p.pos.set(max.x,max.y,min.z);v[3]=p;

	v=buf->Get();//top
	p.pos.set(min.x,min.y,max.z);v[0]=p;
	p.pos.set(min.x,max.y,max.z);v[1]=p;
	p.pos.set(max.x,min.y,max.z);v[2]=p;
	p.pos.set(max.x,max.y,max.z);v[3]=p;

	v=buf->Get();//right
	p.pos.set(max.x,min.y,min.z);v[0]=p;
	p.pos.set(max.x,min.y,max.z);v[1]=p;
	p.pos.set(max.x,max.y,min.z);v[2]=p;
	p.pos.set(max.x,max.y,max.z);v[3]=p;

	v=buf->Get();//left
	p.pos.set(min.x,min.y,min.z);v[0]=p;
	p.pos.set(min.x,max.y,min.z);v[1]=p;
	p.pos.set(min.x,min.y,max.z);v[2]=p;
	p.pos.set(min.x,max.y,max.z);v[3]=p;

	v=buf->Get();//rear
	p.pos.set(min.x,min.y,min.z);v[0]=p;
	p.pos.set(min.x,min.y,max.z);v[1]=p;
	p.pos.set(max.x,min.y,min.z);v[2]=p;
	p.pos.set(max.x,min.y,max.z);v[3]=p;

	v=buf->Get();//front
	p.pos.set(min.x,max.y,min.z);v[0]=p;
	p.pos.set(max.x,max.y,min.z);v[1]=p;
	p.pos.set(min.x,max.y,max.z);v[2]=p;
	p.pos.set(max.x,max.y,max.z);v[3]=p;
	buf->EndDraw();
	
	SetRenderState( D3DRS_FILLMODE,d3d_wireframe);
	SetRenderState( RS_ZWRITEENABLE, d3d_zwrite );
}

float cD3DRender::GetLineLength(const char *string)
{
	MTG();
	return CurrentFont->GetLineLength(string);
}

float cD3DRender::GetFontLength(const char *string)
{ 
	MTG();
	return CurrentFont->GetLength(string);
}

float cD3DRender::GetFontHeight(const char *string)
{ 
	MTG();
	return CurrentFont->GetHeight(string);
}

float cD3DRender::GetCharLength(const char c)
{
	MTG();
	return CurrentFont->GetCharLength(c);
}

void cD3DRender::OutTextRect(int x,int y,const char *string,ALIGN_TEXT align,Vect2f& bmin,Vect2f& bmax)
{
	bmin.set(x,y);
	bmax.set(x,y);
	if(CurrentFont==0)
	{
		VISASSERT(0 && "Font not set");
		return;
	}

	float xOfs=(float)x, yOfs=(float)y;
	const cFontInternal* cf=CurrentFont->GetInternal();
	sColor4c diffuse(0,0,0,0);

	float xSize = CurrentFont->GetScale().x*cf->GetTexture()->GetWidth(),
		ySize = CurrentFont->size(); //CurrentFont->GetScale().y*cf->FontHeight*cf->GetTexture()->GetWidth();

	float v_add=cf->FontHeight+1/(double)(1<<cf->GetTexture()->GetY());
	int nSymbol=0;
	for( const char* str=string; *str; str++, yOfs+=ySize)
	{
		xOfs=(float)x;
		if( align>=0 )
		{
			float StringWidth = GetLineLength( str );
			if( align==0 )
				xOfs -= round(StringWidth*0.5f);
			else
				xOfs -= round(StringWidth);
		}
		for( ; *str!=10 ; str++ )
		{
			ChangeTextColor(str,diffuse);
			if(*str==10)
				break;

			int c=(unsigned char)*str;
			if( !c ) goto LABEL_DRAW;
			if(c<32)continue;
			const Vect3f& size=cf->Font[c];

			bmin.x=min(xOfs,bmin.x);
			bmin.y=min(yOfs,bmin.y);
			xOfs+=xSize*size.z-1;
			bmax.x=max(xOfs,bmax.x);
			bmax.y=max(yOfs+ySize,bmax.y);

			nSymbol++;
		}
	}
LABEL_DRAW:

	return;
}

void cD3DRender::OutText(int x,int y,const char *string,const sColor4f& color,ALIGN_TEXT align,eBlendMode blend_mode)
{
	if(CurrentFont==0)
	{
		VISASSERT(CurrentFont!=0);
		return;
	}
	
	float xOfs=(float)x, yOfs=(float)y;
	sColor4c diffuse(color);
	const cFontInternal* cf=CurrentFont->GetInternal();

	SetTexture(0,CurrentFont->GetTexture());
	SetTextureBase(1,NULL);
	SetTextureBase(2,NULL);
	SetBlendStateAlphaRef(blend_mode);

	SetRenderState(D3DRS_SPECULARENABLE,FALSE);
	SetRenderState(D3DRS_NORMALIZENORMALS,FALSE);

	psFont->Select();

	if (ABS(CurrentFont->GetScale().x - 1.0f) < 0.01f ||
		ABS(CurrentFont->GetScale().y - 1.0f) < 0.01f)
		SetSamplerData(0,sampler_clamp_point);
	else
		SetSamplerData(0,sampler_clamp_linear);
	

	float xSize = CurrentFont->GetScale().x*cf->GetTexture()->GetWidth(),
		  ySize = CurrentFont->size();

	float v_add=cf->FontHeight+1/(double)(1<<cf->GetTexture()->GetY());

	QuadBufferXYZWDT1.BeginDraw();

	for( const char* str=string; *str; str++, yOfs+=ySize)
	{
		xOfs=(float)x;
		if( align>=0 )
		{
			float StringWidth = GetLineLength( str );
			if( align==0 )
				xOfs -= round(StringWidth*0.5f);
			else
				xOfs -= round(StringWidth);
		}
		for( ; *str!=10 ; str++ )
		{
			ChangeTextColor(str,diffuse);
			if(*str==10)
				break;

			int c=(unsigned char)*str;
			if( !c ) goto LABEL_DRAW;
			if(c<32)continue;
			const Vect3f& size=cf->Font[c];

			sVertexXYZWDT1* v=QuadBufferXYZWDT1.Get();
			sVertexXYZWDT1 &v1=v[1],&v2=v[0],
						   &v3=v[2],&v4=v[3];

			v1.x=v4.x=xOfs;
			v1.y=v2.y=yOfs;
			xOfs+=xSize*size.z-1;
			v3.x=v2.x=xOfs;
			v3.y=v4.y=yOfs+ySize;
			v1.u1()=v4.u1()=size.x;
			v1.v1()=v2.v1()=size.y;
			v3.u1()=v2.u1()=v1.u1()+size.z;
			v3.v1()=v4.v1()=v1.v1()+v_add;
			v1.diffuse=v2.diffuse=v3.diffuse=v4.diffuse=diffuse;
			v1.z=v2.z=v3.z=v4.z=0.001f;
			v1.w=v2.w=v3.w=v4.w=0.001f;
		}
	}
LABEL_DRAW:

	QuadBufferXYZWDT1.EndDraw();
}

int cD3DRender::OutTextLine(int x, int y, const char *textline, const char* end, const sColor4c& color, eBlendMode blend_mode, int xRangeMin, int xRangeMax)
{
	VISASSERT(CurrentFont != 0);

	float xOfs = (float)x;
	float yOfs = (float)y;
	const cFontInternal* cf = CurrentFont->GetInternal();

	SetTexture(0, CurrentFont->GetTexture());
	SetTextureBase(1, NULL);
	SetTextureBase(2, NULL);
	SetBlendStateAlphaRef(blend_mode);

	SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);

	psFont->Select();

	if (ABS(CurrentFont->GetScale().x - 1.0f) < 0.01f ||
		ABS(CurrentFont->GetScale().y - 1.0f) < 0.01f)
		SetSamplerData(0, sampler_clamp_point);
	else
		SetSamplerData(0, sampler_clamp_linear);

	float xSize = CurrentFont->GetScale().x * cf->GetTexture()->GetWidth();
	float yOfs2 = yOfs + CurrentFont->size();
	float v_add = cf->FontHeight + 1. / (double)(1 << cf->GetTexture()->GetY());

	QuadBufferXYZWDT1.BeginDraw();

	BYTE symbol;
	for(const char* str = textline; str != end; ++str)
	{
		symbol = (BYTE)*str;

		if(symbol < 32)
			continue;
		
		const Vect3f& size = cf->Font[symbol];

		if(xRangeMin >= 0 && round(xOfs) < xRangeMin){
			xOfs += xSize * size.z - 1.f;
			continue;
		}
		
		if(xRangeMax >= 0 && round(xOfs + xSize * size.z) > xRangeMax)
			break;

		sVertexXYZWDT1* v = QuadBufferXYZWDT1.Get();
		sVertexXYZWDT1 &v1 = v[1], &v2 = v[0], &v3 = v[2], &v4 = v[3];

		v1.x=v4.x = xOfs;
		v1.y=v2.y = yOfs;
		
		xOfs += xSize * size.z - 1.f;
		
		v3.x=v2.x = xOfs;
		v3.y=v4.y = yOfs2;
		v1.u1()=v4.u1() = size.x;
		v1.v1()=v2.v1() = size.y;
		v3.u1()=v2.u1() = v1.u1() + size.z;
		v3.v1()=v4.v1() = v1.v1() + v_add;
		v1.diffuse=v2.diffuse=v3.diffuse=v4.diffuse = color;
		v1.z=v2.z=v3.z=v4.z = 0.001f;
		v1.w=v2.w=v3.w=v4.w = 0.001f;
	}

	QuadBufferXYZWDT1.EndDraw();
	return round(xOfs);
}

void cD3DRender::OutText(int x,int y,const char *string,const sColor4f& color,ALIGN_TEXT align,eBlendMode blend_mode,
			cTexture* pTexture,eColorMode mode,Vect2f uv,Vect2f duv,float phase,float lerp_factor)
{
	VISASSERT(pTexture);
	if(CurrentFont==0)
	{
		VISASSERT(CurrentFont!=0);
		return;
	}
	
	duv.x*=1024.0f/GetSizeX();
	duv.y*=768.0f/GetSizeY();
	float xOfs=(float)x, yOfs=(float)y;
	sColor4c diffuse(color);
	sColor4c lerp(255*lerp_factor,255*lerp_factor,255*lerp_factor,255*(1-lerp_factor));
	const cFontInternal* cf=CurrentFont->GetInternal();

//	SetNoMaterial(blend_mode,phase,CurrentFont->GetTexture(),pTexture,mode);
	SetNoMaterial(blend_mode,MatXf::ID,phase,pTexture,CurrentFont->GetTexture(),mode);
//	SetNoMaterial(blend_mode,phase,pTexture);

	DWORD index1=GetTextureStageState(1,D3DTSS_TEXCOORDINDEX);
	SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,1);
	SetTextureStageState(1,D3DTSS_TEXCOORDINDEX,0);
	SetSamplerData(0,sampler_clamp_point);

	{
		SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATECOLOR_ADDALPHA);
		SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TFACTOR);
		SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_TEXTURE);
		SetRenderState(D3DRS_TEXTUREFACTOR,lerp.RGBA());

		SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
		SetTextureStageState(1,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
		SetTextureStageState(1,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);

//		SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_MODULATE);
		SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
		SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_CURRENT);
//	   DWORD dwNumPasses;
//	   RDCALL(lpD3DDevice->ValidateDevice( &dwNumPasses ));
	}


	float xSize = CurrentFont->GetScale().x*cf->GetTexture()->GetWidth(),
		  ySize = CurrentFont->size(); //CurrentFont->GetScale().y*cf->FontHeight*cf->GetTexture()->GetHeight();

	float v_add=cf->FontHeight+1/(double)(1<<cf->GetTexture()->GetY());

	QuadBufferXYZWDT2.BeginDraw();

	for( const char* str=string; *str; str++, yOfs+=ySize)
	{
		xOfs=(float)x;
		if( align>=0 )
		{
			float StringWidth = GetLineLength( str );
			if( align==0 )
				xOfs -= round(StringWidth*0.5f);
			else
				xOfs -= round(StringWidth);
		}
		for( ; *str!=10 ; str++ )
		{
			ChangeTextColor(str,diffuse);

			int c=(unsigned char)*str;
			if( !c ) goto LABEL_DRAW;
			if(c==10)break;
			if(c<32)continue;
			const Vect3f& size=cf->Font[c];

			sVertexXYZWDT2* v=QuadBufferXYZWDT2.Get();
			sVertexXYZWDT2 &v1=v[1],&v2=v[0],
						   &v3=v[2],&v4=v[3];

			float x0,x1,y0,y1;
			x0=xOfs;
			x1=xOfs+xSize*size.z-1;
			y0=yOfs;
			y1=yOfs+ySize;
			v1.x=v4.x=x0;
			v1.y=v2.y=y0;
			v3.x=v2.x=x1;
			v3.y=v4.y=y1;
			v1.u1()=v4.u1()=size.x;
			v1.v1()=v2.v1()=size.y;
			v3.u1()=v2.u1()=v1.u1()+size.z;
			v3.v1()=v4.v1()=v1.v1()+v_add;
			v1.diffuse=v2.diffuse=v3.diffuse=v4.diffuse=diffuse;
			v1.z=v2.z=v3.z=v4.z=0.001f;
			v1.w=v2.w=v3.w=v4.w=0.001f;

			v1.u2()=v4.u2()=(x0-x)*duv.x+uv.x;
			v1.v2()=v2.v2()=(y0-y)*duv.y+uv.y;
			v3.u2()=v2.u2()=(x1-x)*duv.x+uv.x;
			v3.v2()=v4.v2()=(y1-y)*duv.y+uv.y;

			xOfs=x1;
		}
	}
LABEL_DRAW:

	QuadBufferXYZWDT2.EndDraw();
	SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);
	SetTextureStageState(1,D3DTSS_TEXCOORDINDEX,index1);

	SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

	SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

	SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );

	SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	SetTextureStageState( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
}

struct LightByTexture
{
	cTexture* texture;
	vector<cUnkLight*> addBlendingLight;
	vector<cUnkLight*> subBlendingLight;
};

void Rotate2D(Mat3f& out,const Mat3f& in)
{
	float cos=0;
	float sin;
	if(in[1][1] > FLT_EPS)
	{
		float tan = in[0][1]/in[1][1];
		cos = 1/sqrtf(tan*tan+1);
		sin = cos*tan;
	}else
	{
		sin = in[0][1]>0?1:-1;
	}
	out.xx=cos; out.xy=-sin; out.xz=0;
	out.yx=sin; out.yy=cos; out.yz=0;
	out.zx=0; out.zy=0; out.zz=1;
}

void cD3DRender::Draw(class cScene *Scene, bool objects)
{ 
	DWORD old_colorwrite=gb_RenderDevice3D->GetRenderState(D3DRS_COLORWRITEENABLE);
	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
	int i;
	cQuadBuffer<sVertexXYZDT1>* quad=GetQuadBufferXYZDT1();
	SetVertexShader(NULL);
	SetPixelShader(NULL);
	int size=Scene->circle_shadow.size();
	if(size)
	{
		if(!bSphereShadowLoaded)
		{
			//pSphereShadow=GetTexLibrary()->GetElement3D("Scripts\\Resource\\balmer\\lightmap.tga");
			pSphereShadow=GetTexLibrary()->GetSpericalTexture();
			bSphereShadowLoaded=true;
		}

		if(!objects)
		{
			SetNoMaterial(ALPHA_BLEND,MatXf::ID);
			SetTexture(0,pSphereShadow);
			SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);

			quad->BeginDraw();
			for(i=0;i<size;i++)
			{
				cScene::CircleShadow& c=Scene->circle_shadow[i];
				sVertexXYZDT1 *v=quad->Get();
				Vect3f& p=c.pos;
				float r=c.radius;
				sColor4c Diffuse=c.color;//”читываем, что 0.5 - это нормальна€ €ркость.
				Diffuse.r=Diffuse.r>>1;
				Diffuse.g=Diffuse.g>>1;
				Diffuse.b=Diffuse.b>>1;
				v[0].pos.x=p.x-r; v[0].pos.y=p.y-r; v[0].pos.z=p.z; v[0].u1()=0; v[0].v1()=0;v[0].diffuse=Diffuse;
				v[1].pos.x=p.x-r; v[1].pos.y=p.y+r; v[1].pos.z=p.z; v[1].u1()=0; v[1].v1()=1;v[1].diffuse=Diffuse;
				v[2].pos.x=p.x+r; v[2].pos.y=p.y-r; v[2].pos.z=p.z; v[2].u1()=1; v[2].v1()=0;v[2].diffuse=Diffuse;
				v[3].pos.x=p.x+r; v[3].pos.y=p.y+r; v[3].pos.z=p.z; v[3].u1()=1; v[3].v1()=1;v[3].diffuse=Diffuse;
			}
			quad->EndDraw();
		}
	}

	vector<LightByTexture> light;
	for(i=0;i<Scene->GetNumberLight();i++)
	{
		cUnkLight* ULight=Scene->GetLight(i);
		if(ULight && !ULight->GetAttr(ATTRLIGHT_IGNORE)&& ((!objects && ULight->GetAttr(ATTRLIGHT_SPHERICAL_TERRAIN)) || 
			(objects && ULight->GetAttr(ATTRLIGHT_SPHERICAL_OBJECT))))
		{
			cTexture* pTexture=ULight->GetTexture();

			LightByTexture* pl=NULL;
			for(int j=0;j<light.size();j++)
			if(light[j].texture==pTexture)
			{
				pl=&light[j];
				break;
			}
			
			if(!pl)
			{
				light.push_back(LightByTexture());
				pl=&light.back();
				pl->texture=pTexture;
			}

			if (ULight->GetBlending() == ALPHA_ADDBLEND)
				pl->addBlendingLight.push_back(ULight);
			else
				pl->subBlendingLight.push_back(ULight);
		}
	}

	if(!light.empty())
	{
		//SetNoMaterial(ALPHA_ADDBLEND);

		for(i=0;i<light.size();i++)
		{
			LightByTexture& pl=light[i];
			if(!pl.subBlendingLight.empty())
			{
				SetNoMaterial(ALPHA_SUBBLEND,MatXf::ID,0,pl.texture);
				quad->BeginDraw();
				for(int j=0;j<pl.subBlendingLight.size();j++)
				{
					cUnkLight* ULight=pl.subBlendingLight[j];
					//sColor4c Diffuse(ULight->GetDiffuse()); 
					sColor4c Diffuse(
						round(ULight->GetDiffuse().a*ULight->GetDiffuse().r*255),
						round(ULight->GetDiffuse().a*ULight->GetDiffuse().g*255),
						round(ULight->GetDiffuse().a*ULight->GetDiffuse().b*255),255);

					sVertexXYZDT1 *v=quad->Get();
					Vect3f& p=ULight->GetPos();
					float r=ULight->GetRadius();
					Vect3f sx,sy;

					Mat3f mat;
					Rotate2D(mat,ULight->GetGlobalMatrix().rot());
					mat.invXform(Vect3f(r,0,0),sx);
					mat.invXform(Vect3f(0,r,0),sy);
					v[0].pos.x=p.x-sx.x-sy.x; v[0].pos.y=p.y-sx.y-sy.y; v[0].pos.z=p.z; v[0].u1()=0; v[0].v1()=0;v[0].diffuse=Diffuse;
					v[1].pos.x=p.x-sx.x+sy.x; v[1].pos.y=p.y-sx.y+sy.y; v[1].pos.z=p.z; v[1].u1()=0; v[1].v1()=1;v[1].diffuse=Diffuse;
					v[2].pos.x=p.x+sx.x-sy.x; v[2].pos.y=p.y+sx.y-sy.y; v[2].pos.z=p.z; v[2].u1()=1; v[2].v1()=0;v[2].diffuse=Diffuse;
					v[3].pos.x=p.x+sx.x+sy.x; v[3].pos.y=p.y+sx.y+sy.y; v[3].pos.z=p.z; v[3].u1()=1; v[3].v1()=1;v[3].diffuse=Diffuse;

					//v[0].pos.x=p.x-r; v[0].pos.y=p.y-r; v[0].pos.z=p.z; v[0].u1()=0; v[0].v1()=0;v[0].diffuse=Diffuse;
					//v[1].pos.x=p.x-r; v[1].pos.y=p.y+r; v[1].pos.z=p.z; v[1].u1()=0; v[1].v1()=1;v[1].diffuse=Diffuse;
					//v[2].pos.x=p.x+r; v[2].pos.y=p.y-r; v[2].pos.z=p.z; v[2].u1()=1; v[2].v1()=0;v[2].diffuse=Diffuse;
					//v[3].pos.x=p.x+r; v[3].pos.y=p.y+r; v[3].pos.z=p.z; v[3].u1()=1; v[3].v1()=1;v[3].diffuse=Diffuse;
				}
				quad->EndDraw();
			}

			if(!pl.addBlendingLight.empty())
			{
				SetNoMaterial(ALPHA_ADDBLEND,MatXf::ID,0,pl.texture);
				quad->BeginDraw();
				for(int j=0;j<pl.addBlendingLight.size();j++)
				{
					cUnkLight* ULight=pl.addBlendingLight[j];
					//sColor4c Diffuse(ULight->GetDiffuse()); 
					sColor4c Diffuse(round(ULight->GetDiffuse().a*ULight->GetDiffuse().r*255),
						round(ULight->GetDiffuse().a*ULight->GetDiffuse().g*255),
						round(ULight->GetDiffuse().a*ULight->GetDiffuse().b*255),255);
					sVertexXYZDT1 *v=quad->Get();
					Vect3f& p=ULight->GetPos();
					float r=ULight->GetRadius();
					Vect3f sx,sy;
					Mat3f mat;
					Rotate2D(mat,ULight->GetGlobalMatrix().rot());
 					mat.invXform(Vect3f(r,0,0),sx);
					mat.invXform(Vect3f(0,r,0),sy);
					v[0].pos.x=p.x-sx.x-sy.x; v[0].pos.y=p.y-sx.y-sy.y; v[0].pos.z=p.z; v[0].u1()=0; v[0].v1()=0;v[0].diffuse=Diffuse;
					v[1].pos.x=p.x-sx.x+sy.x; v[1].pos.y=p.y-sx.y+sy.y; v[1].pos.z=p.z; v[1].u1()=0; v[1].v1()=1;v[1].diffuse=Diffuse;
					v[2].pos.x=p.x+sx.x-sy.x; v[2].pos.y=p.y+sx.y-sy.y; v[2].pos.z=p.z; v[2].u1()=1; v[2].v1()=0;v[2].diffuse=Diffuse;
					v[3].pos.x=p.x+sx.x+sy.x; v[3].pos.y=p.y+sx.y+sy.y; v[3].pos.z=p.z; v[3].u1()=1; v[3].v1()=1;v[3].diffuse=Diffuse;

					//v[0].pos.x=p.x-r; v[0].pos.y=p.y-r; v[0].pos.z=p.z; v[0].u1()=0; v[0].v1()=0;v[0].diffuse=Diffuse;
					//v[1].pos.x=p.x-r; v[1].pos.y=p.y+r; v[1].pos.z=p.z; v[1].u1()=0; v[1].v1()=1;v[1].diffuse=Diffuse;
					//v[2].pos.x=p.x+r; v[2].pos.y=p.y-r; v[2].pos.z=p.z; v[2].u1()=1; v[2].v1()=0;v[2].diffuse=Diffuse;
					//v[3].pos.x=p.x+r; v[3].pos.y=p.y+r; v[3].pos.z=p.z; v[3].u1()=1; v[3].v1()=1;v[3].diffuse=Diffuse;
				}
				quad->EndDraw();
			}
		}
	}

	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_colorwrite);
}
