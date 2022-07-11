#include "StdAfxRD.h"
#include "cCamera.h"
#include "Scene.h"
#include <algorithm>
#include "TileMap.h"
#include "font.h"
#include "postEffects.h"
/*
О frustum отделённым от cCamera

*/

class CameraShader
{
public:
	PSShowMap* pShowMap;
	PSShowAlpha* pShowAlpha;

	CameraShader()
	{
		pShowMap=new PSShowMap;
		pShowMap->Restore();
		pShowAlpha=new PSShowAlpha;
		pShowAlpha->Restore();
	}

	~CameraShader()
	{
		if(pShowMap)delete pShowMap;
		if(pShowAlpha)delete pShowAlpha;
	}

};

void cVisGeneric::InitShaders()
{
	if(!shaders)
		shaders=new CameraShader;
}

void cVisGeneric::ReleaseShaders()
{
	if(shaders)
	{
		delete shaders;
		shaders=NULL;
	}
}

cCamera::cCamera(cScene *UClass)
{
	SetAttribute(ATTRCAMERA_WRITE_ALPHA);
	Identity(GlobalMatrix);
	matProj.identify();
	matView.identify();
	matViewProj.identify();
	matViewProjScr.identify();
	matProjScr.identify();

	IParent=NULL;
	
	Pos.set(0.f,0.f,0.f);
	Center.set(0.5f,0.5f);
	zPlane.set(1,1e6);

	Clip.set(-0.5f,-0.5f,0.5f,0.5f);

	RenderTarget=0;
	pZBuffer=NULL;

	Focus.set(1,1);
	FocusViewPort.set(0,0);
	ScaleViewPort.set(0,0);
	RenderSize.set(0,0);

	camerapass=SCENENODE_OBJECT;

	pTestGrid=NULL;
	TestGridShl=0;
	RootCamera=this;
	Parent=NULL;

	VISASSERT(gb_RenderDevice);
	GetScene() = UClass;

	h_reflection=0;
	RenderSurface=NULL;
	phone_color.set(255,255,255);
	pZTexture=NULL;
	is_shadow_in_current_camera=false;

	pSecondRTTexture=NULL;

	grassObj = NULL;

}
cCamera::~cCamera()
{
	PreDrawScene();
	RELEASE(pZTexture);
	delete pTestGrid;
}

void TempDrawShadow(int width,int height);

class SortObjectSpecial
{
public:
	bool operator ()(const cBaseGraphObject* p1,const cBaseGraphObject* p2)const
	{
		return p1->GetSpecialSortIndex()<p2->GetSpecialSortIndex();
	}
};
void cCamera::ClearFloatZBuffer()
{
	DWORD fogenable = gb_RenderDevice3D->GetRenderState(D3DRS_FOGENABLE);
	gb_RenderDevice3D->SetRenderState(D3DRS_FOGENABLE,FALSE);
	gb_RenderDevice3D->SetBlendState(ALPHA_NONE);
	gb_RenderDevice3D->dtAdvance->pPSClearAlpha->Select(FLT_INF,0,0,1);
	cVertexBuffer<sVertexXYZWD>* pBuf=gb_RenderDevice3D->GetBufferXYZWD();
	sVertexXYZWD* v=	pBuf->Lock(4);

	v[0].z=v[1].z=v[2].z=v[3].z=1.000f;
	v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=sColor4c(255,255,255,255);
	v[0].x=v[1].x=-0.5f; v[0].y=v[2].y=-0.5f; 
	v[3].x=v[2].x=-0.5f+gb_RenderDevice3D->GetSizeX(); v[1].y=v[3].y=-0.5f+gb_RenderDevice3D->GetSizeY(); 

	pBuf->Unlock(4);
	pBuf->DrawPrimitive(PT_TRIANGLESTRIP,2);
	gb_RenderDevice3D->SetRenderState(D3DRS_FOGENABLE,fogenable);
}

bool in_draw_assert=false;
void cCamera::DrawScene()
{
	xassert(!GetAttribute(ATTRCAMERA_SHADOW|ATTRCAMERA_SHADOWMAP));
	if(IsBadClip())	{
		xassert(0 && "Is bad clip");
		return;
	}
	if(!Parent) {
		gb_RenderDevice->FlushPrimitive3D();
		gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
  		gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_anisotropic);

		cCamera* pShadow=FindChildCamera(ATTRCAMERA_SHADOWMAP);
		gb_RenderDevice3D->SetAdvance(pShadow!=0);
		if(pShadow) {			
			gb_RenderDevice3D->SetShadowMatViewProj(pShadow->matViewProj); // pShadow->matViewProj
			if(gb_RenderDevice3D->dtAdvance->GetShadowMap())
				gb_RenderDevice3D->SetShadowMapSize(gb_RenderDevice3D->dtAdvance->GetShadowMap()->GetWidth());
		}
		pShadow = FindChildCamera(ATTRCAMERA_FLOAT_ZBUFFER);
		if (pShadow)
		{
			gb_RenderDevice3D->SetFloatZBufferMatViewProj(pShadow->matViewProj);
			if(gb_RenderDevice3D->dtFixed->GetFloatMap())
				gb_RenderDevice3D->SetFloatZBufferSize(gb_RenderDevice3D->dtFixed->GetFloatMap()->GetWidth(),
													   gb_RenderDevice3D->dtFixed->GetFloatMap()->GetHeight());

		}
		xassert(!in_draw_assert);
		in_draw_assert=true;
	}

	is_shadow_in_current_camera=false;
	if(!Parent)
	{
		is_shadow_in_current_camera=FindChildCamera(ATTRCAMERA_SHADOWMAP)?true:false;
	}else
	{
		is_shadow_in_current_camera=Parent->FindChildCamera(ATTRCAMERA_SHADOWMAP)?true:false;
	}

	vector<cCamera*>::iterator it_c;
	FOR_EACH(child,it_c)
		(*it_c)->DrawScene();

	if(GetAttribute(ATTRCAMERA_WRITE_ALPHA) && DrawArray[SCENENODE_FLAT_SILHOUETTE].empty()||GetAttribute(ATTRCAMERA_REFLECTION))
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
	else
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);

	gb_RenderDevice->SetDrawNode(this);
//	gb_RenderDevice3D->SetRenderTarget1(GetSecondRT());

	if(!Option_ShowType[SHOW_REDLECTION] && GetAttribute(ATTRCAMERA_REFLECTION))
		return;
	if(!Option_ShowType[SHOW_NORMAL] && !Parent)
	{
		Set2DRenderState();
		return;
	}

	if (GetAttribute(ATTRCAMERA_FLOAT_ZBUFFER))
		ClearFloatZBuffer();
	if(GetAttribute(ATTRCAMERA_CLEARZBUFFER) )
		ClearZBuffer();
	if(GetAttribute(ATTRCAMERA_SHOWCLIP))
		 ShowClip();

	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHAREF,0);
	DWORD fogenable;

	DrawObjectFirst();

	gb_RenderDevice->SetRenderState( RS_ZWRITEENABLE, GetAttr(ATTRCAMERA_NOZWRITE)?FALSE:TRUE );

	float fBiasSlope=0;
	gb_RenderDevice3D->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&fBiasSlope);


	fogenable=gb_RenderDevice3D->GetRenderState(D3DRS_FOGENABLE);

	DrawTilemapObject();
	if(Option_ShowType[SHOW_OBJECT])
		DrawObjectNoZ(SCENENODE_OBJECT_NOZ_BEFORE_GRASS);
	if(grassObj)
		grassObj->Draw(this);

	if(Option_ShowType[SHOW_OBJECT]) {
		DrawType* draw=gb_RenderDevice3D->dtAdvance;
		draw->BeginDraw();

		DrawObjectNoZ(SCENENODE_OBJECT_NOZ_AFTER_GRASS);
		DrawObject(SCENENODE_OBJECT);
		DrawSilhouetteObject();
		DrawObjectSpecial(SCENENODE_OBJECTSPECIAL);
		DrawObject(SCENENODE_UNDERWATER);
		DrawSortObject();
		//DrawObject2Pass();
	}
	//-----
	//if(!Parent&&Option_UseDOF)
	//{
	//	DrawToZBuffer();
	//}

	//------
	//DebugDrawFrustum();
	if (GetScene()->GetMirageCamera())
		GetScene()->GetMirageCamera()->DrawScene();
	gb_RenderDevice->FlushPrimitive3D();
	gb_RenderDevice->SetRenderState(RS_FOGENABLE,fogenable);

  	if(!Parent) {
		DrawShadowDebug();
		Set2DRenderState();
		if(false) {
			cCamera* pShadow = FindChildCamera(ATTRCAMERA_SHADOWMAP);
			if(pShadow) {
				cFont* pFont=gb_VisGeneric->CreateDebugFont();
				gb_RenderDevice->SetFont(pFont);
				char s[128];
				sprintf(s,"ZMin=%f",pShadow->zPlane.x);
				gb_RenderDevice->OutText(800,10,s,sColor4f(0,0,1,1));
				sprintf(s,"ZMax=%f",pShadow->zPlane.y);
				gb_RenderDevice->OutText(800,30,s,sColor4f(0,0,1,1));
				sprintf(s,"pos=%.0f,%.0f,%.0f",Pos.x,Pos.y,Pos.z);
				gb_RenderDevice->OutText(800,50,s,sColor4f(0,0,1,1));

				gb_RenderDevice->SetFont(NULL);
				pFont->Release();
			}
		}
		in_draw_assert=false;
		//DrawAlphaPlane();
		if(gb_VisGeneric->IsSilhouettesEnabled() && !DrawArray[SCENENODE_FLAT_SILHOUETTE].empty())
			DrawSilhouettePlane();
	}

	if(!Parent && Option_ShowRenderTextureDBG==7)
		TempDrawShadow(vp.Width,vp.Height);
//	if(!Parent)DrawTestGrid();
	for(int i=0;i<4;i++)
		gb_RenderDevice3D->SetSamplerData(i,sampler_wrap_linear);
//???	gb_RenderDevice3D->RestoreRenderTarget();
}
void cCamera::DrawToZBuffer()
{
	DWORD colorEnableState = gb_RenderDevice3D->GetRenderState(D3DRS_COLORWRITEENABLE);
	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA);
	gb_RenderDevice->SetRenderState( RS_ZWRITEENABLE, FALSE );
	gb_RenderDevice3D->dtAdvance->pPSClearAlpha->Select(sColor4f(0,0,0,1));
	cVertexBuffer<sVertexXYZWD>* pBuf=gb_RenderDevice3D->GetBufferXYZWD();
	sVertexXYZWD* v=	pBuf->Lock(4);

	v[0].z=v[1].z=v[2].z=v[3].z=1.000f;
	v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=sColor4c(255,255,255,255);
	v[0].x=v[1].x=-0.5f; v[0].y=v[2].y=-0.5f; 
	v[3].x=v[2].x=-0.5f+gb_RenderDevice3D->GetSizeX(); v[1].y=v[3].y=-0.5f+gb_RenderDevice3D->GetSizeY(); 

	pBuf->Unlock(4);
	pBuf->DrawPrimitive(PT_TRIANGLESTRIP,2);
	DWORD ZFUNC=gb_RenderDevice3D->GetRenderState(D3DRS_ZFUNC);
	gb_RenderDevice3D->SetRenderState( D3DRS_ZFUNC, D3DCMP_EQUAL );
	gb_RenderDevice3D->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
	DWORD fogenable1=gb_RenderDevice3D->GetRenderState(D3DRS_FOGENABLE);
	gb_RenderDevice->SetRenderState(RS_FOGENABLE,FALSE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
	SetAttr(ATTRCAMERA_ZBUFFER);
	//DrawObjectFirst();
	DrawTilemapObject();
	DrawObject(SCENENODE_OBJECT);
	DrawObject(SCENENODE_FLAT_SILHOUETTE);
	DrawObjectSpecial(SCENENODE_OBJECTSPECIAL);
	//DrawSilhouetteObject();
	//DrawSortObject();
	ClearAttr(ATTRCAMERA_ZBUFFER);
	gb_RenderDevice->SetRenderState( RS_ZWRITEENABLE, TRUE);
	gb_RenderDevice3D->SetRenderState( D3DRS_ZFUNC, ZFUNC );
	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,colorEnableState);
	gb_RenderDevice->SetRenderState(RS_FOGENABLE,fogenable1);

}

void cCamera::Set2DRenderState()
{
	gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_point);
  	gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
	gb_RenderDevice->SetRenderState(RS_FOGENABLE,FALSE);
}

void cCamera::PreDrawScene()
{
	child.clear();

	for(int j=0;j<MAXSCENENODE;j++)
		DrawArray[j].clear();

	SortArray.clear();
	arZPlane.clear();
	grassObj=NULL;
}

void cCamera::SetClip(const sRectangle4f &clip) 
{
	Clip=clip;
	Update();
}

/*
Ortho
2/w  0    0           0
0    2/h  0           0
0    0    1/(zf-zn)   0
0    0    zn/(zn-zf)  1

Perspective
2*zn/w  0       0              0
0       2*zn/h  0              0
0       0       zf/(zf-zn)     1
0       0       zn*zf/(zn-zf)  0
*/

void cCamera::Update() 
{ 
	xassert(fabsf(fabsf(GlobalMatrix.rot().det())-1)<1e-3f);

	UpdateViewport();

	if(!GetAttribute(ATTRCAMERA_NOT_CALC_PROJ))
	{
		memset(&matProj,0,sizeof(matProj));
		if(GetAttribute(ATTRCAMERA_PERSPECTIVE))
		{
			float zc=zPlane.y/(zPlane.y-zPlane.x);
			matProj._11=2*Focus.x*ScaleViewPort.x/(Clip.xmax()-Clip.xmin());
			matProj._22=2*Focus.y*ScaleViewPort.y/(Clip.ymax()-Clip.ymin());
			matProj._33=zc;
			matProj._43=-zc*zPlane.x;

			matProj._31=0;
			matProj._32=0;
			matProj._34=1;
		}else
		{
			matProj._11=2*Focus.x*ScaleViewPort.x/(Clip.xmax()-Clip.xmin());
			matProj._22=2*Focus.y*ScaleViewPort.y/(Clip.ymax()-Clip.ymin());
			matProj._33=1/(zPlane.y-zPlane.x);
			matProj._43=zPlane.x/(zPlane.x-zPlane.y);
			matProj._41=0;
			matProj._42=0;
			matProj._44=1;
		}
	}

	vp.X = round((GetCenterX()+Clip.xmin())*RenderSize.x);
	vp.Y = round((GetCenterY()+Clip.ymin())*RenderSize.y);
	vp.Width = round((GetCenterX()+Clip.xmax())*RenderSize.x)-vp.X;
	vp.Height = round((GetCenterY()+Clip.ymax())*RenderSize.y)-vp.Y;
	vp.MinZ=0; vp.MaxZ=1;

	matView.set( 
		GetMatrix().rot()[0][0], GetMatrix().rot()[1][0], GetMatrix().rot()[2][0], 0,
		GetMatrix().rot()[0][1], GetMatrix().rot()[1][1], GetMatrix().rot()[2][1], 0,
		GetMatrix().rot()[0][2], GetMatrix().rot()[1][2], GetMatrix().rot()[2][2], 0,
		GetMatrix().trans().x,   GetMatrix().trans().y,   GetMatrix().trans().z,   1 );
	matViewProj = matView * matProj;

	UpdateViewProjScr();

	WorldI=GetMatrix().rot().xrow();
	WorldJ=GetAttribute(ATTRCAMERA_REFLECTION)?-GetMatrix().rot().yrow():GetMatrix().rot().yrow();
	WorldK=GetMatrix().rot().zrow();
	// определение местоположения камеры в мировом пространстве
	GetPos() = GetMatrix().invXformPoint(Vect3f(0,0,0));

	CalcClipPlane();
}

void cCamera::UpdateViewProjScr()
{
	CMatrix matScr( 
		float(+vp.Width/2),			0,					0,				0,
		0,					float(-vp.Height/2),			0,				0,
		0,						0,				vp.MaxZ-vp.MinZ,	0,
		float(vp.X+vp.Width/2),	float(vp.Y+vp.Height/2),	vp.MinZ,			1 );
	matViewProjScr = matViewProj*matScr;
	matProjScr = matProj*matScr;
}

/*
Perspective
2*zn/w  0       0              0
0       2*zn/h  0              0
0       0       zf/(zf-zn)     1
0       0       zn*zf/(zn-zf)  0

Orto
2/w  0    0           0
0    2/h  0           0
0    0    1/(zf-zn)   0
0    0    zn/(zn-zf)  1
*/
void cCamera::CalcClipPlane()
{
	if(GetAttribute(ATTRCAMERA_PERSPECTIVE))
	{
		sPlane4f* v=PlaneClip3d;
		Vect3f p00,p01,p10,p11,d00,d01,d10,d11;
		GetFrustumPoint(p00,p01,p10,p11,d00,d01,d10,d11);

		if(GetAttribute(ATTRCAMERA_REFLECTION)!=0)
		{
			v[0].Set(d00,d01,d11);			// far z
			v[1].Set(p00,p11,p01);			// near z
			v[2].Set(GetPos(),p00,p10);		// left
			v[3].Set(GetPos(),p11,p01);		// right
			v[4].Set(GetPos(),p10,p11);		// top
			v[5].Set(GetPos(),p01,p00);		// bottom
		}
		else
		{
			v[0].Set(d00,d11,d01);			// far z
			v[1].Set(p00,p01,p11);			// near z
			v[2].Set(GetPos(),p10,p00);		// left
			v[3].Set(GetPos(),p01,p11);		// right
			v[4].Set(GetPos(),p11,p10);		// top
			v[5].Set(GetPos(),p00,p01);		// bottom
		}
	}else
	{
		//float div=GetPos().z/GetFocusViewPort().x;
		//Vect2f w(GetFocusViewPort().x/GetScaleViewPort().x*div,GetFocusViewPort().y/GetScaleViewPort().y*div);
		Vect2f w(1/(Focus.x*ScaleViewPort.x),1/(Focus.y*ScaleViewPort.y));

		Vect3f p00,p01,p11,p10,d00,d01,d10,d11;
		GetMatrix().invXformPoint(Vect3f(Clip.xmin()*w.x,Clip.ymin()*w.y,zPlane.x),p00);
		GetMatrix().invXformPoint(Vect3f(Clip.xmax()*w.x,Clip.ymin()*w.y,zPlane.x),p01);
		GetMatrix().invXformPoint(Vect3f(Clip.xmax()*w.x,Clip.ymax()*w.y,zPlane.x),p11);
		GetMatrix().invXformPoint(Vect3f(Clip.xmin()*w.x,Clip.ymax()*w.y,zPlane.x),p10);

		GetMatrix().invXformPoint(Vect3f(Clip.xmin()*w.x,Clip.ymin()*w.y,zPlane.y),d00);
		GetMatrix().invXformPoint(Vect3f(Clip.xmax()*w.x,Clip.ymin()*w.y,zPlane.y),d01);
		GetMatrix().invXformPoint(Vect3f(Clip.xmin()*w.x,Clip.ymax()*w.y,zPlane.y),d10);
		GetMatrix().invXformPoint(Vect3f(Clip.xmax()*w.x,Clip.ymax()*w.y,zPlane.y),d11);

		PlaneClip3d[0].Set(d00,d11,d01);			// far z
		PlaneClip3d[1].Set(p00,p01,p11);			// near z
		PlaneClip3d[2].Set(d10,p00,p10);			// left
		PlaneClip3d[3].Set(d01,p11,p01);			// right
		PlaneClip3d[4].Set(d11,p10,p11);			// top
		PlaneClip3d[5].Set(d00,p01,p00);			// bottom
	}
}

void cCamera::GetFrustumPoint(Vect3f& p00,Vect3f& p01,Vect3f& p10,Vect3f& p11,Vect3f& d00,Vect3f& d01,Vect3f& d10,Vect3f& d11,float rmul)
{
	if(GetAttribute(ATTRCAMERA_PERSPECTIVE))
	{
		float fx=GetFocusX();
		float rx,ry;
		rx=ScaleViewPort.x;ry=RenderSize.y/(float)RenderSize.x;
		rx*=rmul/fx;ry*=rmul/fx;

		float cx=(Clip.xmin()+Clip.xmax())*0.5f,cy=(Clip.ymin()+Clip.ymax())*0.5f;
		float xi=Clip.xmin()-cx,xa=Clip.xmax()-cx;
		float yi=Clip.ymin()-cy,ya=Clip.ymax()-cy;
	/*
		GetMatrix().invXformPoint(Vect3f(xi*rx,yi*ry,GetFocusX())*zPlane.x,p00);
		GetMatrix().invXformPoint(Vect3f(xa*rx,yi*ry,GetFocusX())*zPlane.x,p01);
		GetMatrix().invXformPoint(Vect3f(xa*rx,ya*ry,GetFocusX())*zPlane.x,p11);
		GetMatrix().invXformPoint(Vect3f(xi*rx,ya*ry,GetFocusX())*zPlane.x,p10);
		GetMatrix().invXformPoint(Vect3f(xi*rx,yi*ry,          1)*zPlane.y,d00);
		GetMatrix().invXformPoint(Vect3f(xa*rx,yi*ry,          1)*zPlane.y,d01);
		GetMatrix().invXformPoint(Vect3f(xi*rx,ya*ry,          1)*zPlane.y,d10);
		GetMatrix().invXformPoint(Vect3f(xa*rx,ya*ry,          1)*zPlane.y,d11);
	*/
		GetMatrix().invXformPoint(Vect3f(xi*rx,yi*ry,1)*zPlane.x,p00);
		GetMatrix().invXformPoint(Vect3f(xa*rx,yi*ry,1)*zPlane.x,p01);
		GetMatrix().invXformPoint(Vect3f(xa*rx,ya*ry,1)*zPlane.x,p11);
		GetMatrix().invXformPoint(Vect3f(xi*rx,ya*ry,1)*zPlane.x,p10);
		GetMatrix().invXformPoint(Vect3f(xi*rx,yi*ry,1)*zPlane.y,d00);
		GetMatrix().invXformPoint(Vect3f(xa*rx,yi*ry,1)*zPlane.y,d01);
		GetMatrix().invXformPoint(Vect3f(xi*rx,ya*ry,1)*zPlane.y,d10);
		GetMatrix().invXformPoint(Vect3f(xa*rx,ya*ry,1)*zPlane.y,d11);
	}else
	{
		Vect2f w(1/(Focus.x*ScaleViewPort.x),1/(Focus.y*ScaleViewPort.y));

		GetMatrix().invXformPoint(Vect3f(Clip.xmin()*w.x,Clip.ymin()*w.y,zPlane.x),p00);
		GetMatrix().invXformPoint(Vect3f(Clip.xmax()*w.x,Clip.ymin()*w.y,zPlane.x),p01);
		GetMatrix().invXformPoint(Vect3f(Clip.xmax()*w.x,Clip.ymax()*w.y,zPlane.x),p11);
		GetMatrix().invXformPoint(Vect3f(Clip.xmin()*w.x,Clip.ymax()*w.y,zPlane.x),p10);

		GetMatrix().invXformPoint(Vect3f(Clip.xmin()*w.x,Clip.ymin()*w.y,zPlane.y),d00);
		GetMatrix().invXformPoint(Vect3f(Clip.xmax()*w.x,Clip.ymin()*w.y,zPlane.y),d01);
		GetMatrix().invXformPoint(Vect3f(Clip.xmin()*w.x,Clip.ymax()*w.y,zPlane.y),d10);
		GetMatrix().invXformPoint(Vect3f(Clip.xmax()*w.x,Clip.ymax()*w.y,zPlane.y),d11);
	}
}

void cCamera::DebugDrawFrustum(sColor4c color)
{
	Vect3f p00,p01,p10,p11,d00,d01,d10,d11;
	GetFrustumPoint(p00,p01,p10,p11,d00,d01,d10,d11,0.95f);
/*
	Vect3f pt00,pt01,pt10,pt11;
	float b=0.5f,m=0.5f;
	pt00=p00*b+d00*m;
	pt01=p01*b+d01*m;
	pt10=p10*b+d10*m;
	pt11=p11*b+d11*m;

	gb_RenderDevice->DrawLine(pt00,pt01,color);
	gb_RenderDevice->DrawLine(pt01,pt11,color);
	gb_RenderDevice->DrawLine(pt11,pt10,color);
	gb_RenderDevice->DrawLine(pt10,pt00,color);
/*/
	gb_RenderDevice->DrawLine(p00,p01,color);
	gb_RenderDevice->DrawLine(p01,p11,color);
	gb_RenderDevice->DrawLine(p11,p10,color);
	gb_RenderDevice->DrawLine(p10,p00,color);

	gb_RenderDevice->DrawLine(d00,d01,color);
	gb_RenderDevice->DrawLine(d01,d11,color);
	gb_RenderDevice->DrawLine(d11,d10,color);
	gb_RenderDevice->DrawLine(d10,d00,color);

	gb_RenderDevice->DrawLine(p00,d00,color);
	gb_RenderDevice->DrawLine(p01,d01,color);
	gb_RenderDevice->DrawLine(p11,d11,color);
	gb_RenderDevice->DrawLine(p10,d10,color);
/**/
	gb_RenderDevice3D->FlushPrimitive3DWorld();
}

void cCamera::GetPlaneClip(sPlane4f PlaneClip[5],const sRectangle4f *Rect) 
{ 
	if(GetAttribute(ATTRCAMERA_PERSPECTIVE))
	{
		float cx=(Clip.xmin()+Clip.xmax())*0.5f,cy=(Clip.ymin()+Clip.ymax())*0.5f;
		float xi=Rect->xmin()-cx,xa=Rect->xmax()-cx;
		float yi=-(Rect->ymin()-cy),ya=-(Rect->ymax()-cy);

		Vect2f focus(zPlane.x/(GetScaleViewPort().x*GetFocusX()),zPlane.x/(GetScaleViewPort().y*GetFocusY()));
		Vect3f Center=GetMatrix().invXformPoint(Vect3f(0,0,0));
		Vect3f p00=GetMatrix().invXformPoint(Vect3f(xi*focus.x,yi*focus.y,zPlane.x),p00);
		Vect3f p01=GetMatrix().invXformPoint(Vect3f(xa*focus.x,yi*focus.y,zPlane.x),p01);
		Vect3f p11=GetMatrix().invXformPoint(Vect3f(xa*focus.x,ya*focus.y,zPlane.x),p11);
		Vect3f p10=GetMatrix().invXformPoint(Vect3f(xi*focus.x,ya*focus.y,zPlane.x),p10);
		PlaneClip[0].Set(p00,p11,p01);				// near z
		PlaneClip[1].Set(Center,p00,p10);			// left
		PlaneClip[2].Set(Center,p11,p01);			// right
		PlaneClip[3].Set(Center,p10,p11);			// top
		PlaneClip[4].Set(Center,p01,p00);			// bottom
	}else
	{
		float div=GetPos().z/GetFocusViewPort().x;
		Vect2f wCamera(GetFocusViewPort().x/GetScaleViewPort().x*div,GetFocusViewPort().y/GetScaleViewPort().y*div);
		Vect3f p010=GetMatrix().invXformPoint(Vect3f(Rect->xmax()*wCamera.x,Rect->ymin()*wCamera.y,0),p010);
		Vect3f p100=GetMatrix().invXformPoint(Vect3f(Rect->xmin()*wCamera.x,Rect->ymax()*wCamera.y,0),p100);
		Vect3f p001=GetMatrix().invXformPoint(Vect3f(Rect->xmin()*wCamera.x,Rect->ymin()*wCamera.y,zPlane.x),p001);
		Vect3f p011=GetMatrix().invXformPoint(Vect3f(Rect->xmax()*wCamera.x,Rect->ymin()*wCamera.y,zPlane.x),p011);
		Vect3f p111=GetMatrix().invXformPoint(Vect3f(Rect->xmax()*wCamera.x,Rect->ymax()*wCamera.y,zPlane.x),p111);
		Vect3f p101=GetMatrix().invXformPoint(Vect3f(Rect->xmin()*wCamera.x,Rect->ymax()*wCamera.y,zPlane.x),p101);
		PlaneClip[0].Set(p001,p111,p011);			// near z
		PlaneClip[1].Set(p100,p001,p101);			// left
		PlaneClip[2].Set(p010,p111,p011);			// right
		PlaneClip[3].Set(p100,p101,p111);			// top
		PlaneClip[4].Set(p010,p011,p001);			// bottom
	}
}
void cCamera::SetPosition(const MatXf& matrix)
{ 
	GetMatrix()=matrix; 

	Update();
}

void cCamera::SetFrustum(const Vect2f *center,const sRectangle4f *clip,const Vect2f *focus,const Vect2f *zplane)
{
	if(clip) Clip=*clip;
	if(center) Center=*center;
	if(focus) Focus=*focus;
	if(zplane)
		zPlane=*zplane;

	Update();
}

void cCamera::GetFrustum(Vect2f *center,sRectangle4f *clip,Vect2f *focus,Vect2f *zplane)
{
	if(clip) *clip=Clip; 
	if(center) center->set(GetCenterX(),GetCenterY());
	if(focus) focus->set(GetFocusX(),GetFocusY()); 
	if(zplane) *zplane=zPlane; 
}

void cCamera::UpdateViewport()
{
	// обновление данных ViewPort
	if(RenderSurface==0 && RenderTarget==0)
		RenderSize.set(float(gb_RenderDevice->GetSizeX()),float(gb_RenderDevice->GetSizeY()));

	FocusViewPort.set(GetFocusX()*RenderSize.x,GetFocusY()*RenderSize.x);
	if (RenderSize.y != 0) {
		ScaleViewPort.set(1,RenderSize.x/RenderSize.y);
	} else {
		ScaleViewPort.set(1, 1.33f);
	}
}

void cCamera::SetRenderTarget(cTexture *pTexture,IDirect3DSurface9* pZBuf)
{
	RenderTarget=pTexture;

	RenderSize.set(float(RenderTarget->GetWidth()),float(RenderTarget->GetHeight()));
	RenderSurface=NULL;

	pZBuffer=pZBuf;
	UpdateViewport();
}

void cCamera::SetRenderTarget(IDirect3DSurface9* pSurface,IDirect3DSurface9* pZBuf)
{
	RenderTarget=NULL;
	RenderSurface=pSurface;
	if(RenderSurface)
	{
		Vect2i size=GetSize(RenderSurface);
		RenderSize.set(float(size.x),float(size.y));
	}
	pZBuffer=pZBuf;
	UpdateViewport();
}


void cCamera::ConvertorWorldToViewPort(const Vect3f *pw,Vect3f *pv,Vect3f *pe)
{
	Vect3f pv0,pe0;
	if(pw==0) return;
	if(pv==0) pv=&pv0;
	if(pe==0) pe=&pe0;
	matViewProjScr.Convert( *pw, *pv, *pe );
}

void cCamera::ConvertorWorldToViewPort(const Vect3f *pw,float WorldRadius,Vect3f *pe,int *ScreenRadius)
{
	Vect3f pv,pe0;
	if(pw==0) return;
	if(pe==0) pe=&pe0;
	matViewProjScr.Convert( *pw, pv, *pe );
	if(ScreenRadius) *ScreenRadius=round(WorldRadius*GetFocusViewPort().x/pv.z);
}
void cCamera::CorrectWorldRadius(const Vect3f *pw, float *WorldRadius, int ScreenRadius)
{
	Vect3f pv,pe0;
	if(pw==0) return;
	matViewProjScr.Convert( *pw, pv, pe0 );
	float wrldR = ScreenRadius*pv.z/GetFocusViewPort().x;
	if(wrldR > *WorldRadius)
		*WorldRadius = wrldR;
}
void cCamera::ConvertorCameraToWorld(const Vect2f *screen_pos,Vect3f *pw)
{
	float x,y;
	float cx=(Clip.xmin()+Clip.xmax())*0.5f,
		  cy=(Clip.ymin()+Clip.ymax())*0.5f;
	x=screen_pos->x-cx-(Center.x-0.5f);
	y=screen_pos->y-cy-(Center.y-0.5f);

	Vect2f focus;
	if(GetAttribute(ATTRCAMERA_PERSPECTIVE))
		focus.set(zPlane.x/(GetScaleViewPort().x*GetFocusX()),
				  zPlane.x/(GetScaleViewPort().y*GetFocusY()));
	else
		focus.set(1/(GetScaleViewPort().x*GetFocusX()),1/(GetScaleViewPort().y*GetFocusY()));

	GetMatrix().invXformPoint(Vect3f(x*focus.x,-y*focus.y,zPlane.x),*pw);
}

void cCamera::GetWorldRay(const Vect2f& screen_pos,Vect3f& pos,Vect3f& dir)
{
	if(GetAttribute(ATTRCAMERA_PERSPECTIVE))
	{
		ConvertorCameraToWorld(&screen_pos,&pos);
		dir=pos-GetPos();
		dir.normalize();
	}else
	{
		ConvertorCameraToWorld(&screen_pos,&pos);
		dir=GetWorldK();
	}
}

void cCamera::SetCopy(cCamera* DrawNode)
{
	DrawNode->Attribute=Attribute & ~DWORD(ATTRCAMERA_CLEARZBUFFER);
	DrawNode->Pos=Pos;
	DrawNode->Focus=Focus;
	DrawNode->Center=Center;
	DrawNode->Clip=Clip;
	DrawNode->zPlane=zPlane;

	DrawNode->GlobalMatrix=GlobalMatrix;
//	DrawNode->IParent=IParent;

	DrawNode->Pos=Pos;
	DrawNode->matProj=matProj;
	DrawNode->matView=matView;
	DrawNode->matViewProj=matViewProj;
	DrawNode->matViewProjScr=matViewProjScr;
	DrawNode->matProjScr=matProjScr;
	DrawNode->zPlane=zPlane;
	DrawNode->vp=vp;

	for(int i=0;i<PlaneClip3d_size;i++)
		DrawNode->PlaneClip3d[i]=PlaneClip3d[i];

	DrawNode->FocusViewPort=FocusViewPort;
	DrawNode->ScaleViewPort=ScaleViewPort;
	DrawNode->RenderSize=RenderSize;
	DrawNode->RenderTarget=GetRenderTarget();
/* Мешает Shadow map???
	for(i=0;i<MAXSCENENODE;i++)
		DrawNode->DrawArray[i].clear();
	DrawNode->SortArray.clear();
	DrawNode->arZPlane.clear();
*/
}

void cCamera::DrawSortObject()
{
	if(GetSecondRT())
	{
		gb_RenderDevice3D->SetRenderTarget1(NULL);
		gb_RenderDevice3D->SetTexture(6,GetSecondRT());
	}

	camerapass=SCENENODE_OBJECTSORT;
	DWORD old_cullmode=gb_RenderDevice3D->GetRenderState(D3DRS_CULLMODE);
	gb_RenderDevice->SetRenderState( RS_ZWRITEENABLE, FALSE );
	gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
	stable_sort(SortArray.begin(),SortArray.end(),ObjectSortByRadius());

	vector<ObjectSort>::iterator it;
	FOR_EACH( SortArray, it )
	{
		it->obj->Draw(this);
	}

	gb_RenderDevice->SetRenderState( RS_ZWRITEENABLE, TRUE );
	gb_RenderDevice3D->SetRenderState( D3DRS_CULLMODE, old_cullmode );

	if(GetSecondRT())
	{
		gb_RenderDevice3D->SetTexture(6,NULL);
	}
}

void cCamera::AttachNoRecursive(int pos,cBaseGraphObject* UObject)
{
	if( pos!=SCENENODE_OBJECTSORT && pos!= SCENENODE_OBJECT_2PASS)
	{
		if(!Parent)
		{
			if(!UObject->GetAttr(ATTRUNKOBJ_IGNORE_NORMALCAMERA))
				DrawArray[pos].push_back(UObject);
		}else
		{
			DrawArray[pos].push_back(UObject);
		}
	}else
	{
		Vect3f cur_pos=UObject->GetCenterObject();
		float distance=cur_pos.distance(GetPos());
		if(!UObject->GetAttr(ATTRUNKOBJ_IGNORE_NORMALCAMERA))
			SortArray.push_back( ObjectSort(distance,UObject) );
	}
}

void cCamera::Attach(int pos,cBaseGraphObject *UObject)
{ 
	vector<cCamera*>::iterator it_c;
	if( pos!=SCENENODE_OBJECTSORT )
	{
		if(!UObject->GetAttr(ATTRUNKOBJ_IGNORE_NORMALCAMERA))
			DrawArray[pos].push_back(UObject);

		FOR_EACH(child,it_c)
		{
			if((*it_c)->GetAttribute(UObject->GetAttr(ATTRCAMERA_REFLECTION|ATTRCAMERA_SHADOW|ATTRCAMERA_SHADOWMAP|ATTRCAMERA_FLOAT_ZBUFFER)))
				(*it_c)->DrawArray[pos].push_back(UObject);
		}
	}else
	{
		Vect3f cur_pos=UObject->GetCenterObject();
		float distance=cur_pos.distance(GetPos());
		if(!UObject->GetAttr(ATTRUNKOBJ_IGNORE_NORMALCAMERA))
			SortArray.push_back( ObjectSort(distance,UObject) );
		FOR_EACH(child,it_c)
		if((*it_c)->GetAttribute(UObject->GetAttr(ATTRCAMERA_REFLECTION|ATTRCAMERA_SHADOW|ATTRCAMERA_FLOAT_ZBUFFER)))
		{
			float distance=cur_pos.distance((*it_c)->GetPos());
			(*it_c)->SortArray.push_back( ObjectSort(distance,UObject) );
		}
		if (UObject->GetAttr(ATTRCAMERA_MIRAGE)&&GetScene()->GetMirageCamera())
		{
			float distance=cur_pos.distance(GetScene()->GetMirageCamera()->GetPos());
			GetScene()->GetMirageCamera()->SortArray.push_back( ObjectSort(distance,UObject) );
		}
	}
}

void cCamera::SetGrass(cBaseGraphObject* grass)
{
	grassObj = grass;
	vector<cCamera*>::iterator it_c;
	FOR_EACH(child,it_c)
	{
		if((*it_c)->GetAttribute(ATTRCAMERA_FLOAT_ZBUFFER))
			(*it_c)->SetGrass(grass);
	}
}

void cCamera::DrawSilhouettePlane()
{
/*
	Объекты с ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE выводить после.
	Сортировать по этому признаку.
	Если есть такие объекты, вызывать потом DrawShadowPlane()
*/
	gb_RenderDevice3D->SetVertexShader(NULL);
	gb_RenderDevice3D->SetPixelShader(NULL);
	cD3DRender* rd=gb_RenderDevice3D;
	DWORD fog=rd->GetRenderState( D3DRS_FOGENABLE);
    // Set renderstates (disable z-buffering, enable stencil, disable fog, and
    // turn on alphablending)
    rd->SetRenderState( D3DRS_ZENABLE,          FALSE );
    rd->SetRenderState( D3DRS_STENCILENABLE,    TRUE );
    rd->SetRenderState( D3DRS_FOGENABLE,        FALSE );
//    rd->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	rd->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

	rd->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_INVDESTALPHA );
	rd->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_DESTALPHA );

    rd->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    rd->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    rd->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2 );
    rd->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    rd->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    rd->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );

	/////////////////////////////////////////////////////////////////////////////////
	vector<cBaseGraphObject*>& drawList = DrawArray[SCENENODE_FLAT_SILHOUETTE];
    vector<cBaseGraphObject*>::iterator it;

	{
		rd->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		rd->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE );
	}

	//!!! Прямоугольнички выводятся всегда, хотя могли бы выводиться, только когда прошёл тест на occlusion query и немного fillrate сэкономить.

    FOR_EACH(drawList, it) {
        xassert((*it)->GetKind() == KIND_OBJ_3DX);
		cObject3dx* obj = static_cast<cObject3dx*>((*it));
		if(obj->GetAttr(ATTR3DX_ALWAYS_FLAT_SILUETTE))
		{
			rd->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		}else
		{
		    rd->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		}

        int silhouetteIndex = obj->GetSilhouetteIndex();
        sColor4c diffuse= gb_VisGeneric->GetSilhouetteColor(silhouetteIndex);

		rd->SetRenderState(D3DRS_STENCILREF,  GetSilhouetteStencil(silhouetteIndex));

/*
		sBox6f box;
		obj->GetBoundBox(box);

		Vect3f points[] = {
			Vect3f(box.xmin(), box.ymin(), box.zmin()),
			Vect3f(box.xmin(), box.ymin(), box.zmax()),			
			Vect3f(box.xmin(), box.ymax(), box.zmin()),			
			Vect3f(box.xmin(), box.ymax(), box.zmax()),
			Vect3f(box.xmax(), box.ymin(), box.zmin()),
			Vect3f(box.xmax(), box.ymin(), box.zmax()),			
			Vect3f(box.xmax(), box.ymax(), box.zmin()),			
			Vect3f(box.xmax(), box.ymax(), box.zmax())
		};


		Vect3f top_left(points[0]);
		matViewProjScr.Convert(obj->GetPosition().trans() + points[0], top_left); 
		Vect3f right_bottom(top_left);

		for(int i = 1; i < sizeof(points)/sizeof(points[0]); ++i) {
			Vect3f screenPoint;
			matViewProjScr.Convert(obj->GetPosition().trans() + points[i], screenPoint); 
			if(screenPoint.x < top_left.x)
				top_left.x = screenPoint.x;
			if(screenPoint.y < top_left.y)
				top_left.y = screenPoint.y;
			if(screenPoint.x > right_bottom.x)
				right_bottom.x = screenPoint.x;
			if(screenPoint.y > right_bottom.y)
				right_bottom.y = screenPoint.y;
		}		
		int x1 = top_left.xi();
		int y1 = top_left.yi();
		int x2 = right_bottom.xi();
		int y2 = right_bottom.yi();
/*/
		sBox6f box=obj->CalcDynamicBoundBox(GetMatrix());

		Vect3f points[] = {
			Vect3f(box.xmin(), box.ymin(), box.zmin()),
			Vect3f(box.xmin(), box.ymin(), box.zmax()),			
			Vect3f(box.xmin(), box.ymax(), box.zmin()),			
			Vect3f(box.xmin(), box.ymax(), box.zmax()),
			Vect3f(box.xmax(), box.ymin(), box.zmin()),
			Vect3f(box.xmax(), box.ymin(), box.zmax()),			
			Vect3f(box.xmax(), box.ymax(), box.zmin()),			
			Vect3f(box.xmax(), box.ymax(), box.zmax())
		};


		Vect3f top_left(points[0]);
		matProjScr.Convert(points[0], top_left); 
		Vect3f right_bottom(top_left);

		for(int i = 1; i < sizeof(points)/sizeof(points[0]); ++i) {
			Vect3f screenPoint;
			matProjScr.Convert(points[i], screenPoint); 
			if(screenPoint.x < top_left.x)
				top_left.x = screenPoint.x;
			if(screenPoint.y < top_left.y)
				top_left.y = screenPoint.y;
			if(screenPoint.x > right_bottom.x)
				right_bottom.x = screenPoint.x;
			if(screenPoint.y > right_bottom.y)
				right_bottom.y = screenPoint.y;
		}
		const max_size=10000;
		top_left.x=clamp(top_left.x,-max_size,+max_size);
		top_left.y=clamp(top_left.y,-max_size,+max_size);
		right_bottom.x=clamp(right_bottom.x,-max_size,+max_size);
		right_bottom.y=clamp(right_bottom.y,-max_size,+max_size);
		int x1 = top_left.xi();
		int y1 = top_left.yi();
		int x2 = right_bottom.xi();
		int y2 = right_bottom.yi();
/**/
		{
			cVertexBuffer<sVertexXYZWD>* buf=rd->GetBufferXYZWD();
			sVertexXYZWD* v=buf->Lock(4);
			v[0].z=v[1].z=v[2].z=v[3].z=0.001f;
			v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
			v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=diffuse;
			v[0].x=v[1].x=float(x1); v[0].y=v[2].y=float(y1); 
			v[3].x=v[2].x=float(x2); v[1].y=v[3].y=float(y2); 
			buf->Unlock(4);
			buf->DrawPrimitive(PT_TRIANGLESTRIP,2);
		}
    }

    // Restore render states
    rd->SetRenderState( D3DRS_ZENABLE,          TRUE );
    rd->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
    rd->SetRenderState( D3DRS_FOGENABLE,        fog );
    rd->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
}

void cCamera::DrawAlphaPlane()
{
	gb_RenderDevice3D->SetVertexShader(NULL);
	gb_RenderDevice3D->SetPixelShader(NULL);
	cD3DRender* rd=gb_RenderDevice3D;
	DWORD fog=rd->GetRenderState( D3DRS_FOGENABLE);
    // Set renderstates (disable z-buffering, enable stencil, disable fog, and
    // turn on alphablending)
    rd->SetRenderState( D3DRS_ZENABLE,          FALSE );
    rd->SetRenderState( D3DRS_FOGENABLE,        FALSE );
    rd->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );

	rd->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_INVDESTALPHA );
	rd->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_DESTALPHA );

    rd->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    rd->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    rd->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2 );
    rd->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    rd->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    rd->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );


	int x1 = vp.X;
	int y1 = vp.Y;
	int x2 = vp.X+vp.Width;
	int y2 = vp.Y+vp.Height;
	{
		cVertexBuffer<sVertexXYZWD>* buf=rd->GetBufferXYZWD();
		sVertexXYZWD* v=buf->Lock(4);
		v[0].z=v[1].z=v[2].z=v[3].z=0.001f;
		v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
		v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=sColor4c(255,0,255);
		v[0].x=v[1].x=float(x1); v[0].y=v[2].y=float(y1); 
		v[3].x=v[2].x=float(x2); v[1].y=v[3].y=float(y2); 
		buf->Unlock(4);
		buf->DrawPrimitive(PT_TRIANGLESTRIP,2);
	}

    // Restore render states
    rd->SetRenderState( D3DRS_ZENABLE,          TRUE );
    rd->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
    rd->SetRenderState( D3DRS_FOGENABLE,        fog );
    rd->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
}

eTestVisible cCamera::GridTest(Vect3f p[8])
{
	for(int i=0;i<8;i++)
	{
		int x=round(p[i].x)>>TestGridShl,y=round(p[i].y)>>TestGridShl;
		if(x<0 || x>=TestGridSize.x || y<0 || y>=TestGridSize.y)
			continue;
		if(pTestGrid[x+y*TestGridSize.x])
			return VISIBLE_INTERSECT;
	}

	return VISIBLE_OUTSIDE;
}

eTestVisible cCamera::TestVisible(const MatXf &matrix,const Vect3f &min,const Vect3f &max)
{ // для BoundingBox с границами min && max
	Vect3f	p[8];
	matrix.xformPoint(Vect3f(min.x,min.y,min.z),p[0]);
	matrix.xformPoint(Vect3f(max.x,min.y,min.z),p[1]);
	matrix.xformPoint(Vect3f(min.x,max.y,min.z),p[2]);
	matrix.xformPoint(Vect3f(max.x,max.y,min.z),p[3]);
	matrix.xformPoint(Vect3f(min.x,min.y,max.z),p[4]);
	matrix.xformPoint(Vect3f(max.x,min.y,max.z),p[5]);
	matrix.xformPoint(Vect3f(min.x,max.y,max.z),p[6]);
	matrix.xformPoint(Vect3f(max.x,max.y,max.z),p[7]);

	if(RootCamera->pTestGrid)
		return RootCamera->GridTest(p);

	for(int i=0;i<GetNumberPlaneClip3d();i++)
	{
		if(GetPlaneClip3d(i).GetDistance(p[0])>=0) continue;
		if(GetPlaneClip3d(i).GetDistance(p[1])>=0) continue;
		if(GetPlaneClip3d(i).GetDistance(p[2])>=0) continue;
		if(GetPlaneClip3d(i).GetDistance(p[3])>=0) continue;
		if(GetPlaneClip3d(i).GetDistance(p[4])>=0) continue;
		if(GetPlaneClip3d(i).GetDistance(p[5])>=0) continue;
		if(GetPlaneClip3d(i).GetDistance(p[6])>=0) continue;
		if(GetPlaneClip3d(i).GetDistance(p[7])>=0) continue;
		return VISIBLE_OUTSIDE;
	}
	return VISIBLE_INTERSECT;
}

//*
eTestVisible cCamera::TestVisible(const Vect3f &min,const Vect3f &max)
{ // для BoundingBox с границами min && max, заданными в глобальным координатах
	for(int i=0;i<GetNumberPlaneClip3d();i++)
	{
		sPlane4f& p=GetPlaneClip3d(i);
		if(p.GetDistance(min)>=0) continue;
		if(p.GetDistance(Vect3f(max.x,min.y,min.z))>=0) continue;
		if(p.GetDistance(Vect3f(min.x,max.y,min.z))>=0) continue;
		if(p.GetDistance(Vect3f(max.x,max.y,min.z))>=0) continue;
		if(p.GetDistance(Vect3f(min.x,min.y,max.z))>=0) continue;
		if(p.GetDistance(Vect3f(max.x,min.y,max.z))>=0) continue;
		if(p.GetDistance(Vect3f(min.x,max.y,max.z))>=0) continue;
		if(p.GetDistance(max)>=0) continue;
		return VISIBLE_OUTSIDE;
	}
	return VISIBLE_INTERSECT;
}
/**/
/*
int cCamera::TestVisible(const Vect3f &vmin,const Vect3f &vmax)
{ // для BoundingBox с границами min && max, заданными в глобальным координатах
	Vect3f v[8];
	matViewProj.Convert(vmin,v[0]);
	matViewProj.Convert(Vect3f(vmax.x,vmin.y,vmin.z),v[1]);
	matViewProj.Convert(Vect3f(vmin.x,vmax.y,vmin.z),v[2]);
	matViewProj.Convert(Vect3f(vmax.x,vmax.y,vmin.z),v[3]);
	matViewProj.Convert(Vect3f(vmin.x,vmin.y,vmax.z),v[4]);
	matViewProj.Convert(Vect3f(vmax.x,vmin.y,vmax.z),v[5]);
	matViewProj.Convert(Vect3f(vmin.x,vmax.y,vmax.z),v[6]);
	matViewProj.Convert(vmax,v[7]);

	const float MinX=-1;
	const float MaxX=+1;
	const float MinY=-1;
	const float MaxY=+1;
	float MinZ=vp.MinZ,MaxZ=vp.MaxZ;

	for(int i=0;i<8;i++)
	{
		Vect3f& c=v[i];
		if(c.x<MinX || c.x>MaxX)continue;
		if(c.y<MinY || c.y>MaxY)continue;
		if(c.z<MinZ || c.z>MaxZ)continue;
		return 0;
	}
	return 1;
}
/**/
//*
eTestVisible cCamera::TestVisibleComplete(const Vect3f &min,const Vect3f &max)
{
	VISASSERT(GetNumberPlaneClip3d()==6);
	float m, n;
	int i;
	eTestVisible result = VISIBLE_INSIDE;
	float dx=max.x-min.x;
	float dy=max.y-min.y;
	float dz=max.z-min.z;
	VISASSERT(dx>=0 && dy>=0 && dz>=0);

	for (i = 0; i < GetNumberPlaneClip3d(); i++)
	{
		sPlane4f& p=GetPlaneClip3d(i);
		m = (min.x * p.A) + (min.y * p.B) + (min.z * p.C) + p.D;
		n = (dx * ABS(p.A)) + (dy * ABS(p.B)) + (dz * ABS(p.C));

		if (m + n < 0) return VISIBLE_OUTSIDE;
		if (m - n < 0) result = VISIBLE_INTERSECT;

    }
	return result;
}
/**/

void cCamera::ClearZBuffer()
{
	RDCALL(gb_RenderDevice3D->lpD3DDevice->Clear(0,NULL,D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1, 0));
}

void cCamera::ShowClip()
{
	gb_RenderDevice->DrawRectangle(vp.X,vp.Y,vp.Width-1,vp.Height-1, sColor4c(255,255,255,255),true);
}

cCamera* cCamera::FindChildCamera(int AttributeCamera)
{
	if(GetAttribute(AttributeCamera))
		return this;

	vector<cCamera*>::iterator it_c;
	FOR_EACH(child,it_c)
	{
		cCamera* p=(*it_c)->FindChildCamera(AttributeCamera);
		if(p)return p;
	}

	return NULL;
}

void cCamera::DrawObjectFirst()
{
	camerapass=SCENENODE_OBJECTFIRST;
	int number_draw=GetNumberDraw(SCENENODE_OBJECTFIRST);
	for( int nObj=0; nObj<number_draw; nObj++ )
		GetDraw(SCENENODE_OBJECTFIRST,nObj)->Draw(this);
}

#include "TileMap.h"
void cCamera::EnableGridTest(int grid_dx,int grid_dy,int grid_size)
{
	InitGridTest(grid_dx,grid_dy,grid_size);
	VISASSERT(!Parent && pTestGrid);
	CalcTestForGrid();
}

void cCamera::InitGridTest(int grid_dx,int grid_dy,int grid_size)
{
	if(pTestGrid)
	{
		if(TestGridSize.x==grid_dx && TestGridSize.y==grid_dy)
			return;
		delete pTestGrid;
	}

	TestGridSize.x=grid_dx;
	TestGridSize.y=grid_dy;

	TestGridShl=0;
	while(grid_size>1)
	{
		TestGridShl++;
		grid_size=grid_size>>1;
	}
	VISASSERT(grid_size==1);
	
	pTestGrid=new BYTE[TestGridSize.x*TestGridSize.y];
}

void cCamera::CalcTestForGrid()
{
	Vect2i TileSize(1<<TestGridShl,1<<TestGridShl);
	calcVisMap(this,TestGridSize,TileSize,pTestGrid,true);

	vector<cCamera*>::iterator it_c;
	FOR_EACH(child,it_c)
	{
		if((*it_c)->GetAttribute(ATTRCAMERA_SHADOWMAP))
			calcVisMap(*it_c,TestGridSize,TileSize,pTestGrid,false);
	}

}

void cCamera::DrawTestGrid()
{
	if(pTestGrid==NULL)
		return;
	int dx=TestGridSize.x,dy=TestGridSize.y;
	int minx=14,miny=639;

	int r,g,b,a=80;
	r=g=b=255;

	int mul=dx>32?2:4;

	for(int y=0;y<dy;y++)
	for(int x=0;x<dx;x++)
	{
		if(pTestGrid[y*dx+x])
		{
			int xx=x*mul+minx,yy=y*mul+miny;
			gb_RenderDevice3D->DrawRectangle(xx,yy,mul,mul,sColor4c(r,g,b,a));
		}
	}
}

void cCamera::AttachChild(cCamera *c)
{
	child.push_back(c);
	c->Parent=this;
	c->RootCamera=RootCamera;
}

void cCamera::DrawShadowDebug()
{
	if(Option_ShowRenderTextureDBG)
	{
		cCamera* pShadow=NULL;
		if(Option_ShowRenderTextureDBG==8)
		{
			DWORD fogenable=gb_RenderDevice3D->GetRenderState(D3DRS_FOGENABLE);
			gb_RenderDevice->SetRenderState(RS_FOGENABLE,FALSE);
			gb_RenderDevice3D->SetPixelShader(NULL);
			gb_RenderDevice3D->SetVertexShader(NULL);
			float mi=0.0f,ma=1.0f;
			gb_RenderDevice3D->DrawSprite(0,0,512,512,
				mi,mi,ma-mi,ma-mi,gb_RenderDevice3D->dtFixed->GetFloatMap());
			gb_RenderDevice->SetRenderState(RS_FOGENABLE,fogenable);
			return;
		}
		if(Option_ShowRenderTextureDBG==5)
		{
			pShadow=FindChildCamera(ATTRCAMERA_REFLECTION);
		}else
		{
			pShadow=FindChildCamera((Option_ShowRenderTextureDBG!=2 && Option_ShowRenderTextureDBG!=6)?ATTRCAMERA_SHADOWMAP:ATTRCAMERA_SHADOW);
		}
		if(pShadow && pShadow->GetRenderTarget())
		{
			DWORD fogenable=gb_RenderDevice3D->GetRenderState(D3DRS_FOGENABLE);
			gb_RenderDevice->SetRenderState(RS_FOGENABLE,FALSE);

			//const int size=256;
			const int size=512;
			if(Option_ShowRenderTextureDBG!=4 && Option_ShowRenderTextureDBG!=6)
			{
				gb_RenderDevice3D->SetPixelShader(NULL);
				gb_RenderDevice3D->SetVertexShader(NULL);
				float mi=0.0f,ma=1.0f;
				gb_RenderDevice3D->DrawSprite(0,0,size,size,
						mi,mi,ma-mi,ma-mi,pShadow->GetRenderTarget(),sColor4c(255,255,255,255),0,ALPHA_BLEND);
			}else
			{
				float mi=0.0f,ma=1.0f;
				int x1=0,y1=0;
				int x2=x1+size,y2=y1+size;
				float u1=mi,v1=mi;
				float du=ma-mi,dv=ma-mi;

				gb_RenderDevice->SetNoMaterial(ALPHA_NONE,MatXf::ID,0);//FIXME
				gb_RenderDevice3D->SetVertexShader(NULL);


				gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
				gb_RenderDevice3D->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
				gb_RenderDevice3D->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

				/*
				  gb_RenderDevice3D->SetTexture(0,gb_RenderDevice3D->dtAdvance->GetTZBuffer());
			    /*/
				  gb_RenderDevice3D->SetTexture(0,pShadow->GetRenderTarget());
				if(Option_ShowRenderTextureDBG==6)
					gb_VisGeneric->GetShaders()->pShowAlpha->Select();
				else
					gb_VisGeneric->GetShaders()->pShowMap->Select();
				/**/


				sColor4c ColorMul=sColor4c(255,255,255,255);
				cVertexBuffer<sVertexXYZWDT1>& BufferXYZWDT1=*gb_RenderDevice3D->GetBufferXYZWDT1();
				sVertexXYZWDT1* v=BufferXYZWDT1.Lock(4);
				v[0].z=v[1].z=v[2].z=v[3].z=0.001f;
				v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
				v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=ColorMul;
				v[0].x=v[1].x=-0.5f+(float)x1; v[0].y=v[2].y=-0.5f+(float)y1; 
				v[3].x=v[2].x=-0.5f+(float)x2; v[1].y=v[3].y=-0.5f+(float)y2; 
				v[0].u1()=u1;    v[0].v1()=v1;
				v[1].u1()=u1;    v[1].v1()=v1+dv;
				v[2].u1()=u1+du; v[2].v1()=v1;
				v[3].u1()=u1+du; v[3].v1()=v1+dv;
				BufferXYZWDT1.Unlock(4);

				BufferXYZWDT1.DrawPrimitive(PT_TRIANGLESTRIP,2);
			}

			gb_RenderDevice3D->SetPixelShader(NULL);
			gb_RenderDevice->SetRenderState(RS_FOGENABLE,fogenable);
		}
	}
}

void cCamera::GetLighting(Vect3f& l)
{
	GetScene()->GetLighting(&l);
}

void cVisGeneric::DrawInfo()
{
	if(Option_ShowType[SHOW_INFO])
	{
		cFont* pFont=gb_VisGeneric->CreateDebugFont();
		gb_RenderDevice->SetFont(pFont);

		char s[128];
		sprintf(s,"1 normal camera - %s",Option_ShowType[SHOW_NORMAL]?"show":"hide");
		gb_RenderDevice->OutText(800,10,s,sColor4f(1,1,0,1));
		
		sprintf(s,"2 shadow camera - %s",Option_ShowType[SHOW_SHADOW]?"show":"hide");
		gb_RenderDevice->OutText(800,30,s,sColor4f(1,1,0,1));

		sprintf(s,"3 reflection camera - %s",Option_ShowType[SHOW_REDLECTION]?"show":"hide");
		gb_RenderDevice->OutText(800,50,s,sColor4f(1,1,0,1));

		sprintf(s,"4 tilemap - %s",Option_ShowType[SHOW_TILEMAP]?"show":"hide");
		gb_RenderDevice->OutText(800,70,s,sColor4f(1,1,0,1));

		sprintf(s,"5 object - %s",Option_ShowType[SHOW_OBJECT]?"show":"hide");
		gb_RenderDevice->OutText(800,90,s,sColor4f(1,1,0,1));

		gb_RenderDevice->SetFont(NULL);
		pFont->Release();
	}	
}

bool cCamera::IsBadClip()
{
	Vect2f sz=RenderSize;
	if(vp.X<0 || vp.Y<0)
		return true;
	if(vp.X+vp.Width>sz.x)
		return true;
	if(vp.Y+vp.Height>sz.y)
		return true;

	return false;
}

///////////////////////////////cCameraPlanarLight/
cCameraPlanarLight::cCameraPlanarLight(cScene *UClass,bool objects_)
:cCamera(UClass)
{
	objects=objects_;
	//SetFoneColor(sColor4c(0,0,0,0));
	SetFoneColor(sColor4c(128,128,128,0));
}

void cCameraPlanarLight::DrawScene()
{
	//gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_ALPHA);
	gb_RenderDevice->SetDrawNode(this);
	gb_RenderDevice3D->SetGlobalLight(NULL);

	gb_RenderDevice->SetRenderState( RS_ZWRITEENABLE, FALSE );
	DWORD ZFUNC=gb_RenderDevice3D->GetRenderState(D3DRS_ZFUNC);
	gb_RenderDevice3D->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );


	DWORD fogenable=gb_RenderDevice3D->GetRenderState(D3DRS_FOGENABLE);
	gb_RenderDevice->SetRenderState(RS_FOGENABLE,FALSE);

	DrawObjectFirst();
	gb_RenderDevice3D->Draw(GetScene(),objects);
	if(GetScene()->GetFogOfWar())
		((cBaseGraphObject*)GetScene()->GetFogOfWar())->Draw(this);

	gb_RenderDevice3D->SetRenderState( D3DRS_ZFUNC, ZFUNC );
	gb_RenderDevice->SetRenderState(RS_FOGENABLE,fogenable);
}

void TempDrawShadow(int width,int height)
{
	gb_RenderDevice3D->SetPixelShader(NULL);
	gb_RenderDevice3D->SetVertexShader(NULL);
	gb_RenderDevice3D->SetVertexDeclaration(sVertexXYZWD::declaration);

	gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_GREATER);
	gb_RenderDevice3D->SetNoMaterial(ALPHA_BLEND,MatXf::ID);

	for(int c=255;c>=0;c-=8)
	{
		sVertexXYZWD Vertex[4];
		float xOfs=0,yOfs=0;
		Vertex[0].x=xOfs;     Vertex[0].y=yOfs;     
		Vertex[1].x=xOfs;     Vertex[1].y=yOfs+height;
		Vertex[2].x=xOfs+width;Vertex[2].y=yOfs;     
		Vertex[3].x=xOfs+width;Vertex[3].y=yOfs+height;
		for(int i=0;i<4;i++)
		{
			Vertex[i].z=Vertex[i].w=c/256.0f;
			Vertex[i].diffuse.set(c,c,c,255);
		}
		gb_RenderDevice3D->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,Vertex,sizeof(Vertex[0]));
	}

	gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
}

void cCamera::DrawObjectNoZ(eSceneNode nType)
{
	camerapass=nType;
	vector<cBaseGraphObject*>& obj=DrawArray[nType];
	if(!obj.empty())
	{
		vector<cBaseGraphObject*>::iterator it;

		DWORD zfunc=gb_RenderDevice3D->GetRenderState(D3DRS_ZFUNC);
		gb_RenderDevice3D->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS);
		DWORD zwrite=gb_RenderDevice3D->GetRenderState(D3DRS_ZWRITEENABLE);
		gb_RenderDevice3D->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
		DWORD fogenable=gb_RenderDevice3D->GetRenderState(D3DRS_FOGENABLE);
//balmer1		gb_RenderDevice3D->SetRenderState(D3DRS_FOGENABLE,FALSE);
		DWORD old_cullmode=gb_RenderDevice3D->GetRenderState(D3DRS_CULLMODE);
		gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);

		FOR_EACH(obj,it)
		{
			(*it)->Draw(this);
		}

		gb_RenderDevice3D->SetRenderState( D3DRS_ZFUNC, zfunc );
		gb_RenderDevice3D->SetRenderState( D3DRS_ZWRITEENABLE, zwrite );
		gb_RenderDevice3D->SetRenderState( D3DRS_FOGENABLE,fogenable);

		gb_RenderDevice->SetRenderState( RS_CULLMODE, old_cullmode );
	}

}

void cCamera::DrawObject(eSceneNode nType)
{
	camerapass=nType;
	gb_RenderDevice->SetRenderState( RS_CULLMODE, -1 );
	vector<cBaseGraphObject*>& obj=DrawArray[nType];
	vector<cBaseGraphObject*>::iterator it;
	FOR_EACH(obj,it)
	{
		(*it)->Draw(this);
	}
}
void cCamera::DrawObject2Pass()
{
	camerapass=SCENENODE_ZPASS;
	vector<cBaseGraphObject*>& obj=DrawArray[SCENENODE_OBJECT_2PASS];
	vector<cBaseGraphObject*>::iterator it;
	//gb_RenderDevice->SetRenderState( RS_CULLMODE,D3DCULL_CW);
	// 1 pass - отрисовка в Z буффер
	gb_RenderDevice3D->SetRenderState(RS_ZWRITEENABLE,FALSE);
	FOR_EACH(obj,it)
	{
		(*it)->Draw(this);
	}
	gb_RenderDevice3D->SetRenderState(RS_ZWRITEENABLE,TRUE);
	// 2 pass - отрисовка в Color буффер
	//camerapass=SCENENODE_OBJECT_2PASS;
	//FOR_EACH(obj,it)
	//{
	//	(*it)->Draw(this);
	//}
}

void cCamera::DrawObjectSpecial(eSceneNode nType)
{
	camerapass=nType;
	vector<cBaseGraphObject*>& obj=DrawArray[nType];

	gb_RenderDevice->SetRenderState( RS_CULLMODE, D3DCULL_NONE );
	sort(obj.begin(),obj.end(),SortObjectSpecial());

	vector<cBaseGraphObject*>::iterator it;
	FOR_EACH(obj,it)
	{
		(*it)->Draw(this);
	}
}

void cCamera::DrawTilemapObject()
{
	bool is_silouette=false;//!DrawArray[SCENENODE_FLAT_SILHOUETTE].empty();

	DWORD cullmode;
	if(is_silouette)
	{
		cullmode=gb_RenderDevice3D->GetRenderState(D3DRS_CULLMODE);
		gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
		gb_RenderDevice3D->SetRenderState(D3DRS_STENCILENABLE,TRUE);
		gb_RenderDevice3D->SetRenderState(D3DRS_STENCILFUNC,D3DCMP_ALWAYS);
		gb_RenderDevice3D->SetRenderState(D3DRS_STENCILFAIL,D3DSTENCILOP_KEEP);
		gb_RenderDevice3D->SetRenderState(D3DRS_STENCILZFAIL,D3DSTENCILOP_INCR);
		gb_RenderDevice3D->SetRenderState(D3DRS_STENCILPASS,D3DSTENCILOP_INCR);
	}

	int number_draw=GetNumberDraw(SCENENODE_OBJECT_TILEMAP);
	for( int nObj=0; nObj<number_draw; nObj++ )
		GetDraw(SCENENODE_OBJECT_TILEMAP,nObj)->Draw(this);

	if(is_silouette)
	{
		gb_RenderDevice3D->SetRenderState(D3DRS_STENCILENABLE,FALSE);
		gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,cullmode);
	}
}

void cCamera::DrawSilhouetteObject()
{
	gb_RenderDevice->SetRenderState( RS_CULLMODE, -1 );
	eSceneNode nType=SCENENODE_FLAT_SILHOUETTE;
	camerapass=nType;

	if(Parent)
	{
		DrawObject(SCENENODE_FLAT_SILHOUETTE);
		return;
	}

	vector<cBaseGraphObject*>& drawList=DrawArray[nType];
	vector<cBaseGraphObject*>::iterator it;

	if(!drawList.empty()) {
		std::vector<ObjectSort> sort_array;
		std::vector<ObjectSort>::iterator sort_it;
		sort_array.reserve(drawList.size());
		FOR_EACH(drawList, it) {
			float distance = Pos.distance((*it)->GetPosition().trans());
			sort_array.push_back(ObjectSort(distance, *it));
		}
		stable_sort(sort_array.begin(), sort_array.end(), ObjectSortByRadius());

		VISASSERT(gb_RenderDevice3D->GetRenderMode()&RENDERDEVICE_MODE_STENCIL);

		gb_RenderDevice3D->SetRenderState(D3DRS_STENCILZFAIL,D3DSTENCILOP_REPLACE);
		gb_RenderDevice3D->SetRenderState(D3DRS_STENCILENABLE,TRUE);
		gb_RenderDevice3D->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		DWORD old_colorwrite=gb_RenderDevice3D->GetRenderState(D3DRS_COLORWRITEENABLE);
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_ALPHA);

		FOR_EACH(sort_array, sort_it) {
			xassert(sort_it->obj->GetKind() == KIND_OBJ_3DX);
			cObject3dx* obj = static_cast<cObject3dx*>(sort_it->obj);

			gb_RenderDevice3D->SetRenderState(D3DRS_STENCILREF,GetSilhouetteStencil(obj->GetSilhouetteIndex()));
			gb_RenderDevice3D->SetRenderState(D3DRS_STENCILZFAIL,D3DSTENCILOP_REPLACE);
			if(obj->GetAttr(ATTR3DX_ALWAYS_FLAT_SILUETTE))
			{
				gb_RenderDevice3D->SetRenderState(D3DRS_STENCILPASS,D3DSTENCILOP_REPLACE);
			}else
			{
				gb_RenderDevice3D->SetRenderState(D3DRS_STENCILPASS,D3DSTENCILOP_KEEP);
				if(obj->QueryVisibleIsVisible())
				{
					gb_RenderDevice3D->SetRenderState(D3DRS_STENCILPASS,D3DSTENCILOP_KEEP);
					gb_RenderDevice3D->SetRenderState(D3DRS_STENCILZFAIL,D3DSTENCILOP_KEEP);
				}else
				{
					gb_RenderDevice3D->SetRenderState(D3DRS_STENCILPASS,D3DSTENCILOP_KEEP);
					gb_RenderDevice3D->SetRenderState(D3DRS_STENCILZFAIL,D3DSTENCILOP_REPLACE);
				}
			}

			
			obj->Draw(this);
			obj->QueryVisible(this);
		}

		gb_RenderDevice3D->SetRenderState(D3DRS_STENCILZFAIL,D3DSTENCILOP_KEEP);
		gb_RenderDevice3D->SetRenderState(D3DRS_STENCILENABLE,FALSE);
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_colorwrite);
	}
}

cCameraMirageMap::cCameraMirageMap(cScene* scene)
: cCamera(scene)
{

}

void cCameraMirageMap::DrawScene()
{
	if(!gb_RenderDevice3D->dtFixed->GetMirageMap()||!GetScene()->GetTileMap())
		return;
	gb_RenderDevice->SetDrawNode(this);
	//gb_RenderDevice3D->SetRenderTarget(gb_RenderDevice3D->dtAdvance->GetMirageMap(),gb_RenderDevice3D->lpZBuffer);
	D3DSURFACE_DESC desc;
	gb_RenderDevice3D->lpZBuffer->GetDesc(&desc);

	RDCALL(gb_RenderDevice3D->lpD3DDevice->Clear(0,NULL,D3DCLEAR_TARGET,0x00808080,0,0));
	//gb_RenderDevice3D->lpD3DDevice->Clear(0,NULL,D3DCLEAR_TARGET,0xFF808080,0,0);
	//gb_RenderDevice3D->lpD3DDevice->Clear(0,NULL,D3DCLEAR_TARGET,0x00FFFFFF,0,0);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
	gb_RenderDevice3D->SetRenderState(RS_FOGENABLE,FALSE);
	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
	//gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE,false);

	gb_RenderDevice->SetRenderState( RS_CULLMODE, D3DCULL_NONE );
	DrawSortObject();
	//RenderDevice->FlushPrimitive3D();
	gb_RenderDevice3D->RestoreRenderTarget();
}

cCameraShadowMap::cCameraShadowMap(cScene* scene)
: cCamera(scene)
{	

}

extern Vect2f debug_zminmax;
void cCameraShadowMap::DrawScene()
{
	xassert(GetAttribute(ATTRCAMERA_SHADOW | ATTRCAMERA_SHADOWMAP));
	if(IsBadClip())	{
		xassert(0 && "Is bad clip");
		return;
	}

	vector<cCamera*>::iterator it_c;
	FOR_EACH(child,it_c)
		(*it_c)->DrawScene();

	gb_RenderDevice->SetDrawNode(this);

	if(!Option_ShowType[SHOW_SHADOW])
		return;

	DWORD old_cullmode=gb_RenderDevice3D->GetRenderState(D3DRS_CULLMODE);
//	gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);//bias должен в другую сторону смотреть.

	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHAREF,0);
	DrawObjectFirst();

	gb_RenderDevice3D->SetGlobalLight(NULL);

	gb_RenderDevice->SetRenderState( RS_ZWRITEENABLE, GetAttr(ATTRCAMERA_NOZWRITE)?FALSE:TRUE );

	float fBiasSlope=0.0f;
    if(gb_RenderDevice3D->dtAdvance && gb_RenderDevice3D->dtAdvance->GetID()==DT_RADEON9700)
        fBiasSlope=0.0f;
    else
        fBiasSlope=2.0f;
	gb_RenderDevice3D->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&fBiasSlope);

	DWORD fogenable=gb_RenderDevice3D->GetRenderState(D3DRS_FOGENABLE);
	if(GetAttribute(ATTRCAMERA_SHADOW|ATTRCAMERA_SHADOWMAP))
		gb_RenderDevice->SetRenderState(RS_FOGENABLE,FALSE);
	
	DrawTilemapObject();
	
	DrawType* draw=draw=gb_RenderDevice3D->dtAdvance;
	draw->BeginDrawShadow();
	
	if(Option_ShowType[SHOW_OBJECT]) {
		if(camerapass==SCENENODE_OBJECT)
		{
			for(int i=0;i<2;i++)
			{
				gb_RenderDevice3D->SetSamplerData(i,sampler_wrap_anisotropic);
			}
		}

		DrawObject(SCENENODE_OBJECT);
		DrawSilhouetteObject();
		DrawObjectSpecial(SCENENODE_OBJECTSPECIAL);
		DrawSortObject();
		if(false)
		{
			gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
			DebugDrawFrustum(sColor4c(0,255,0,255));

			Vect2f old_zplane=Parent->GetZPlane();
			Parent->SetFrustum(0,0,0,&debug_zminmax);
			Parent->DebugDrawFrustum(sColor4c(255,0,0,255));
			Parent->SetFrustum(0,0,0,&old_zplane);
		}
	}

	gb_RenderDevice->FlushPrimitive3D();
	draw->EndDrawShadow();

	if(Option_ShowRenderTextureDBG==3)
		TempDrawShadow(vp.Width,vp.Height);
	
	gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,old_cullmode);
	gb_RenderDevice->SetRenderState(RS_FOGENABLE,fogenable);
}

Vect2f cCamera::CalcZMinZMaxShadowReciver()
{
    Vect2f all(zPlane.y,zPlane.x);//Тут правильно.

    eSceneNode nType[]={SCENENODE_OBJECT,SCENENODE_FLAT_SILHOUETTE};
    for(int itype=0;itype<sizeof(nType)/sizeof(nType[0]);itype++)
    {
        eSceneNode type=nType[itype];
        vector<cBaseGraphObject*>& objects=DrawArray[type];
        vector<cBaseGraphObject*>::iterator it;
        FOR_EACH(objects,it)
        {
            cBaseGraphObject* p=*it;
            if(p->GetKind()==KIND_OBJ_3DX)
            {
                Vect3f pos=p->GetCenterObject();
                float radius=p->GetBoundRadius();
                Vect3f out_pos;

                GetMatrix().xformPoint(pos,out_pos);
                all.x=min(all.x,out_pos.z-radius);
                all.y=max(all.x,out_pos.z+radius);
            }
        }
    }

    all.x=max(all.x,zPlane.x);
    all.y=min(all.y,zPlane.y);

//    if(all.x>all.y) all=zPlane;

    return all;
}

sBox6f cCamera::CalcShadowReciverInSpace(D3DXMATRIX matrix)
{
	sBox6f box;
	box.SetInvalidBox();
    eSceneNode nType[]={SCENENODE_OBJECT,SCENENODE_FLAT_SILHOUETTE};
    for(int itype=0;itype<sizeof(nType)/sizeof(nType[0]);itype++)
    {
        eSceneNode type=nType[itype];
        vector<cBaseGraphObject*>& objects=DrawArray[type];
        vector<cBaseGraphObject*>::iterator it;
        FOR_EACH(objects,it)
        {
            cBaseGraphObject* p=*it;
            if(p->GetKind()==KIND_OBJ_3DX)
            {
				sBox6f bc;
                p->GetBoundBox(bc);
				MatXf mobj=p->GetPosition();
				Vect3f p[8];
				mobj.xformPoint(Vect3f(bc.min.x,bc.min.y,bc.min.z),p[0]);
				mobj.xformPoint(Vect3f(bc.max.x,bc.min.y,bc.min.z),p[1]);
				mobj.xformPoint(Vect3f(bc.min.x,bc.max.y,bc.min.z),p[2]);
				mobj.xformPoint(Vect3f(bc.max.x,bc.max.y,bc.min.z),p[3]);
				mobj.xformPoint(Vect3f(bc.min.x,bc.min.y,bc.max.z),p[4]);
				mobj.xformPoint(Vect3f(bc.max.x,bc.min.y,bc.max.z),p[5]);
				mobj.xformPoint(Vect3f(bc.min.x,bc.max.y,bc.max.z),p[6]);
				mobj.xformPoint(Vect3f(bc.max.x,bc.max.y,bc.max.z),p[7]);
				for(int i=0;i<8;i++)
				{
					Vect3f out_pos,out_pos1;
					D3DXVec3TransformCoord((D3DXVECTOR3*) &out_pos1, (D3DXVECTOR3*)&p[i], &matrix);
					((CMatrix*)&matrix)->xformPoint(p[i],out_pos);
					box.AddBound(out_pos);
				}
            }
        }
    }
	return box;
}

void cCamera::SetZTexture(cTexture* zTexture)
{
	RELEASE(pZTexture);
	pZTexture=zTexture;
	if(pZTexture)
		pZTexture->AddRef();
}

void cCamera::SetFrustumPositionAutoCenter(sRectangle4f& camera_position,float focusx)
{
	Vect2f vSize(camera_position.xmax()-camera_position.xmin(),camera_position.ymax()-camera_position.ymin());
	Vect2f vC = (camera_position.min + camera_position.max)*0.5;
	Clip=sRectangle4f(-vSize.x/2,-vSize.y/2,vSize.x/2,vSize.y/2);
	Center=vC;

	Focus.x=vSize.x*focusx;
	Focus.y=vSize.x*focusx;
	Update();
}

