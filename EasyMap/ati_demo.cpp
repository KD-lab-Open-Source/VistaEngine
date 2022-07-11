#include "stdafx.h"
#include "Runtime3D.h"
#include "..\render\src\ZIPStream.h"
#include <crtdbg.h>
#include "..\Render\src\MultiRegion.h"
#include "..\Util\Serialization\AttribEditorInterface.h"
#include "..\Render\src\perlin.h"
#include "..\Render\inc\IVisD3D.h"
#include "bink.h" // binkw32.lib, binkw32.dll

class BinkSimplePlayer
{
public:
	BinkSimplePlayer();
	~BinkSimplePlayer();

	static void preInit();

	bool open(const char* bink_file);
	void quant();
	
	const char* getBinkFileName() const { return binkFile_.c_str(); }

	void setPhase(float phase);
	float getPhase() const { return pBinkInfo_->Frames ? float(pBinkInfo_->FrameNum) / float(pBinkInfo_->Frames) : 1.f; }

	void setPause(bool pause) { BinkPause(pBinkInfo_, pause); }
	bool getPause() const { return pBinkInfo_->Paused; }

	void setVolume(float vol);

	cTexture* getTexture() const { return pTextureBink_; }
	Vect2i getSize() const { return Vect2i((int)pBinkInfo_->Width, (int)pBinkInfo_->Height); }

	bool isEnd() const { xassert(pBinkInfo_); return pBinkInfo_->FrameNum == pBinkInfo_->Frames; }

	U32 flags() const { return pBinkInfo_->BinkType; } 

private:
	void release();
	void computeFrame();

	string binkFile_;
	HBINK pBinkInfo_;
	cTexture* pTextureBink_;
};

void BinkSimplePlayer::preInit()
{
//	BinkSoundUseDirectSound(SNDGetDirectSound());
}

BinkSimplePlayer::BinkSimplePlayer()
{
	pBinkInfo_ = 0;
	pTextureBink_ = 0;
}

BinkSimplePlayer::~BinkSimplePlayer()
{
	release();
}

void BinkSimplePlayer::release()
{
	if(pBinkInfo_)
	{
		BinkSetVolume(pBinkInfo_, 0, 0);
		BinkClose(pBinkInfo_);
		pBinkInfo_ = 0;
		binkFile_.clear();
	}

	RELEASE(pTextureBink_);
}

bool BinkSimplePlayer::open(const char* bink_file)
{
	release();
	
	if((pBinkInfo_ = BinkOpen(bink_file, BINKALPHA)) == NULL)
	{
		pBinkInfo_=0;
		return false;
	}

	binkFile_ = bink_file;

	if(!pBinkInfo_->Frames || !pBinkInfo_->Width || !pBinkInfo_->Height){
		release();
		return false;
	}

	int dx_real = Power2up(pBinkInfo_->Width);
	int dy_real = Power2up(pBinkInfo_->Height);
//	int dx_real = (pBinkInfo_->Width);
//	int dy_real = (pBinkInfo_->Height);

	if((pTextureBink_ = gb_VisGeneric->CreateTexture(dx_real, dy_real, false)) == NULL){
		release();
		return false;
	}

	return true;
}

void BinkSimplePlayer::computeFrame()
{
	xassert(pBinkInfo_);
	if(isEnd())
		return;

	BinkDoFrame(pBinkInfo_);

	int pitch = 0;
	BYTE* ptr = pTextureBink_->LockTexture(pitch, Vect2i(0, 0), Vect2i((int)pBinkInfo_->Width, (int)pBinkInfo_->Height));

	BinkCopyToBuffer(pBinkInfo_, ptr, pitch, pBinkInfo_->Height, 0, 0, BINKSURFACE32A);

	pTextureBink_->UnlockTexture();
	
	if(!isEnd())
		BinkNextFrame(pBinkInfo_);
}

void BinkSimplePlayer::setPhase(float phase)
{
	xassert(pBinkInfo_);
	BinkGoto(pBinkInfo_, clamp(clamp(phase, 0.f, 1.f) * (pBinkInfo_->Frames - 1), 1, pBinkInfo_->Frames), 0);
	computeFrame();
}

void BinkSimplePlayer::setVolume(float vol)
{
	xassert(pBinkInfo_);
	const float amplification = 0.8f;

	if(vol < amplification)
		vol = vol / amplification * 32768;
	else
		vol = 32767 + (vol - amplification) / (1.f - amplification) * 32768;

	BinkSetVolume(pBinkInfo_, 0, round(vol));
}

void BinkSimplePlayer::quant()
{
	xassert(pBinkInfo_);
	if(BinkWait(pBinkInfo_))
		return;
	computeFrame();
}

class IceTerraInterface:public TerraInterface
{
public:
	Perlin2d perlin;

	IceTerraInterface()
		:perlin(64,64)
	{
		perlin.setNumOctave(5);
	}

	void postInit(class cTileMap* tm){}

	int SizeX(){return 2048;}
	int SizeY(){return 2048;}

	int GetZ(int x,int y)
	{
		return GetZf(x,y);
	}

	float GetZf(int x,int y)
	{
		return perlin.get(x*0.02f,y*0.02f)*128;
	}

	void GetZMinMax(int tile_x,int tile_y,int tile_dx,int tile_dy,BYTE& out_zmin,BYTE& out_zmax)
	{
		BYTE zmin=255,zmax=0;
		int shift=2;

		int dx=tile_dx>>shift,dy=tile_dy>>shift;

		int x_start=tile_x>>shift,y_start=tile_y>>shift;
		int x_end=x_start+dx,y_end=y_start+dy;
		for(int y=y_start;y<y_end;y++)
		{
			for(int x=x_start;x<x_end;x++)
			{
				BYTE z=GetZ(x,y);
				if(z<zmin)
					zmin=z;
				if(z>zmax)
					zmax=z;
			}
		}

		out_zmin=zmin;
		out_zmax=zmax;
	}

	virtual class MultiRegion* GetRegion(){return NULL;}

	virtual void GetTileColor(char* Texture,DWORD pitch,int xstart,int ystart,int xend,int yend,int step)
	{
		for(int y = ystart; y < yend; y += step)
		{
			DWORD* tx=(DWORD*)Texture;
			for (int x = xstart; x < xend; x += step)
			{
				DWORD color=0xFF809080;

				*tx = color;
				tx++;
			}
			Texture += pitch;
		}
		
	}

	virtual void GetTileZ(char* Texture,DWORD pitch,int xstart,int ystart,int xend,int yend,int step)
	{
		int size=pitch>>2;
		xassert((yend+step-ystart)<=size*step);
		xassert((xend+step-xstart)<=size*step);
		for(int y = ystart,ybuffer=0; y < yend; y += step,ybuffer++)
		{
			int * tx=(int*)(Texture+ybuffer*pitch);
			for (int x = xstart; x < xend; x += step,tx++)
			{
				int z=int(GetZf(x,y)*255);
				*tx=z;
			}
		}
	}
};

cTileMap		*terMapPoint;
cFont* pFont;

class Demo3D:public Runtime3D
{
	double prevtime;
    Vect3f	vPosition;
	Vect2f	AnglePosition;

	bool wireframe;

	bool run_light;
	float light_angle;
public:
	Demo3D();
	~Demo3D();
	void Init();
	void Quant();

	void Animate(float dt);
	void Draw();

	void CameraQuant(float dt);
	void DrawMousePos();
	void KeyDown(int key);

	Vect3f MousePos3D();

	void LogicQuant();

	void Load();
	void Done();
protected:
	bool mouse_lbutton;
	bool mouse_rbutton;
	float logic_quant;
	int logic_speed;
	float logic_time;

	BinkSimplePlayer player;

	void OnLButtonDown(){mouse_lbutton=true;};
	void OnLButtonUp(){mouse_lbutton=false;};
	void OnRButtonDown(){mouse_rbutton=true;};
	void OnRButtonUp(){mouse_rbutton=false;};

	class cWaterReflection* pWater;
};

class cWaterReflection:public cBaseGraphObject
{
	enum {reflection_z=200};
public:
	cWaterReflection();
	~cWaterReflection();
	void PreDraw(cCamera *pCamera)
	{
		pCamera->AttachNoRecursive(SCENENODE_OBJECTSPECIAL,this);
		cCamera* pReflection=pCamera->FindChildCamera(ATTRCAMERA_REFLECTION);
		if(pReflection)
			pReflection->SetZTexture(pWaterZ);
		IParent->SetZReflection(reflection_z);
	}
	void Draw(cCamera *pCamera);
	void Animate(float dt) { animate_time+=dt*1e-3f; }
	virtual const MatXf& GetPosition() const {return MatXf::ID;};
protected:
	class VSWater* vsShader;
	class PSWater* psShader;
	cTexture *pBump,*pBump1;
	float animate_time;
	cTexture* pWaterZ;
};

Runtime3D* CreateRuntime3D()
{
	return new Demo3D;
}

Demo3D::Demo3D()
{
	_controlfp( _controlfp(0,0) & ~(EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL |  EM_INVALID),  MCW_EM ); 
	_controlfp(_PC_24,_MCW_PC );

//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF);
	prevtime=0;
	terMapPoint=NULL;
	pFont=NULL;

//	AnglePosition.set(0,M_PI/2);
	AnglePosition.set(0.68803710f,3.1725435f);
	vPosition.set(1052,1295,401);

	wireframe=false;
	mouse_lbutton=false;
	mouse_rbutton=false;
	logic_quant=0.1f;
	logic_speed=2;
	logic_time=0;
	run_light=false;
	light_angle=15;

	gb_VisGeneric->SetUseTextureCache(false);
	gb_VisGeneric->SetUseMeshCache(true);
	gb_VisGeneric->SetFavoriteLoadDDS(true);
	pWater=NULL;
}

Demo3D::~Demo3D()
{
	Done();

	RELEASE(pFont);
}

void Demo3D::Init()
{
	gb_VisGeneric->SetEffectLibraryPath("RESOURCE\\FX","RESOURCE\\FX\\Textures");
//	terScene->DisableTileMapVisibleTest();
/*
	gb_VisGeneric->SetShadowType(SHADOW_MAP_SELF,3);
	gb_VisGeneric->SetShadowMapSelf4x4(false);
	gb_VisGeneric->SetTileMapTypeNormal(true);
	gb_VisGeneric->SetTilemapDetail(false);
*/
//	gb_VisGeneric->SetMapLevel(0);
	gb_VisGeneric->SetMapLevel(100);
//	gb_VisGeneric->SetMapLevel(10000);

	gb_VisGeneric->SetEnableBump(true);
	gb_VisGeneric->EnableOcclusion(true);
//	gb_VisGeneric->SetShowRenderTextureDBG(TRUE);

	pFont=gb_VisGeneric->CreateFont("Scripts\\Resource\\fonts\\Arial.font",20);
	terRenderDevice->SetDefaultFont(pFont);

	Load();
}

void Demo3D::Done()
{
	RELEASE(pWater);
	RELEASE(terMapPoint);

	RELEASE(gb_Camera);
	RELEASE(terScene);
}

void Demo3D::Load()
{
	// создание сцены
	terScene=gb_VisGeneric->CreateScene(); 
	// создание камеры
	gb_Camera=terScene->CreateCamera();
	gb_Camera->SetAttr(ATTRCAMERA_PERSPECTIVE); // перспектива
	MatXf CameraMatrix;
	Identity(CameraMatrix);
	Vect3f CameraPos(0,0,-512);
	SetPosition(CameraMatrix,CameraPos,Vect3f(0,0,0));
	SetCameraPosition(gb_Camera,CameraMatrix);

	gb_Camera->SetFrustum(							// устанавливается пирамида видимости
		&Vect2f(0.5f,0.5f),							// центр камеры
		&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),		// видимая область камеры
		&Vect2f(1.f,1.f),							// фокус камеры
		&Vect2f(30.0f,13000.0f)
		//&Vect2f(30.0f,2000.0f)
		);

//	gb_Camera->SetFrustrumPositionAutoCenter(sRectangle4f(200/1280.0f,200/1024.0f,300/1280.0f,600/1024.0f),1);

	terScene->SetSun(Vect3f(-1,0,-1),sColor4f(1,1,1,1),sColor4f(1,1,1,1),sColor4f(1,1,1,1));
	terScene->EnableReflection(true);
	
	//terMapPoint = terScene->CreateMap(ice,3);
	if(terMapPoint)
	{
		IceTerraInterface* ice=new IceTerraInterface;
		terMapPoint->SetZeroplastColor(0,sColor4f(1,1,1));
		terMapPoint->SetZeroplastColor(1,sColor4f(1,0,0));
		
		terScene->GetTilemapDetailTextures().SetTexture(1,"RESOURCE\\TerrainData\\Textures\\Noise\\G_Tex_Details_Ground_006_noise.tga");
	}

//	gb_VisGeneric->EnableSilhouettes(true);
	gb_VisGeneric->SetSilhouetteColor(0,sColor4c(255,0,0));

//	player.open("resource\\Video\\nVidia.bik");
	player.open("resource\\Video\\KD.bik");
	

	gb_VisGeneric->SetUseLod(true);

	if(terMapPoint)
	{
		pWater=new cWaterReflection;
		terScene->AttachObj(pWater);
	}
}

void Demo3D::Quant()
{
	Runtime3D::Quant();

	double curtime=clockf();
	float dt=min(100.0,curtime-prevtime);
	CameraQuant(dt);
	Animate(dt);
	prevtime=curtime;

	Draw();

	if(logic_speed!=3)
		logic_time+=dt*1e-3f;
	bool unlimit=logic_quant==0;
	if(logic_time>=logic_quant || unlimit)
	{
		LogicQuant();

		logic_time-=logic_quant;
		if(unlimit)
			logic_time=0;
	}
}

void Demo3D::LogicQuant()
{
}

void Demo3D::Draw()
{
	sColor4f Color(0.2f,0.2f,1.0f,1.0f);
	terRenderDevice->Fill(Color.GetR(),Color.GetG(),Color.GetB(),0);
	terRenderDevice->BeginScene();
//	terRenderDevice->SetGlobalFog(sColor4f(0.0f,1.0f,0.0f,1),Vect2f(1000,2000));
	terRenderDevice->SetGlobalFog(sColor4f(0.5f,0.5f,0.5f,1),Vect2f(-1,-2));

	gb_RenderDevice3D->SetRenderState(D3DRS_FILLMODE,wireframe?D3DFILL_WIREFRAME:D3DFILL_SOLID);
	terRenderDevice->SetFont(pFont);

	if(terMapPoint)
		terScene->Draw(gb_Camera);

	gb_RenderDevice3D->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
	player.quant();
	if(player.isEnd())
		PostMessage(g_hWnd,WM_QUIT,0,0);

	gb_RenderDevice3D->DrawSprite(0,0,gb_RenderDevice3D->GetSizeX(),gb_RenderDevice3D->GetSizeY(),0,0,
		player.getSize().x/(float)player.getTexture()->GetWidth(),
		player.getSize().y/(float)player.getTexture()->GetHeight(),
		player.getTexture());

	Runtime3D::DrawFps(256,0);

//	DrawMousePos();

	terRenderDevice->SetFont(NULL);
//	gb_Camera->DebugDrawFrustum();

	terRenderDevice->EndScene();
	terRenderDevice->Flush();
}

void Demo3D::Animate(float dt)
{
	terScene->SetDeltaTime(dt);
}

bool GetK(int key)
{
	return GetAsyncKeyState(key);
}

void Demo3D::CameraQuant(float dt)
{
    float fElapsedTime=dt/1000.0f;

    float fSpeed        = 800.0f*fElapsedTime;

	if(GetK(VK_CONTROL))
	{
		float fAngularSpeed = 4.0f*fElapsedTime;
		if(GetK(VK_UP))     AnglePosition.x+=-fAngularSpeed;
		if(GetK(VK_DOWN))   AnglePosition.x+=+fAngularSpeed;
		if(GetK(VK_LEFT))   AnglePosition.y+=-fAngularSpeed;
		if(GetK(VK_RIGHT))  AnglePosition.y+=+fAngularSpeed;
	}else
	{

		if(GetK(VK_RIGHT))  vPosition+=gb_Camera->GetWorldI()*fSpeed;
		if(GetK(VK_LEFT))   vPosition-=gb_Camera->GetWorldI()*fSpeed;
		if(GetK(VK_UP))     vPosition+=gb_Camera->GetWorldK()*fSpeed;
		if(GetK(VK_DOWN))   vPosition-=gb_Camera->GetWorldK()*fSpeed;
		if(GetK(VK_PRIOR))	vPosition+=gb_Camera->GetWorldJ()*fSpeed;
		if(GetK(VK_NEXT))   vPosition-=gb_Camera->GetWorldJ()*fSpeed;
	}

	Vect3f Target;
	Target.x=AnglePosition.x;
	Target.y=0;
	Target.z=M_PI/2 - AnglePosition.y;

	MatXf m = MatXf::ID;
	Mat3f mx(Target.x, X_AXIS),my(Target.y, Y_AXIS),mz(Target.z, Z_AXIS);
	m.rot() = mx*my*mz;
	::Translate(m,-vPosition);
	SetCameraPosition(gb_Camera,m);
}

void Demo3D::KeyDown(int key)
{
	//Temp Added by @!!ex
	if(key=='W')
	{
		wireframe=!wireframe;
	}

	if(key=='4')
	{
		gb_VisGeneric->SetShadowTSM(!gb_VisGeneric->GetShadowTSM());
	}

	if(key=='5')
	{
		static bool up=true;
		up=!up;
		if(up)
			gb_VisGeneric->SetShadowType(SHADOW_MAP_SELF,3);
		else
			gb_VisGeneric->SetShadowType(SHADOW_NONE,3);
	}
	if(key=='6')
	{
		static bool up=false;
		up=!up;
		gb_VisGeneric->SetShadowMapSelf4x4(up);
	}
	if(key=='7')
	{
		static bool up=false;
		up=!up;
		gb_VisGeneric->SetTileMapTypeNormal(up);
	}

	if(key=='8')
	{
		gb_VisGeneric->SetTilemapDetail(!gb_VisGeneric->GetTilemapDetail());
	}

	if(key=='9')
	{
		terScene->EnableReflection(!terScene->IsReflection());
	}

	if(key==VK_F5)
	{
		logic_quant=0.1f;
		logic_speed=0;
	}

	if(key==VK_F6)
	{
		logic_quant=0.05f;
		logic_speed=1;
	}

	if(key==VK_F7)
	{
		logic_quant=10000;
		logic_speed=2;
	}
	if(key==VK_F8)
	{
		Done();
		Load();
	}

	if(key=='R')
	{
//		run_light=!run_light;
		terRenderDevice->RestoreDeviceForce();
	}

	if(key==VK_F9)
	{
		terRenderDevice->ChangeSize(1024,768,RENDERDEVICE_MODE_WINDOW);
	}
}


void UpdateRegionMap(int x1,int y1,int x2,int y2)
{
	if(terMapPoint) terMapPoint->UpdateMap(Vect2i(x1,y1),Vect2i(x2,y2));
}

Vect3f Demo3D::MousePos3D()
{
	Vect2f pos_in(mouse_pos.x/(float)terRenderDevice->GetSizeX()-0.5f,
		mouse_pos.y/(float)terRenderDevice->GetSizeY()-0.5f);

	Vect3f pos,dir;
	gb_Camera->GetWorldRay(pos_in,pos,dir);
	Vect3f trace;
	//bool b=terScene->TraceSegment(pos,(pos+dir*2000),&trace);
	bool b=terScene->TraceDir(pos,dir,&trace);
	pos=trace;
	return pos;
}

void Demo3D::DrawMousePos()
{
	Vect2f bmin,bmax;
	terRenderDevice->OutTextRect(0,0,"A",ALIGN_TEXT_LEFT,bmin,bmax);
	int dy=bmax.y+1;

	Vect3f pos=MousePos3D();
	cCamera* pCamera=gb_Camera;

	terRenderDevice->SetDrawTransform(pCamera);
	Vect3f I=pCamera->GetWorldI()*10.0f,J=pCamera->GetWorldJ()*10.0f;

	char str[256];
	sprintf(str,"%.2f,%.2f,%.2f",pos.x,pos.y,pos.z);
	int y=100;
	terRenderDevice->OutText(20,y,str,sColor4f(1,1,0,1));
	y+=dy;

	terRenderDevice->DrawLine(pos-I,pos+I,sColor4c(255,255,255,255));
	terRenderDevice->DrawLine(pos-J,pos+J,sColor4c(255,255,255,255));
	terRenderDevice->FlushPrimitive3D();

}

///////////////

Vect3f To3D(const Vect2f& pos)
{
	if(!terMapPoint)
		return Vect3f(pos.x,pos.y,0);
	Vect3f p;
	p.x=pos.x;
	p.y=pos.y;
	TerraInterface* t=terMapPoint->GetTerra();

	int x = round(pos.x), y = round(pos.y);
	if(x >= 0 && x < t->SizeX() && y >= 0 && y < t->SizeY())
		p.z=t->GetZf(x,y);
	else
		p.z=0;
	return p;
}

AttribEditorInterface& attribEditorInterface()
{
    xassert(0);
    return *(AttribEditorInterface*)(0);
}


cWaterReflection::cWaterReflection()
:cBaseGraphObject(0)
{
	psShader=new PSWater;
	vsShader=new VSWater;
	psShader->SetTechnique(4);
	vsShader->SetTechnique(4);
	vsShader->Restore();
	psShader->Restore();

	pBump=GetTexLibrary()->GetElement3D("Scripts\\Resource\\balmer\\shader\\waves.dds");
	pBump1=GetTexLibrary()->GetElement3D("Scripts\\Resource\\balmer\\shader\\waves1.dds");
	animate_time=0;

	pWaterZ=NULL;
	pWaterZ=GetTexLibrary()->CreateTexture(129,129,SURFMT_A8L8,true);
	int pitch_reflection=0;
	BYTE* pDataReflection=NULL;
	pDataReflection=pWaterZ->LockTexture(pitch_reflection);

	for(int y=0;y<pWaterZ->GetHeight();y++)
	{
		WORD* p=(WORD*)(pDataReflection+y*pitch_reflection);
		for(int x=0;x<pWaterZ->GetWidth();x++,p++)
		{
			*p=reflection_z<<8;
		}
	}

	pWaterZ->UnlockTexture();
}

cWaterReflection::~cWaterReflection()
{
	RELEASE(pWaterZ);
	RELEASE(pBump);
	RELEASE(pBump1);
	delete psShader;
	delete vsShader;
}

void cWaterReflection::Draw(cCamera *pCamera)
{
	cD3DRender* rd=gb_RenderDevice3D;
	int old_zwrite=rd->GetRenderState(RS_ZWRITEENABLE);;
	rd->SetRenderState(RS_ZWRITEENABLE,FALSE);
	cQuadBuffer<sVertexXYZDT1>* quad=gb_RenderDevice->GetQuadBufferXYZDT1();
	rd->SetNoMaterial(ALPHA_BLEND, MatXf::ID);
	Vect2f size(terMapPoint->GetTerra()->SizeX(),terMapPoint->GetTerra()->SizeY());

	vsShader->SetSpeedSky(Vect2f(2.0f/size.x,2.0f/size.y),Vect2f(0,fmodf(animate_time*0.01,1.0f)));
	vsShader->EnableZBuffer(false);
	psShader->EnableZBuffer(false);
	sColor4f plain_reflection_color(1,1,1,1);
	psShader->SetReflectionColor(plain_reflection_color);
	vsShader->Select();
	psShader->Select();

	float time_x=fmodf(animate_time*0.03,1.0f);
	float time_y=fmodf(animate_time*0.02,1.0f);
	float time_x1=fmodf(-animate_time*0.022,1.0f);
	float time_y1=fmodf(-animate_time*0.033,1.0f);
	const float speed_scale=5e-3f;
	const float speed_scale1=7e-3f;
	cCamera* pReflection=pCamera->FindChildCamera(ATTRCAMERA_REFLECTION);
	vsShader->SetMirrorMatrix(pReflection);
	vsShader->SetSpeed(Vect2f(speed_scale,speed_scale),Vect2f(time_x,time_y));
	vsShader->SetSpeed1(Vect2f(speed_scale1,speed_scale1),Vect2f(time_x1,time_y1));

	rd->SetTexture(0,pBump);
	rd->SetTexture(1,pBump1);
	if(pReflection)
		rd->SetTexture(2,pReflection->GetRenderTarget());

	gb_RenderDevice3D->SetSamplerData( 0,sampler_wrap_linear);
	gb_RenderDevice3D->SetSamplerData( 1,sampler_wrap_linear);
	gb_RenderDevice3D->SetSamplerData( 2,sampler_clamp_linear);
	gb_RenderDevice3D->SetSamplerData( 3,sampler_clamp_linear);

	Vect3f min,max;
	min.set(0,0,reflection_z);
	max.set(size.x,size.y,min.z);
	quad->BeginDraw();
	sVertexXYZDT1 p,*v;
	p.diffuse.set(128,128,128,128);
	p.GetTexel().set(0,0);
	v=quad->Get();
	p.pos.set(min.x,min.y,max.z);v[0]=p;
	p.pos.set(min.x,max.y,max.z);v[1]=p;
	p.pos.set(max.x,min.y,max.z);v[2]=p;
	p.pos.set(max.x,max.y,max.z);v[3]=p;
	quad->EndDraw();
	rd->SetRenderState(RS_ZWRITEENABLE,old_zwrite);
}
