#include "stdafx.h"
#include "Runtime3D.h"
#include "..\Terra\terra.h"
#include "TerraInterface.inl"
#include <crtdbg.h>
#include "..\Water\Water.h"
#include "..\Water\ice.h"
#include "..\Render\src\FogOfWar.h"
#include "..\Render\src\MultiRegion.h"
#include "..\Render\src\lighting.h"
#include "..\Water\OrCircle.h"
#include "..\Water\SkyObject.h"
#include "..\Render\client\WinVideo.h"
#include "..\Util\Serialization\AttribEditorInterface.h"
//#include "..\Render\client\ForceField.h"
//#include "..\Render\Src\Fields.h"
#include "..\Render\src\postEffects.h"
#include "..\Render\src\perlin.h"
#include "..\Render\inc\IVisD3D.h"
#include "BikeRotateTest.h"
#include <direct.h>


void EdgeDetection(MultiRegion& column,int xmin,int ymin,int dx,int dy,vector<Vect2i>& edge_point);
Vect3f To3D(const Vect2f& pos);

cEffect* effect=NULL;
class IceTerraInterface:public StandartTerraInterface
{
public:
	MultiRegion region_; 
//	Perlin2d perlin;


	IceTerraInterface()
		:region_(vMap.H_SIZE,vMap.V_SIZE,1)
		//,perlin(64,64)
	{
//		perlin.setNumOctave(5);
	}

	class MultiRegion* GetRegion()
	{
		return &region_;
	}	

	void DrawOnRegion(int layer, const Vect2i& point, float radius)
	{
		LockColumn();

		ShapeRegion circle;
		circle.circle(radius,layer+1);
		circle.move(point.x,point.y);
		region_.operate(circle);

		UnlockColumn();
	}
/*
	int GetZ(int x,int y)
	{
		return GetZf(x,y);
	}

	float GetZf(int x,int y)
	{
		return perlin.get(x*0.02f,y*0.02f)*128;
	}
*/
};

cTileMap		*terMapPoint;
cFont* pFont;

const char* editEffectLibraryName(HWND hwnd, const char* initialString)
{
	xassert(0);
	return 0;
}

class Demo3D:public Runtime3D
{
	double prevtime;
    Vect3f	vPosition;
	Vect2f	AnglePosition;

	bool wireframe;

	EffectKey* effect_key;
	cWater* pWater;

	cObject3dx* sphere;
	c3dx* soldier;
	
	bool run_light;
	float light_angle;
	cUnkLight* pSphereLight;
	FOW_HANDLE fog_cursor;

	cLighting* pLighting;
	cEnvironmentEarth* pEnvirinmentEarch;
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
	enum 
	{
		sound_size=4,
	};
	bool show_help;

	enum
	{
		CLIMATE_HOT=0,
		CLIMATE_NORMAL=1,
	} climate;

	int enviroment_water;
	vector<Vect2i> water_source;
	bool mouse_lbutton;
	bool mouse_rbutton;
	float logic_quant;
	int logic_speed;
	float logic_time;

	void OnLButtonDown(){mouse_lbutton=true;};
	void OnLButtonUp(){mouse_lbutton=false;};
	void OnRButtonDown(){mouse_rbutton=true;};
	void OnRButtonUp(){mouse_rbutton=false;};

	void ShowHelp();
	cEnvironmentTime* enviroment_time;
	
	cTemperature* pTemperature;

	void TestCircle();

	vector<cSimply3dx*> simply_array;
	vector<cObject3dx*> object_array;

	void DrawNearIce();

	cFogOfWar* pFogOfWar;
	FogOfWarMap* pFogMap;
	FOW_HANDLE fog_handle;
};

Vect2i fog_position(30,30);
int fog_radius=57;

void TestSaveVRML(const char* filename);

Runtime3D* CreateRuntime3D()
{
	return new Demo3D;
}

Demo3D::Demo3D()
{
	_chdir("J:\\Maelstrom");

//	_controlfp( _controlfp(0,0) & ~(EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL |  EM_INVALID),  MCW_EM ); 
//	_controlfp(_PC_24,_MCW_PC );


	vMap.disableLoadContainerPMO();
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF);
	prevtime=0;
	terMapPoint=NULL;
	pFont=NULL;

	AnglePosition.set(0,M_PI/2);
	//vPosition.set(1298.4243f,1555.3824f,310.27997f);
//	vPosition.set(1428,1690,401);
//	AnglePosition.set(0.50387114f,1.5707964f);
	vPosition.set(1052,1295,401);
	//AnglePosition.set(0.90537894f,1.9203902f);

	wireframe=false;
	effect_key=NULL;
	pWater=NULL;
	sphere=NULL;
	show_help=false;
	climate=CLIMATE_NORMAL;
	mouse_lbutton=false;
	mouse_rbutton=false;
	logic_quant=0.1f;
	logic_speed=2;
	logic_time=0;
	run_light=false;
	light_angle=15;
	enviroment_time=NULL;
	pSphereLight=NULL;
	pTemperature=NULL;
	soldier=NULL;
	pLighting=NULL;

	pFogMap=NULL;
	pFogOfWar=NULL;
	pEnvirinmentEarch=NULL;
	fog_handle=-1;

	gb_VisGeneric->SetUseTextureCache(true);
	gb_VisGeneric->SetUseMeshCache(true);
	gb_VisGeneric->SetFavoriteLoadDDS(true);
	gb_VisGeneric->SetTextureDetailLevel(2);
//	BikeInitTest();
}

Demo3D::~Demo3D()
{
//	BikeReleaseTest();
	Done();

	RELEASE(pFont);
}

void GetNormalArray(TerraInterface *TileMap,char* texture,int pitch,int xstart,int ystart,int xend,int yend,int step);
void GetNormalArrayOld(TerraInterface *TileMap,char* texture,int pitch,int xstart,int ystart,int xend,int yend,int step);
void GetNormalArrayShort(TerraInterface *TileMap,char* texture,int pitch,int xstart,int ystart,int xend,int yend,int step);
//old =2808.476563
//normal=1903.262817
//short=780.543457
//short=250.7
void TestNormal(TerraInterface *terra)
{
	float begin_time=xclock();
	const border=1;
	const size=TILEMAP_SIZE+border*2;
	WORD data[size*size];
	for(int y=0;y<terra->SizeY();y+=TILEMAP_SIZE)
	for(int x=0;x<terra->SizeX();x+=TILEMAP_SIZE)
	{
		GetNormalArrayShort(terra,(char*)data,size*2,x-border,y-border,x+TILEMAP_SIZE+border,y+TILEMAP_SIZE+border,0);
	}

	dprintf("time=%f\n",xclock()-begin_time);
}

void Demo3D::Init()
{
	gb_VisGeneric->SetEffectLibraryPath("RESOURCE\\FX","RESOURCE\\FX\\Textures");
//	terScene->DisableTileMapVisibleTest();
//	TestSaveVRML("resource\\balmer\\wind.3DX");
/*
	gb_VisGeneric->SetShadowType(SHADOW_MAP_SELF,3);
	gb_VisGeneric->SetShadowMapSelf4x4(false);
	gb_VisGeneric->SetTileMapTypeNormal(true);
	gb_VisGeneric->SetTilemapDetail(false);
*/
//	gb_VisGeneric->SetMapLevel(0);
	gb_VisGeneric->SetMapLevel(100);
//	gb_VisGeneric->SetMapLevel(10000);
	vMap.ShadowControl(false);

	gb_VisGeneric->SetEnableBump(true);
	gb_VisGeneric->EnableOcclusion(true);
//	gb_VisGeneric->SetShowRenderTextureDBG(TRUE);

	pFont=gb_VisGeneric->CreateFont("Scripts\\Resource\\fonts\\Arial.font",20);
	terRenderDevice->SetDefaultFont(pFont);

	Load();
}

void Demo3D::Done()
{
	RELEASE(pEnvirinmentEarch);
	if(pFogMap)
		pFogMap->deleteVisible(fog_handle);
	RELEASE(pFogOfWar);
	delete pFogMap;
	pFogMap=NULL;
	for(int i=0;i<simply_array.size();i++)
		simply_array[i]->Release();
	simply_array.clear();

	for(int i=0;i<object_array.size();i++)
		object_array[i]->Release();
	object_array.clear();

	delete enviroment_time;
	enviroment_time=NULL;
	RELEASE(sphere);
	RELEASE(soldier);
	RELEASE(pWater);
	RELEASE(pTemperature);

	RELEASE(pLighting);

	RELEASE(pSphereLight);
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
		//&Vect2f(30.0f,13000.0f)
		&Vect2f(30.0f,2000.0f)
		);

//	gb_Camera->SetFrustrumPositionAutoCenter(sRectangle4f(200/1280.0f,200/1024.0f,300/1280.0f,600/1024.0f),1);

	terScene->SetSun(Vect3f(-1,0,-1),sColor4f(1,1,1,1),sColor4f(1,1,1,1),sColor4f(1,1,1,1));
//*
	vMap.prepare("RESOURCE\\WORLDS");
	
	static int worldnum=0;
	if(worldnum==0)
		vMap.load("C2_M1");
	else
		vMap.load("11");
	worldnum=(worldnum+1)%2;
	
	
	vMap.WorldRender();
	vMap.renderQuant();

	//terMapPoint = terScene->CreateMap(new StandartTerraInterface,0);
	IceTerraInterface* ice=new IceTerraInterface;
	ice->DrawOnRegion(1,Vect2i(1441,1492),200);
//	ice->DrawOnRegion(1,Vect2i(1441,1492),80);
//	ice->DrawOnRegion(1,Vect2i(1241,1492),10);
//	ice->DrawOnRegion(1,Vect2i(1261,1492),10);
//	ice->DrawOnRegion(1,Vect2i(1281,1492),10);
//	ice->DrawOnRegion(1,Vect2i(1301,1492),10);

	terMapPoint = terScene->CreateMap(ice,3);
	terMapPoint->SetZeroplastColor(0,sColor4f(1,1,1));
	terMapPoint->SetZeroplastColor(1,sColor4f(1,0,0));
	
	terScene->GetTilemapDetailTextures().SetTexture(1,"RESOURCE\\TerrainData\\Textures\\Noise\\G_Tex_Details_Ground_006_noise.tga");
//	TestNormal(ice);
/*
	effect_lib.Load("RESOURCE\\FX\\zemitter.effect","RESOURCE\\FX\\Textures");
	effect_key=effect_lib.Get("Effect");

	EmitterKeyLight* key=new EmitterKeyLight;
	key->texture_name="RESOURCE\\EFFECT\\lightmap.tga";
	effect_key->key.push_back(key);

/**/

//	cEffect::test_far_visible=true;
//#define effectname "G_Fx_Volcano_Medium_001"
//#define effectname "G_Fx_Healing_Spot_001"
#define effectname "G_Fx_Hero_Sign_001"
//#define effectname "G_Fx_Bomb_Explosion_Surface_002"
//#define effectname "G_Fx_Hero_Sign_L5"
 
	effect_key=gb_EffectLibrary->Get("RESOURCE\\FX\\"effectname".effect",1,"RESOURCE\\FX\\TEXTURES\\",sColor4c(255,0,0));
	
	gb_VisGeneric->SetSilhouetteColor(0,sColor4c(255,0,0));

	terScene->EnableReflection(false);

//	pFogOfWar=terScene->CreateFogOfWar();
	if(pFogOfWar)
	{
		pFogOfWar->SetFogColor(sColor4c(255,0,0,255));
		pFogOfWar->SetScoutAreaAlpha(255);


		pFogMap=pFogOfWar->CreateMap();
		fog_handle=pFogMap->createVisible();
		pFogMap->moveVisible(fog_handle,fog_position.x,fog_position.y,fog_radius);

		pFogOfWar->SelectMap(pFogMap);
	}

	pWater=new cWater;
	if(pWater)
	{
//		pWater->SetEarthColor(sColor4c(128,128,128));
		pWater->Init(terScene);	
		enviroment_water=20;
		pWater->SetEnvironmentWater(enviroment_water);

		terScene->AttachObj(pWater);

		if(terScene->IsReflection())
			pWater->SetTechnique(cWater::WATER_LINEAR_REFLECTION);
		else
			pWater->SetTechnique(cWater::WATER_REFLECTION);

		sColor4c water_river(56,71,90,30),water_sea(20,24,35, 255);
//		pWater->SetZColorHeight(40);
		
		pWater->ShowEnvironmentWater(true);
	}

//	pEnvirinmentEarch=new cEnvironmentEarth("Scripts\\Resource\\balmer\\ice_cleft.tga");
//	terScene->AttachObj(pEnvirinmentEarch);


	//GetTexLibrary()->GetElement("resource\\Cursors\\cursor_atack.avi");
	if(pWater)
	{
		pTemperature=new cTemperature;
		if(pTemperature)
		{
			pTemperature->Init(Vect2i((int)vMap.H_SIZE,(int)vMap.V_SIZE),5,pWater);
			pTemperature->SetTexture("Scripts\\Resource\\balmer\\snow.tga",
									 "Scripts\\Resource\\balmer\\snow_bump.tga",
									 "Scripts\\Resource\\balmer\\ice_cleft.tga"
									 );
			terScene->AttachObj(pTemperature);
		}
	}

	enviroment_time=new cEnvironmentTime(terScene);
	if(enviroment_time)
	{
		enviroment_time->Init();
		enviroment_time->SetFogCircle(true);
	//	enviroment_time->SetSkyModel("RESOURCE\\TerrainData\\Sky\\Sky_Default.3DX","RESOURCE\\TerrainData\\Sky\\Sky_Default_Clouds.3DX");

		enviroment_time->SetShadowIntensity(0.4f);

		enviroment_time->pSkyObj->AddSkyModel("RESOURCE\\TerrainData\\Sky\\Sky_Clouds_Day_Default.3DX");
		enviroment_time->pSkyObj->SetSkyModel();
	}

	gb_VisGeneric->SetUseLod(true);
	sphere=terScene->CreateObject3dx("RESOURCE\\MODELS\\A_SPAWN PACK.3DX");
	
	MatXf matr=MatXf::ID;
	matr.rot().set(0.3f,X_AXIS);
	matr.trans()=To3D(Vect2f(1247,815));
	
	if(sphere)
	{
		sphere->SetSkinColor(sColor4f(1,0,0,1),"resource\\TerrainData\\Models\\Textures\\Auto_Truck_Industrial2_001.tga");
		sphere->SetPosition(matr);
//		sphere->SetScale(0.1f);
		sColor4f color(1,0,1);
		//sphere->SetColor(&color,&color,0);

		gb_VisGeneric->SetHideFactor(10);
//		float r=sphere->GetBoundRadius();
//		dprintf("radius=%f\n",r);
//		sphere->SetAttr(ATTRUNKOBJ_HIDE_BY_DISTANCE);
//		sphere->GetInterpolation()->SetAnimationGroupChain(0,"walk");

//		sphere->SetAttr(ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE);
//		sphere->SetColor(NULL,&sColor4f(1,1,1,0.2f),NULL);
//		sphere->SetVisibilitySetAlpha(0.5f,1);
//		cTexture* pTex=GetTexLibrary()->GetElement3D("RESOURCE\\TerrainData\\Textures\\Noise\\G_Tex_Details_Ground_006_noise.tga");
//		sphere->SetDiffuseTexture(0,pTex);
//		RELEASE(pTex);
	}

	if(sphere && false)
	{
		Vect3f p0=Vect3f::ZERO;
		sphere->Intersect(p0,matr.trans());
		double time=xclock();
		bool ok=sphere->Intersect(p0,matr.trans());
		const num_iterations=1000;
		for(int i=1;i<num_iterations;i++)
			sphere->Intersect(p0,matr.trans());
		time=xclock()-time;
		if(ok)
			dprintf("intersect true time=%f\n",time);
		else
			dprintf("intersect false time=%f\n",time);
	}

//1070-1080 ms - предыдущая скорость алгоритма.
//768 ms - v1
//692 v3
//645.7 v4
//intel 619
//not sse +special 793
//small sse + special 664
//sse + special 587

	{
//		cObject3dx* sample=terScene->CreateObject3dx("RESOURCE\\Models\\Remnants Jeep New.3DX");
//		sample->SetSkinColor(sColor4f(1,0,0));
		matr.trans().x+=100;
		vector<cSimply3dx*> debrises;
		terScene->CreateDebrisesDetached(sphere,debrises);
//		RELEASE(sample);
		int soldier_num=2;
		for(int i=0;i<debrises.size();i++)
		{
			if(i==soldier_num || true)
			{
				soldier=debrises[i];
				soldier->SetPosition(matr);
				soldier->SetScale(10);//sphere->GetScale());
				soldier->Attach();
			}
			else
			{
				debrises[i]->Attach();
				debrises[i]->Release();
			}

		}
	}

	//soldier=terScene->CreateSimply3dx("resource\\TerrainData\\Models\\tree_palm_001.3dx");
	if(soldier && false)
	{
//		soldier->SetSkinColor(sColor4f(1,0,0));
		matr.rot().set(0.3f,Z_AXIS);
		matr.trans()=To3D(Vect2f(2000,500));
		matr.trans().z+=20;
		soldier->SetPosition(matr);
		soldier->SetScale(1.0f);
	//	soldier->SetCircleShadow(64,-1);//50);

	//	soldier->SetAttr(ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE);
	//	sphere->LinkTo(soldier,0);
	}

	sColor4f diffuse(1,1,1,0.7f);
	sColor4f specular(1,1,1,1);
	matr.rot() = Mat3f::ID;

	if(effect_key && false)
	{
		effect=terScene->CreateEffectDetached(*effect_key,sphere,true);
		effect->SetPosition(matr);
		effect->Attach();

//		effect->LinkToNode(soldier,2);
	}


	//pSphereLight=terScene->CreateLight(ATTRLIGHT_SPHERICAL,"Scripts\\Resource\\balmer\\lightmap.tga");
	if(pSphereLight)
	{
		pSphereLight->SetPosition(MatXf::ID);
		static sLightKey AnimKeys1[2];
		AnimKeys1[0].diffuse.set(255,255,255);
		AnimKeys1[0].radius=128;
		pSphereLight->SetAnimKeys(AnimKeys1,1);
	}
/*
	//
	SNDSoundParam snd;
	snd.sound_name="explosion_heavy";
	snd.is3d=true;
	snd.sound_files.push_back("resource\\sounds\\Eff\\explosion_heavy.wav");
//	snd.max_num_sound=2;
	SNDAddSound(snd);


	Vect3f sound_pos[sound_size]=
	{Vect3f(1224,1224,128),
	 Vect3f(1624,1224,128),
	 Vect3f(1224,1624,128),
	};
	for(int i=0;i<sound_size;i++)
	{
		sound[i].Init("explosion_heavy");
		sound[i].SetPos(sound_pos[i]);
		if(i==1)
		{
			sound[i].SetFrequency(2);
			sound[i].SetVolume(0.5f);
		}
		sound[i].Play();
		Sleep(500);
	}
*/

//	pLighting=new cLighting;
	if(pLighting)
	{
		vector<Vect3f> pos_end;

		int sz=15;
		for(int i=0;i<sz;i++)
		{
			float px=200*sin(i*2*M_PI/sz);
			float py=200*cos(i*2*M_PI/sz);
			pos_end.push_back(Vect3f(1500+px,1445+py,255));
		}
		pLighting->Update(Vect3f(1500,1445,255),pos_end);
		terScene->AttachObj(pLighting);
	}

	vector<string> textureNames;
	textureNames.push_back("RESOURCE\\FX\\Textures\\001.tga");
	textureNames.push_back("RESOURCE\\FX\\Textures\\003.tga");
	textureNames.push_back("RESOURCE\\FX\\Textures\\003b.tga");
	textureNames.push_back("RESOURCE\\FX\\Textures\\047.tga");
	
//	if(enviroment_time)
//		enviroment_time->ExportSunParameters("sun.inl");
	return;
	char *tree_name[]=
	{
#include "m\models.inc"
/*
		"Auto_Car_Personal_001.3DX",
		"Auto_Car_Personal_002.3DX",
		"Auto_Car_Personal_006.3DX",
		"Auto_Car_Personal_007.3DX",
		"Auto_Car_Personal_008.3DX",
*/
	};
	int tree_name_size=sizeof(tree_name)/sizeof(tree_name[0]);
	RandomGenerator rnd;
	int dd=60;
	int dx=(vMap.H_SIZE-1)/dd;

	for(int y=0;y<(vMap.V_SIZE-1);y+=dd)
	for(int x=0;x<(vMap.H_SIZE-1);x+=dd)
	{
		cSimply3dx* ps=NULL;
		cObject3dx* p=NULL;
		//char * name=tree_name[xm_random_generator()%tree_name_size];
		char * name=tree_name[((x/dd)+(y/dd)*dx)%tree_name_size];
		string full_name="RESOURCE\\Models\\";
		full_name+=name;
		float z=terMapPoint->GetTerra()->GetZf(x,y);
		MatXf matr=MatXf::ID;
		matr.trans().set(x+xm_random_generator.frnd()*dd/2,y+xm_random_generator.frnd()*dd/2,z);
		matr.rot().set(xm_random_generator.frand()*M_PI,Z_AXIS);
		float opacity=rnd.frand();
		sBox6f box,boxs;
		float r,rs;

		p=terScene->CreateObject3dx(full_name.c_str());
		if(p)
		{
			r=p->GetBoundRadius();

			p->SetScale(0.2*dd/p->GetBoundRadius());
			p->SetPosition(matr);
		//	p->SetVisibilityGroup0("burnt");
		//	p->ClearAttr(ATTRCAMERA_SHADOWMAP);
		//	p->SetAttr(ATTRUNKOBJ_HIDE_BY_DISTANCE);
			p->GetBoundBox(box);

//			p->SetColor(NULL,&sColor4f(1,1,1,opacity));
			object_array.push_back(p);
		}
		//ps=terScene->CreateSimply3dx(full_name.c_str()/*,((x/dd)&1)?0:"burnt"*/);
		if(ps)
		{
			rs=ps->GetBoundRadius();
			ps->SetScale(0.2*dd/ps->GetBoundRadius());
//			ps->SetOpacity(opacity);
			ps->SetPosition(matr);

			ps->GetBoundBox(boxs);
//			ps->SetAttr(ATTRUNKOBJ_HIDE_BY_DISTANCE);
//			ps->SetAttr(ATTRUNKOBJ_IGNORE);
			simply_array.push_back(ps);
		}

		int k=0;
	}

}

void Demo3D::Quant()
{
	Runtime3D::Quant();

	double curtime=xclock();
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

	if(mouse_lbutton && false)
	{
		Vect3f pos=MousePos3D();
		Vect2i posi(pos.x,pos.y);
		//int dh=mouse_lbutton?2:-2;
		//pWater->AddWaterRect(posi.x,posi.y,dh,16);
		int dh=mouse_lbutton?2:-2;
		int filterHeigh=mouse_lbutton?(255<<VX_FRACTION):0;
	}

	if(mouse_rbutton || mouse_lbutton)
	{
		if(pFogMap)
		{
			Vect3f pos=MousePos3D();
			Vect2i posi(pos.x,pos.y);
			fog_position=posi;
			if(mouse_rbutton)
				fog_radius=rand()%300;
			pFogMap->moveVisibleOuterRadius(fog_handle,fog_position.x,fog_position.y,fog_radius);
			//pFogMap->moveVisible(fog_handle,fog_position.x,fog_position.y,fog_radius);
		}
	}

	vMap.renderQuant();



	if(pSphereLight)
	{
		Vect3f pos=MousePos3D();
		MatXf m=MatXf::ID;
		m.trans()=pos;
		pSphereLight->SetPosition(m);
	}else
	{
//		Vect3f pos=MousePos3D();
//		terScene->AddCircleShadow(pos,8,sColor4c(255,255,255,255));
	}
}

void Demo3D::LogicQuant()
{
	if(pWater)
	{
		//pWater->AddWaterRect(2240,2048,1.5f,1*16);
		for(int i=0;i<water_source.size();i++)
		{
			Vect2i pos=water_source[i];
			//pWater->AddWaterRect(pos.x,pos.y,3.0f,1*16);
			pWater->AddWaterRect(pos.x,pos.y,1.0f,2*16);
		}

		pWater->AnimateLogic();
	}

	if(pTemperature)
	{
		static int count=100;
		if(count>0)
		{
			pTemperature->SetT(1024,1024,10,32);
			pTemperature->SetT(1524,1524,10,64);
			pTemperature->SetT(2024,1024,10,64);
			count--;
		}
		pTemperature->LogicQuant();
	}

	if(pFogOfWar)
	{
		pFogMap->quant();
		pFogOfWar->AnimateLogic();
	}
}

void ShowColumn(MultiRegion& region)
{
	sColor4c color[4]=
	{	
		sColor4c(0,0,0),
		sColor4c(255,255,255),
		sColor4c(255,255,255),
		sColor4c(255,255,255)
	};

	for(int y=0;y<region.height();y++)
	{
		MultiRegion::Line& line=region.lines()[y];
		MultiRegion::Line::iterator it;
		
		FOR_EACH(line,it)
		{
			Vect2f p2(it->x0,y);
			Vect3f pos=To3D(p2);
			unsigned char t=it->type;
			if(t>3)
				t=3;
			terRenderDevice->DrawPoint(pos,color[t]);

		}
	}
}

extern vector<Vect3f> sBumpTile_test_list;

void Demo3D::DrawNearIce()
{
	Vect3f pos=MousePos3D();
	int xcenter=round(pos.x),ycenter=round(pos.y);
	int radius=40,step=1;
	for(int y=ycenter-radius;y<ycenter+radius;y+=step)
	for(int x=xcenter-radius;x<xcenter+radius;x+=step)
	{
		int z=pWater->GetZ(x,y);
		bool ice=pTemperature->checkTileWorld(x,y);
		terRenderDevice->DrawPoint(Vect3f(x,y,z),ice?sColor4c(255,0,0,255):sColor4c(0,255,0,255));
	}

}


void Demo3D::Draw()
{
	sColor4f Color(0.5f,0.5f,0,1.0f);
	if(enviroment_time)
		Color=sColor4f(enviroment_time->GetCurFoneColor());
	terRenderDevice->Fill(Color.GetR(),Color.GetG(),Color.GetB(),0);
	terRenderDevice->BeginScene();
	if(enviroment_time)
		enviroment_time->Draw();

//	terRenderDevice->SetGlobalFog(sColor4f(0.0f,1.0f,0.0f,1),Vect2f(1000,2000));
//	terRenderDevice->SetGlobalFog(sColor4f(0.5f,0.5f,0.5f,1),Vect2f(-1,-2));
	if(enviroment_time)
		terRenderDevice->SetGlobalFog( sColor4f(enviroment_time->GetCurFogColor()),Vect2f(1000,2000));
	else
		terRenderDevice->SetGlobalFog(sColor4f(0.5f,0.5f,0.5f,1),Vect2f(-1,-2));

	gb_RenderDevice3D->SetRenderState(D3DRS_FILLMODE,wireframe?D3DFILL_WIREFRAME:D3DFILL_SOLID);
	terRenderDevice->SetFont(pFont);

	if(enviroment_time)
		enviroment_time->DrawEnviroment(gb_Camera);
	terScene->Draw(gb_Camera);

	if(false)
	{
		MultiRegion& column=*terMapPoint->GetTerra()->GetRegion();
//		ShowColumn(column);

		{
			vector<Vect2i> edge_point;
			EdgeDetection(column,1441-90,1492-90,180,180,edge_point);
			int sz=32;
			int x=19*64-sz/2;
			int y=22*64-sz/2;
			float dx=280;//64+sz;
			EdgeDetection(column,x,y,dx,dx,edge_point);
			sColor4c color(0,255,0);
			for(vector<Vect2i>::iterator it=edge_point.begin();it!=edge_point.end();++it)
			{
				Vect2f p2(it->x,it->y);
				Vect3f pos=To3D(p2);
				terRenderDevice->DrawPoint(pos,color);
			}
		}

		sColor4c color(255,255,255);
		vector<Vect3f>::iterator it;
		FOR_EACH(sBumpTile_test_list,it)
		{
			gb_RenderDevice->DrawPoint(*it,color);
		}
	}
//	TestCircle();
//	if(pTemperature)
//	{
//		pTemperature->DebugShow3();
//	}

	if(pSphereLight)
		terRenderDevice->DrawPoint(pSphereLight->GetPos(),sColor4c(255,255,255,255));
	gb_RenderDevice3D->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
//	DrawNearIce();


	if(fog_handle>=0)
	{
		DrawTerraCircle(Vect2f(fog_position.x,fog_position.y),fog_radius,YELLOW);
	}

	Runtime3D::DrawFps(256,0);

//	BikeDrawTest();
//	ShowHelp();
//	DrawMousePos();

/*
	if(pWater)
	{
		char str[256];
		sprintf(str,"%.2f",pWater->GetChangeStat());
		terRenderDevice->OutText(20,120,str,sColor4f(1,1,0,1));
	}
*/

	terRenderDevice->SetFont(NULL);
//	gb_Camera->DebugDrawFrustum();

	//terScene->GetFieldEffect()->SetActive();
	//terScene->GetFieldEffect()->Draw();

	terRenderDevice->EndScene();
	terRenderDevice->Flush();
}

void Demo3D::Animate(float dt)
{
	terScene->SetDeltaTime(dt);

	static bool first_light=true;
	if(enviroment_time)
	if(run_light || first_light)
	{
		first_light=false;
		float ddt=dt*1e-3f;
		light_angle+=ddt*0.8f;
		if(light_angle>=24)
			light_angle=0;

		enviroment_time->SetTime(light_angle);

	}

	if(sphere)
	{
		static float time=0;
		time+=dt*1e-3f;
		float f=1-fmod(time*0.3f,1);
//		soldier->SetOpacity(f);
//		sphere->SetTextureLerpColor(sColor4f(1,0,0,f));

		int node=sphere->FindNode("windR_01");
		Se3f pos;
		pos.rot()=QuatF::ID;
		pos.trans().set(f*300,0,0);
		if(node>=0)
			sphere->SetUserTransform(node,pos);
	}
	MatXf matr=MatXf::ID;
	matr.trans() = MousePos3D();
	//matr.trans().z = 20;

	if(sphere && false)
	{
		static float phase=0;
		static float iphase=0;

		phase+=dt*1e-3f;
		if(phase>1)
			phase-=1;
		sphere->GetInterpolation()->SetAnimationGroupInterpolation(0,phase);
		sphere->SetAnimationGroupPhase(0,0);
		sphere->GetInterpolation()->SetAnimationGroupPhase(0,0);
//		sphere->SetAnimationGroupPhase(0,phase);
	}
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
/*
	if(key=='1')
		gb_VisGeneric->SetShadowHint(1);
	if(key=='2')
		gb_VisGeneric->SetShadowHint(2);
	if(key=='3')
		gb_VisGeneric->SetShadowHint(3);
	if(key=='4')
		gb_VisGeneric->SetShadowHint(4);
	if(key=='0')
		gb_VisGeneric->SetShadowHint(0);
	if(key=='S')
	{
		static bool self=true;
		self=!self;
		if(self)
		{
			gb_VisGeneric->SetShadowType(SHADOW_MAP_SELF,3);
			vMap.ShadowControl(false);
		}else
		{
			gb_VisGeneric->SetShadowType(SHADOW_MAP,3);
			vMap.ShadowControl(true);
		}
		vMap.WorldRender();
		terScene->GetTileMap()->UpdateMap(Vect2i(0,0),Vect2i((int)vMap.H_SIZE,(int)vMap.V_SIZE));
	}

	if(key=='D')
	{
		static bool self4x4=true;
		self4x4=!self4x4;
		gb_VisGeneric->SetShadowMapSelf4x4(self4x4);
	}

*/
	static cEffect* effect=NULL;
	if(key=='E')
	{
		if(effect)
		{
			//effect->SetParticleRate(0);
			effect->StopAndReleaseAfterEnd();
			effect=NULL;
		}
	}
	if(key=='D')
	{
		effect=terScene->CreateEffectDetached(*effect_key,NULL,true);
		Vect3f pos=MousePos3D();
		MatXf matr(Mat3f::ID,pos);
		matr.rot().set(xm_random_generator.frand()*M_PI,Z_AXIS);

		effect->SetPosition(matr);
		effect->Attach();

	}
	if(key=='F')
	{
		static bool enable=false;
		enable=!enable;
		terScene->EnableFogOfWar(enable);
	}

	if(key=='G')
	{
		pFogOfWar->SetDebugShowScoutmap(!pFogOfWar->GetDebugShowScoutmap());
/*
		static bool set=false;
		set=!set;
		if(set)
			soldier->SetAttr(ATTRUNKOBJ_IGNORE);
		else
			soldier->ClearAttr(ATTRUNKOBJ_IGNORE);
*/
	}
/*
	if(key=='H' && sphere)
	{
		int size=sphere->GetNodeNum();
		vector<Mats> matrix(size);
		Mats m=sphere->GetNodePositionMats(0);
		sphere->SetNodePositionMats(0,m);

		sphere->SetPosition(sphere->GetPosition());
	}
*/	
	if(key==VK_F1)
		show_help=!show_help;
	if(key=='1')
	{
		if(pWater)
			pWater->SetRainConstant(+0.03f);
		climate=CLIMATE_HOT;
	}
	if(key=='2')
	{
		if(pWater)
			pWater->SetRainConstant(-0.003f);
		climate=CLIMATE_NORMAL;
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
		if(pWater)
		{
			if(terScene->IsReflection())
				pWater->SetTechnique(cWater::WATER_LINEAR_REFLECTION);
			else
				pWater->SetTechnique(cWater::WATER_REFLECTION);
		}
	}

	if(key=='O')
	{
		if(sphere)
			sphere->GetInterpolation()->SetAnimationGroupChain(0,"stand");
	}

	if(key=='P')
	{
		if(sphere)
			sphere->GetInterpolation()->SetAnimationGroupChain(0,"walk");
	}

	if(key==VK_ADD)
	{
		enviroment_water=min(enviroment_water+1,256);
		if(pWater)
			pWater->SetEnvironmentWater(enviroment_water);
	}
	if(key==VK_SUBTRACT)
	{
		enviroment_water=max(enviroment_water-1,0);
		if(pWater)
			pWater->SetEnvironmentWater(enviroment_water);
	}

	if(key==VK_DELETE)
	{
		Vect3f pos=MousePos3D();
		Vect2i posi(pos.x,pos.y);
		int ifound=-1;
		int dist_found=INT_MAX;
		for(int i=0;i<water_source.size();i++)
		{
			Vect2i p=water_source[i];
			int dist=sqr(p.x-posi.x)+sqr(p.y-posi.y);
			if(dist<dist_found)
			{
				ifound=i;
				dist_found=dist;
			}
		}

		if(ifound>=0)
		{
			water_source.erase(water_source.begin()+ifound);
		}
	}

	//if(key==VK_F5)
	//{
	//	logic_quant=0.1f;
	//	logic_speed=0;
	//}

	//if(key==VK_F6)
	//{
	//	logic_quant=0.05f;
	//	logic_speed=1;
	//}

	//if(key==VK_F7)
	//{
	//	logic_quant=10000;
	//	logic_speed=2;
	//}
	//if(key==VK_F8)
	//{
	//	Done();
	//	Load();
	//}

	if(key=='T')
	{
		static int t=1;
		t++;
		if(pWater)
			pWater->SetTechnique((cWater::WATER_TYPE)(t%3));
	}

	if(key=='R')
	{
//		run_light=!run_light;
		terRenderDevice->RestoreDeviceForce();
	}

	if(key=='S')
	{
		enviroment_time->Save();
	}

	if(key==VK_F9)
	{
		terRenderDevice->ChangeSize(1024,768,RENDERDEVICE_MODE_WINDOW);
	}
/*
	if(key=='Y')
	{
		Vect3f pos=MousePos3D();
		terScene->CreateEffect();
	}
*/
	if(key=='H')
	{
		Vect3f pos=MousePos3D();
		if(pTemperature)
		{
			pTemperature->SetT(pos.x,pos.y,10,32);
		}
	}
}


void UpdateRegionMap(int x1,int y1,int x2,int y2)
{
	if(terMapPoint) terMapPoint->UpdateMap(Vect2i(x1,y1),Vect2i(x2,y2));
}

int terEnergyBorderCheck(int x,int y)
{
	return 0;
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

	if(pWater && false)
	{
		float div=1<<cWater::grid_shift;
		Vect2f vel;
		int xx=round(pos.x/div);
		int yy=round(pos.y/div);
		if(xx<0)xx=0;
		if(yy<0)yy=0;
		pWater->GetVelocity(vel,xx,yy);
		char* type="empty";
		cWater::OnePoint& cur=pWater->Get(xx,yy);
		float z=cur.realHeight();
		float uz=cur.underground_z/float(1<<16);
		if(cur.type==cWater::TYPE::type_filled)
			type="filled";
		sprintf(str,"pos=%i,%i,z=%.2f,uz=%.2f,vel=%.2f,%.2f %s %i",xx,yy,z,uz,vel.x,vel.y,type,cur.ambient_type);
		terRenderDevice->OutText(20,y,str,sColor4f(1,1,0,1));
		y+=dy;
	}

	if(pTemperature)
	{
		bool is=pTemperature->checkTileWorld(round(pos.x),round(pos.y));
		sprintf(str,is?"ice":"");
		terRenderDevice->OutText(20,y,str,sColor4f(1,1,0,1));
		y+=dy;
	}

	//terRenderDevice->DrawLine(pos-I,pos+I,sColor4c(255,255,255,255));
	//terRenderDevice->DrawLine(pos-J,pos+J,sColor4c(255,255,255,255));
	terRenderDevice->FlushPrimitive3D();

}

void TestSaveVRML(const char* filename)
{
	FILE* f=fopen("out.wrl","wt");
fprintf(f,"#VRML V2.0 utf8\n");

vector<Vect3f> point;
vector<sPolygon> polygon;
//obj->GetAllTriangle(point,polygon);
GetAllTriangle3dx(filename,point,polygon);


fprintf(f,
"DEF Box01 Transform {\n"
"  translation 2.415 -9.662 0\n"
"  rotation -1 0 0 -1.571\n"
"  children [\n"
"    Shape {\n"
"      appearance Appearance {\n"
"        material Material {\n"
"          diffuseColor 0.5686 0.1098 0.6941\n"
"        }\n"
"      }\n"
"      geometry DEF Box01-FACES IndexedFaceSet {\n"
"        ccw FALSE\n"
"        solid TRUE\n"
"        coord DEF Box01-COORD Coordinate { point [\n"
);
	for(int i=0;i<point.size();i++)
	{
		Vect3f p=point[i];
		if(i<point.size()-1)
			fprintf(f,"%f %f %f,\n",p.x,p.y,p.z);
		else
			fprintf(f,"%f %f %f\n",p.x,p.y,p.z);
	}
fprintf(f,
"		  ]\n"
"        }\n"
"        coordIndex [\n"
);
	for(i=0;i<polygon.size();i++)
	{
		sPolygon& p=polygon[i];
		fprintf(f,"%i,%i,%i,-1",p.p1,p.p2,p.p3);
		if(i<point.size()-1)
			fprintf(f,",\n");
		else
			fprintf(f,"\n");
	}
fprintf(f,
"		  ]\n"
"        }\n"
"    }\n"
"  ]\n"
"}\n");
	fclose(f);
}

void DrawSpecialString(int x,int y,vector<string>& data,int choose)
{
	for(int i=0;i<data.size();i++)
	{
		const char* str=data[i].c_str();
		terRenderDevice->OutText(x,y,str,i==choose?sColor4f(1,1,1,1):sColor4f(1,1,0,1));
		x+=terRenderDevice->GetFontLength(str);
	}
}

void Demo3D::ShowHelp()
{
	Vect2f bmin,bmax;
	terRenderDevice->OutTextRect(0,0,"A",ALIGN_TEXT_LEFT,bmin,bmax);
	int dy=bmax.y+1;
	int y=dy;

//	terRenderDevice->OutText(10,y,"K-D Lab Water demo (September 2004)",sColor4f(1,1,1,1));
	y+=dy;
	if(!show_help)
	{
//		terRenderDevice->OutText(10,y,"F1 - show help",sColor4f(1,1,0,1));
		return;
	}
	char str[256];

	vector<string> data;
	data.push_back("1 - hot climate, ");
	data.push_back("2 - temperate climate");
	DrawSpecialString(10,y,data,climate);
	y+=dy;
	sprintf(str,"ADD,SUBSTRACT - ocean level=%i",enviroment_water);
	terRenderDevice->OutText(10,y,str,sColor4f(1,1,0,1));
	y+=dy;
	terRenderDevice->OutText(10,y,"INSERT, DELETE - water supply source",sColor4f(1,1,0,1));
	y+=dy;
	terRenderDevice->OutText(10,y,"Left, right mouse button - up, down ground",sColor4f(1,1,0,1));
	y+=dy;
	data.clear();
	data.push_back("SPEED ");
	data.push_back("F5=1x, ");
	data.push_back("F6=2x, ");
	data.push_back("F7=unlimit, ");
	data.push_back("F8=pause");
	DrawSpecialString(10,y,data,logic_speed+1);
	y+=dy;
	terRenderDevice->OutText(10,y,"Z(ADD), X(DELETE) - swim bubble",sColor4f(1,1,0,1));
	y+=dy;
	terRenderDevice->OutText(10,y,"A - add object",sColor4f(1,1,0,1));
	y+=dy;
}

///////////////
sColor4c WHITE(255, 255, 255);
sColor4c RED(255, 0, 0);
sColor4c GREEN(0, 255, 0);
sColor4c BLUE(0, 0, 255);
sColor4c YELLOW(255, 255, 0);
sColor4c MAGENTA(255, 0, 255);
sColor4c CYAN(0, 255, 255);

Vect3f To3D(const Vect2f& pos)
{
	Vect3f p;
	p.x=pos.x;
	p.y=pos.y;

	int x = round(pos.x), y = round(pos.y);
	if(x >= 0 && x < vMap.H_SIZE && y >= 0 && y < vMap.V_SIZE)
		p.z=vMap.GVBuf[vMap.offsetGBuf(x>>kmGrid,y>>kmGrid)];
	else
		p.z=0;
	return p;
}
ShowDispatcher show_dispatcher;
Vect3f G2S(const Vect3f &vg)
{
	Vect3f pv,pe;
	terRenderDevice->GetDrawNode()->ConvertorWorldToViewPort(&vg,&pv,&pe);
	return Vect3f(pe.x, pe.y, pv.z);
}
void clip_line_3D(const Vect3f &v1,const Vect3f &v2,sColor4c color)
{
	terRenderDevice->DrawLine(v1, v2,color);
}
void clip_circle_3D(const Vect3f& vc, float radius, sColor4c color)
{
	float segment_length = 3;
	int N = round(2*M_PI*radius/segment_length);
	if(N < 10)
		N = 10;
	float dphi = 2*M_PI/N;
	Vect3f v0 = vc + Vect3f(radius,0,0);
	for(float phi = dphi;phi < 2*M_PI + dphi/2; phi += dphi)
	{
		Vect3f v1 = vc + Vect3f(cos(phi), sin(phi),0)*radius;
		terRenderDevice->DrawLine(v0, v1,color);
		v0 = v1;
	}
}
void clip_pixel(int x1,int y1,sColor4c color, int size)
{
	terRenderDevice -> DrawPixel(x1,y1,color);
	if(size){
		terRenderDevice -> DrawPixel(x1 + 1,y1,color);
		terRenderDevice -> DrawPixel(x1,y1 + 1,color);
		terRenderDevice -> DrawPixel(x1 + 1,y1 + 1,color);

		if(size == 2){
			terRenderDevice -> DrawPixel(x1 - 1,y1,color);
			terRenderDevice -> DrawPixel(x1,y1 - 1,color);
			terRenderDevice -> DrawPixel(x1 - 1,y1 - 1,color);
			terRenderDevice -> DrawPixel(x1 - 1,y1 + 1,color);
			terRenderDevice -> DrawPixel(x1 + 1,y1 - 1,color);
			}
		}
}

void ShowDispatcher::Shape::show()
{
	switch(type){
		case Point: {
			Vect3f vs = G2S(point);
			if(vs.z > 10)
				clip_pixel(vs.xi(), vs.yi(), color, 1);
			} break;

		case Text: {
			Vect3f vs = G2S(point);
			terRenderDevice->OutText(vs.xi(), vs.yi(), text.c_str(), sColor4f(color));
			} break;

		case Circle: 
			clip_circle_3D(point, radius, color);
			break;

		case Delta: 
			//clip_line_3D(point1, point1 + point2*show_vector_scale, color);
			xassert(0);
			break;

		case Line: 
			clip_line_3D(point1, point2, color);
			break;

		case Triangle: 
			clip_line_3D(points[0], points[1], color);
			clip_line_3D(points[1], points[2], color);
			clip_line_3D(points[2], points[0], color);
			break;

		case Quadrangle: 
			clip_line_3D(points[0], points[1], color);
			clip_line_3D(points[1], points[2], color);
			clip_line_3D(points[2], points[3], color);
			clip_line_3D(points[3], points[0], color);
			break;

		case ConvexArray:
			//showConvex();
			xassert(0);
			break;
		
		default:
			xassert(0);
		}
}
void ShowDispatcher::draw()
{
	cFont* font = 0;
	if(need_font){
		font = gb_VisGeneric->CreateDebugFont();
		terRenderDevice->SetFont(font);
		need_font = false;
	}

	List::iterator i;
	FOR_EACH(shapes, i)
		i -> show();

	if(font){
		terRenderDevice->SetFont(0);
		font->Release();
	}
}

void DrawSpline(vector<OrCircleSpline>& spline,cTexture* pTexture)
{
	if(spline.empty())
		return;
	DrawStrip strip;
//	gb_RenderDevice3D->SetVertexShader(NULL);
//	gb_RenderDevice3D->SetPixelShader(NULL);
	gb_RenderDevice->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,pTexture);
	sColor4c diffuse(255,255,255);

	strip.Begin();
	int size=4;
	const dz=5;

	sVertexXYZDT1 v1,v2;
	v1.diffuse=v2.diffuse=diffuse;
	OrCircleSpline sp=spline.back();;
	for(int i=0;i<spline.size();i++)
	{
		if(i<spline.size())
			sp=spline[i];
		else
			sp=spline[0];
		Vect3f p;
		p.x=sp.pos.x;
		p.y=sp.pos.y;
		float t=sp.t;

		if(p.x>=0 && p.x<vMap.H_SIZE && p.y>=0 && p.y<vMap.V_SIZE)
			p.z=vMap.GetAltWhole(p.x,p.y)+dz;
		else
			p.z=0;
		Vect3f n;
		n.x=sp.norm.x;
		n.y=sp.norm.y;
		n.z=0;

		float sz=size;
		v1.pos=p;
		v2.pos=p;
		v1.pos+=sz*n;
		v2.pos-=sz*n;

		v1.u1()=v2.u1()=t;
		v1.v1()=0;v2.v1()=1;
		strip.Set(v1,v2);
	}
	strip.End();
}

void Demo3D::TestCircle()
{
	_clearfp();
	Vect2f pos0(512,512);
	Vect2f pos1=mouse_pos;
	Vect2f pos2(651,554);
//	TestIntersect(pos0,100,pos1,150);
//	TestSegment(pos0,100,pos1,150,pos2,90);

	OrCircle circles;
	circles.clear();
/*
	circles.add(pos0,100);
	circles.add(pos1,120);
	circles.add(pos2,90);
/*/
/*
	RandomGenerator rnd;
	for(int i=0;i<50;i++)
	{
		circles.add(Vect2f(rnd.frand()*400+100,rnd.frand()*400+100),rnd.frand()*70+10);
	}
	circles.add(pos1,120);
/**/
//*

	for(int iy=0;iy<1;iy++)
	for(int ix=0;ix<3;ix++)
	{
		circles.add(Vect2f(ix*80+100,iy*80+100),41);
	}

	circles.add(Vect2f(100,100),41);

//	circles.add(pos1,120);
/**/
/*
circles.add(Vect2f(292,334),67);
circles.add(Vect2f(222,305),60);
circles.add(Vect2f(161,373),78);
circles.add(Vect2f(212,262),61);
circles.add(Vect2f(343,309),120);
*/
	circles.calc();

	cTexture* pTexture=GetTexLibrary()->GetElement3D("Scripts\\Resource\\Textures\\Region.tga");
//	circles.testDraw();
	list<vector<OrCircleSpline> > splines;
	//circles.get_spline(splines,5);
	circles.get_circle_segment(splines,8,5);
	
	list<vector<OrCircleSpline> >::iterator itv;
	FOR_EACH(splines,itv)
	{
		vector<OrCircleSpline>& spline=*itv;
		DrawSpline(spline,pTexture);

//		vector<Vect3f>::iterator it;
//		FOR_EACH(spline,it)
//		{
//			Vect3f& pos=*it;
//			gb_RenderDevice->DrawPixel(pos.x,pos.y,sColor4c(255,255,255));
//		}

	}
	RELEASE(pTexture);

}

void DrawTerraCircle(Vect2f pos,float radius,sColor4c color)
{
	int segments=max(radius/3.0f,8.0f);
	float addition=2;

	Vect3f prev=To3D(Vect2f(pos.x+radius,pos.y));
	prev.z+=addition;

	for(int i=1;i<=segments;i++)
	{
		float f=(2*M_PI*i)/segments;
		Vect2f p(pos.x+radius*cos(f),pos.y+radius*sin(f));
		Vect3f p3=To3D(p);
		p3.z+=addition;
		terRenderDevice->DrawLine(prev,p3,color);
		prev=p3;
	}
	terRenderDevice->FlushPrimitive3D();
}

