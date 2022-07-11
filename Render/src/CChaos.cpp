#include "StdAfxRD.h"
#include "cChaos.h"
#include "..\Render\VertexDeclarationMacro.h"

int cChaos::count_chaos_struct = 0;

cChaos::cChaos(Vect2f g_size,LPCSTR str_tex0,LPCSTR str_tex1,LPCSTR str_bump,int tile,bool enablebump_)
:cIUnkObj(KIND_NULL)
{
	plane_size=g_size;
	pTex0=NULL;
	time=0;

	enablebump=BUMP_NONE;
	pVS=NULL;
	pPS=NULL;

	if(enablebump_)
	{
		if(gb_RenderDevice3D->DeviceCaps.PixelShaderVersion>= D3DPS_VERSION(2,0))
			enablebump=BUMP_PS14;
		else
		if(gb_RenderDevice3D->DeviceCaps.TextureOpCaps|D3DTEXOPCAPS_BUMPENVMAP)
			enablebump=BUMP_RENDERTARGET;
	}

	stime_tex0.x=stime_tex0.y=0.005f;
	stime_tex1.x=0;
	stime_tex1.y=-0.03f;
	uvmul=2;

	pTex0=NULL;
	pTexRender=NULL;
	pTexBump=NULL;
	if(enablebump==BUMP_PS14)
	{
		pVS=new VSChaos;
		pVS->Restore();
		pPS=new PSChaos;
		pPS->Restore();
		pTexBump=GetTexLibrary()->GetElement2D(str_bump);
	}

	if(enablebump==BUMP_RENDERTARGET)
	{
		pTex0=gb_VisGeneric->CreateRenderTexture(512,512,TEXTURE_RENDER32,false);
		pTexRender=gb_VisGeneric->CreateRenderTexture(512,512,TEXTURE_RENDER32,false);
		//pTexBump=gb_VisGeneric->CreateTexture(str_bump,"UVBump");
		pTexBump=GetTexLibrary()->GetElement2D(str_bump);

		if(pTex0==NULL || pTexRender==NULL)
		{
			enablebump=BUMP_NONE;
			RELEASE(pTex0);
			RELEASE(pTexRender);
		}
	}


	pTex0_0=gb_VisGeneric->CreateTexture(str_tex0);//"RESOURCE\\EFFECT\\WorldGround.tga");
	pTex0_1=gb_VisGeneric->CreateTexture(str_tex1);//"RESOURCE\\EFFECT\\WorldGround01.tga");


	size=tile*sub_div;
	CreateVB();
	CreateIB();
	xassert(count_chaos_struct==0);
	count_chaos_struct++;
}

cChaos::~cChaos()
{
	delete pVS;
	delete pPS;
	gb_RenderDevice3D->DeleteIndexBuffer(ib);
	gb_RenderDevice3D->DeleteVertexBuffer(vb);
	RELEASE(pTex0);
	RELEASE(pTexRender);
	RELEASE(pTexBump);

	RELEASE(pTex0_0);
	RELEASE(pTex0_1);
	count_chaos_struct--;
	xassert(count_chaos_struct==0);
}

void cChaos::SetTextures(LPCSTR str_tex0,LPCSTR str_tex1,LPCSTR str_bump)
{
	if (!pTex0_0 || strcmp(pTex0_0->GetName(), str_tex0)!=0)
	{
		RELEASE(pTex0_0);
		pTex0_0=gb_VisGeneric->CreateTexture(str_tex0);
	}
	if (!pTex0_1 || strcmp(pTex0_1->GetName(), str_tex1)!=0)
	{
		RELEASE(pTex0_0);
		pTex0_1=gb_VisGeneric->CreateTexture(str_tex1);
	}
	if (!pTexBump || strcmp(pTexBump->GetName(), str_bump)!=0)
	{
		RELEASE(pTexBump);
		pTexBump = gb_VisGeneric->CreateTexture(str_bump);
	}
}

void cChaos::Animate(float dt)
{
	time+=dt*1e-3f;
}

void cChaos::RenderAllTexture()
{
	if(enablebump==BUMP_RENDERTARGET)
	{
		cD3DRender* rd=gb_RenderDevice3D;

		BOOL fog=rd->GetRenderState(D3DRS_FOGENABLE);
		rd->SetRenderState(D3DRS_FOGENABLE, FALSE);
		DWORD zenable=rd->GetRenderState(D3DRS_ZENABLE);
		DWORD zwriteenable=rd->GetRenderState(D3DRS_ZWRITEENABLE); 
		RenderTexture();
		RenderTex0();
		rd->SetRenderState(D3DRS_FOGENABLE, fog);
		rd->SetRenderState( D3DRS_ZENABLE, zenable);
		rd->SetRenderState( D3DRS_ZWRITEENABLE, zwriteenable); 
	}
}

void cChaos::PreDraw(cCamera *pCamera)
{
	pCamera->AttachNoRecursive(SCENENODE_OBJECTSPECIAL,this);
}

void cChaos::Draw(cCamera *DrawNode)
{
	cD3DRender* rd=gb_RenderDevice3D;
	BOOL fog=rd->GetRenderState(D3DRS_FOGENABLE);
//	rd->SetRenderState(D3DRS_FOGENABLE, FALSE);
/*
		rd->SetRenderState(D3DRS_FOGENABLE, FALSE);
		rd->DrawSprite(0,0,512,512,
						0,0,1,1,pTexBump);
		rd->SetRenderState(D3DRS_FOGENABLE, TRUE);
*/
	if(false)
	{
		rd->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,pTexBump);
	}else
	if(enablebump==BUMP_PS14)
	{
		for(int ss=0;ss<4;ss++)
		{
			rd->SetSamplerData(ss,sampler_wrap_anisotropic);
		}

		rd->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,pTex0_0,pTex0_1,COLOR_ADD);
		rd->SetTexture(2,pTexBump);
		rd->SetTextureBase(3,NULL);
		float umin,vmin;
		float umin2,vmin2;
		umin=sfmod(time*stime_tex0.x*uvmul*2.0f,1.0f);
		vmin=sfmod(time*stime_tex0.y*uvmul*2.0f,1.0f);
		umin2=sfmod(time*stime_tex1.x*uvmul*0.5f,1.0f);
		vmin2=sfmod(time*stime_tex1.y*uvmul*0.5f,1.0f);

		float umin_b0,vmin_b0,umin_b1,vmin_b1;
		umin_b0=sfmod(time*stime_tex0.x,1.0f);
		vmin_b0=sfmod(time*stime_tex0.y,1.0f);

		umin_b1=sfmod(time*stime_tex1.y*0.2f,1.0f);
		vmin_b1=sfmod(time*stime_tex1.x*0.2f,1.0f);
		pVS->Select(umin,vmin,umin2,vmin2,umin_b0,vmin_b0,umin_b1,vmin_b1);
		pPS->Select();
	}else
	if(enablebump==BUMP_RENDERTARGET)
	{
		//rd->SetDrawTransform(DrawNode);
		//rd->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);

		for(int ss=0;ss<4;ss++)
		{
			gb_RenderDevice3D->SetSamplerData(ss,sampler_wrap_anisotropic);
		}

		if(true)
		{//bump
			rd->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,pTexRender,pTex0);//FIXME

			rd->SetTextureStageState( 0, D3DTSS_BUMPENVMAT00, F2DW(0.05f) );
			rd->SetTextureStageState( 0, D3DTSS_BUMPENVMAT01, F2DW(0.00f) );
			rd->SetTextureStageState( 0, D3DTSS_BUMPENVMAT10, F2DW(0.00f) );
			rd->SetTextureStageState( 0, D3DTSS_BUMPENVMAT11, F2DW(0.05f) );

			rd->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0);
			rd->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_BUMPENVMAP );
			rd->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			rd->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_CURRENT );
			rd->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

			rd->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 1);
			rd->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
			rd->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			rd->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
			rd->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

			rd->SetTextureStageState( 2, D3DTSS_COLOROP,   D3DTOP_DISABLE );
			rd->SetTextureStageState( 2, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
		}else
		{
			rd->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,pTex0);
//			rd->DrawSprite(0,0,512,512,
//							0,0,1,1,pTexRender);
		}
	}else
	{
		rd->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,pTex0_0,pTex0_1,COLOR_ADD);

		rd->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0);
		rd->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 1);
		float umin,vmin;
		D3DXMATRIX mat;

		umin=sfmod(time*stime_tex0.x*uvmul,1.0f);
		vmin=sfmod(time*stime_tex0.y*uvmul,1.0f);
		memset(mat,0,sizeof(mat));
		mat._11=mat._22=mat._33=mat._44=1;
		mat._31 = umin;
		mat._32 = vmin;
		RDCALL(rd->lpD3DDevice->SetTransform(D3DTS_TEXTURE0,&mat));
		rd->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

		umin=sfmod(time*stime_tex1.x*uvmul,1.0f);
		vmin=sfmod(time*stime_tex1.y*uvmul,1.0f);
		memset(mat,0,sizeof(mat));
		mat._11=mat._22=mat._33=mat._44=1;
		mat._31 = umin;
		mat._32 = vmin;
		RDCALL(rd->lpD3DDevice->SetTransform(D3DTS_TEXTURE1,&mat));
		rd->SetTextureStageState( 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

	}

	float woldsize=max(plane_size.x,plane_size.y);
	DWORD fogcolor=rd->GetRenderState(D3DRS_FOGCOLOR);
	DWORD fogstart=rd->GetRenderState(D3DRS_FOGSTART);
	DWORD fogend=rd->GetRenderState(D3DRS_FOGEND);
	DWORD tablemode=rd->GetRenderState(D3DRS_FOGTABLEMODE);
	DWORD vertexmode=rd->GetRenderState( D3DRS_FOGVERTEXMODE);

//	rd->SetRenderState(D3DRS_FOGENABLE, TRUE);

//	rd->SetRenderState(D3DRS_FOGCOLOR, 0);
//	rd->SetRenderState(D3DRS_FOGSTART, F2DW(woldsize*0.5f));
//	rd->SetRenderState(D3DRS_FOGEND,   F2DW(woldsize*1.5f));
//	rd->SetRenderState( D3DRS_FOGTABLEMODE,  D3DFOG_NONE ),
//	rd->SetRenderState( D3DRS_FOGVERTEXMODE,  D3DFOG_LINEAR );

	//rd->SetMatrix(D3DTS_WORLD,GetGlobalMatrix());

	rd->DrawIndexedPrimitive(vb,0,(size+1)*(size+1), ib,0,size*size*2);

	rd->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	rd->SetTextureStageState( 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	rd->SetTextureStageState( 3, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	rd->SetTextureStageState( 3, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	rd->SetRenderState(D3DRS_FOGENABLE,fog);
	rd->SetRenderState(D3DRS_FOGCOLOR, fogcolor);
	rd->SetRenderState(D3DRS_FOGSTART, fogstart);
	rd->SetRenderState(D3DRS_FOGEND,   fogend);
	rd->SetRenderState(D3DRS_FOGTABLEMODE,tablemode);
	rd->SetRenderState(D3DRS_FOGVERTEXMODE,vertexmode);

	rd->SetTextureBase(2,NULL);
	rd->SetTextureBase(3,NULL);
	rd->SetVertexShader(NULL);
	rd->SetPixelShader(NULL);
}

void cChaos::CreateIB()
{
	gb_RenderDevice3D->CreateIndexBuffer(ib,size*size*2);
	sPolygon* p=gb_RenderDevice3D->LockIndexBuffer(ib);

	int vbwidth=size+1;
	for(int y=0;y<size;y++)
	for(int x=0;x<size;x++)
	{
		int i0=x+y*vbwidth;
		p->p1=i0;
		p->p2=i0+vbwidth;
		p->p3=i0+1;
		p++;
		p->p1=i0+1;
		p->p2=i0+vbwidth;
		p->p3=i0+vbwidth+1;
		p++;
	}
	
	gb_RenderDevice3D->UnlockIndexBuffer(ib);
}

void cChaos::CreateVB()
{
	gb_RenderDevice3D->CreateVertexBuffer(vb,(size+1)*(size+1),VTYPE::declaration,false);

	VTYPE* pVertex=(VTYPE*)gb_RenderDevice3D->LockVertexBuffer(vb);
	xassert(sizeof(VTYPE)==vb.GetVertexSize());

	int smin=-(size/sub_div/2),smax=size/sub_div+smin;
	int xmin=smin*plane_size.x,xmax=smax*plane_size.x,
		ymin=smin*plane_size.y,ymax=smax*plane_size.y;
	float deltax=(xmax-xmin)/(float)size;
	float deltay=(ymax-ymin)/(float)size;
	float du,dv;
	du=dv=1.0f/sub_div;
	
	Vect3f n(0,0,1),t(1,0,0),b;
	b.cross(n,t);

	if(enablebump==BUMP_RENDERTARGET)
	{
		for(int iy=0;iy<=size;iy++)
		{
			VTYPE* vout=pVertex+iy*(size+1);

			for(int ix=0;ix<=size;ix++,vout++)
			{
				vout->pos.x=ix*deltax+xmin;
				vout->pos.y=iy*deltay+ymin;
				vout->pos.z=0;

				vout->diffuse.RGBA()=0xFFFFFFFF;
				vout->u1()=ix*du;
				vout->v1()=iy*dv;
				vout->u2()=ix*du*uvmul;
				vout->v2()=iy*dv*uvmul;
			}
		}
	}else
	{
		for(int iy=0;iy<=size;iy++)
		{
			VTYPE* vout=pVertex+iy*(size+1);
			for(int ix=0;ix<=size;ix++,vout++)
			{
				vout->pos.x=ix*deltax+xmin;
				vout->pos.y=iy*deltay+ymin;
				vout->pos.z=0;
				vout->diffuse.RGBA()=0xFFFFFFFF;
				vout->u1()=ix*du*uvmul;
				vout->v1()=iy*dv*uvmul;
				vout->u2()=ix*du*uvmul;
				vout->v2()=iy*dv*uvmul;
			}
		}
	}
	VTYPE* vout=pVertex;
	float radius = (xmax-xmin)/2;
	Vect3f center((xmax+xmin)/2, (ymax+ymin)/2, 0);
	for(int iy=0;iy<=size;iy++)
	for(int ix=0;ix<=size;ix++,vout++)
	{
		Vect3f dp = vout->pos - center;
		if (dp.norm2()>sqr(radius))
		{
			dp.Normalize(radius);
			vout->pos = center + dp;
		}
	}
	gb_RenderDevice3D->UnlockVertexBuffer(vb);
}

void cChaos::InitBumpTexture1(cTexture* pTex)
{
	int Pitch;
	int dx=pTex->GetWidth(),dy=pTex->GetHeight();
	BYTE* pBuffer=(BYTE*)pTex->LockTexture(Pitch);

	float cx=64.0f/dx,cy=64.0f/dy;
	float cx1=60.0f/dx,cy1=60.0f/dy;

	struct VU
	{
		BYTE v,u;
	};

	VISASSERT(sizeof(VU)==2);

	for(int iy=0;iy<dy;iy++)
	{
		VU* p=(VU*)pBuffer;

		for(int ix=0;ix<dx;ix++,p++)
		{
			float du,dv;
			du=0;//cos((ix+iy)*cx*M_PI)+cos((ix-iy)*cx1*M_PI);
			dv=0;//sin((ix+iy)*cy*M_PI)+sin((ix-iy)*cy1*M_PI);
			
			p->u=round(du*16);
			p->v=round(dv*16);
		}

		pBuffer+=Pitch;
	}

	pTex->UnlockTexture();
}

void cChaos::InitBumpTexture2(cTexture* pTex)
{
	int Pitch;
	int dx=pTex->GetWidth(),dy=pTex->GetHeight();
	BYTE* pBuffer=pTex->LockTexture(Pitch);

	float cx=32.0f/dx,cy=32.0f/dy;

	struct VU
	{
		char v,u;
	};

	VISASSERT(sizeof(VU)==2);

	for(int iy=0;iy<dy;iy++)
	{
		VU* p=(VU*)pBuffer;

		for(int ix=0;ix<dx;ix++,p++)
		{
			float du,dv;
			du=cos(iy*cx*M_PI)*0.9f;
			dv=sin(iy*cy*M_PI)*0.9f;
			
			p->u=round((du+1)*64);
			p->v=round((dv+1)*64);
		}

		pBuffer+=Pitch;
	}

	pTex->UnlockTexture();
}

void cChaos::ConvertToBump(cTexture*& pOut,cTexture* pIn)
{
	int dx=pIn->GetWidth(),dy=pIn->GetHeight();
	pOut=gb_VisGeneric->CreateBumpTexture(dx,dy);

	int PitchIn,PitchOut;
	BYTE* pBufferIn=pIn->LockTexture(PitchIn);
	BYTE* pBufferOut=pOut->LockTexture(PitchOut);

	for(int iy=0;iy<dy;iy++)
	{
		BYTE* pi=pBufferIn;
		BYTE* po=pBufferOut;

		for(int ix=0;ix<dx;ix++)
		{
			po[0]=pi[0]>>1;
			po[1]=pi[1]>>1;
			pi+=4;
			po+=2;
		}

		pBufferIn+=PitchIn;
		pBufferOut+=PitchOut;
	}

	pIn->UnlockTexture();
	pOut->UnlockTexture();
}

void cChaos::RenderTexture()
{
	cD3DRender* rd=gb_RenderDevice3D;
	
	rd->SetRenderTarget(pTexRender,NULL);

	float umin,vmin,umin1,vmin1;
	umin=sfmod(time*stime_tex0.x,1.0f);
	vmin=sfmod(time*stime_tex0.y,1.0f);

	umin1=sfmod(time*stime_tex1.x,1.0f);
	vmin1=sfmod(time*stime_tex1.y,1.0f);

	rd->DrawSprite2(0,0,pTexRender->GetWidth(),pTexRender->GetHeight(),
					umin,vmin,1,1,
					umin1,vmin1,1,1,
					pTexBump,pTexBump,sColor4c(255,255,255,255),0,COLOR_ADD);
	rd->RestoreRenderTarget();
}

void cChaos::RenderTex0()
{
	cD3DRender* rd=gb_RenderDevice3D;

	rd->SetRenderTarget(pTex0,NULL);

	float umin,vmin,umin1,vmin1;
	umin=1-fmodf(time*0.1f,1.0f);
	vmin=1-fmodf(time*0.2f,1.0f);

	umin1=fmodf(time*0.1f,1.0f);
	vmin1=fmodf(time*0.2f,1.0f);
	
	rd->DrawSprite2(0,0,pTexRender->GetWidth(),pTexRender->GetHeight(),
					umin,vmin,1,1,
					umin1,vmin1,1,1,
					pTex0_0,pTex0_1,sColor4c(255,255,255,255),0,COLOR_ADD);

	rd->RestoreRenderTarget();
}

