#include "stdafx.h"
#include "EasyMap.h"
#include "Terra\terra.h"
#include "Render\src\FogOfWar.h"
#include "Render\src\MultiRegion.h"
#include "Render\src\lighting.h"
#include "Water\OrCircle.h"
#include "Water\SkyObject.h"
#include "Render\src\FT_Font.h"
#include "Render\Src\WinVideo.h"
#include "Render\Src\TexLibrary.h"
#include "Render\D3D\D3DRender.h"
#include "VistaRender\postEffects.h"
#include "Render\src\perlin.h"
#include "Render\src\Scene.h"
#include "DebugUtil.h"
#include "Serialization\XPrmArchive.h"
#include "kdw\PropertyEditor.h"
#include "kdw\kdWidgetsLib.h"
#include "Serialization\ResourceSelector.h"
#include "Serialization\RadianWrapper.h"
#include "VistaRender\StreamInterpolation.h"
#include "Terra\vmap.h"
#include "FileUtils\FileUtils.h"
#include "UnicodeConverter.h"
#include "DebugPrm.h"
#include "XMath\SafeMath.h"
#include "Render\Src\TileMap.h"
#include "Render\Src\VisGeneric.h"
#include "UserInterface\UI_Minimap.h"
#include "UserInterface\UI_RenderBase.h"

EasyMap* easyMap;
extern HWND g_hWnd;

cEffect* effect=0;

static const char* cfgName = "EasyMap\\easyMap.cfg";

using namespace FT;
Font* pFont;

Vect2i fog_position(30,30);
int fog_radius=57;

Recti aspectedArea(const Vect2i& size, float aspect)
{
	Recti aspected(size);
	if(float(size.x) / float(size.y) < aspect){
		aspected.height(round(float(size.x) / aspect));
		aspected.top((size.y - aspected.height()) / 2);
	}
	else{
		aspected.width(round(float(size.y) * aspect));
		aspected.left((size.x - aspected.width()) / 2);
	}
	return aspected;
}

Runtime3D* CreateRuntime3D()
{
	return new EasyMap;
}

EasyMap::EasyMap()
{
	easyMap = this; 

	vMap.disableLoadContainerPMO();
	prevtime=0;
	tileMap_=0;
	pFont=0;

	updateRay_ = true;
		 
	cameraAngles_.set(1.06f,1.5707964f);
	cameraPosition_.set(1052,1295,401);

	wireframe=false;
	textTest_=false;
	minimapTest_=0;
	textScale_ = 1.f;

	minimapBorder_ = 0;

	climate=CLIMATE_NORMAL;
	mouse_lbutton=false;
	mouse_rbutton=false;
	logic_quant=0.1f;
	logic_speed=2;
	logic_time=0;
	dayTime_=15;
	dayTimeSpeed_ = 0.8f;

	pFogMap=0;
	fog_handle=-1;
}

EasyMap::~EasyMap()
{
	easyMap = 0;
}

bool EasyMap::init(int xScr,int yScr,bool bwindow)
{
	if(!__super::init(xScr, yScr, bwindow))
		return false;

	gb_VisGeneric->SetEffectLibraryPath("RESOURCE\\FX","RESOURCE\\FX\\Textures");

	gb_VisGeneric->SetEnableBump(true);
	gb_VisGeneric->EnableOcclusion(true);

	pFont = fontManager().createFont("Scripts\\Resource\\fonts\\default.ttf", 10);
	renderDevice->SetDefaultFont(pFont);

	UI_RenderBase::instance().setRenderSize(Vect2i(gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY()));
	UI_RenderBase::instance().setWindowPosition(aspectedArea(UI_RenderBase::instance().renderSize(), 4.f / 3.f));

	const char* worldName = check_command_line("-world");
	if(!worldName)
		worldName = "test";
	Load(worldName);

	return true;
}

void EasyMap::finit()
{
	serialize(XPrmOArchive(cfgName));

	minimap().clearEvents();
	minimap().releaseMapTexture();
	if(minimapBorder_)
		minimapBorder_->Release();
	minimapBorder_ = 0;

	streamLogicInterpolator.clear();
	streamLogicCommand.clear();
	streamLogicPostCommand.clear();

	if(pFogMap)
		pFogMap->deleteVisible(fog_handle);
	delete pFogMap;
	pFogMap=0;

	for(int i=0;i<object_array.size();i++)
		object_array[i]->Release();
	object_array.clear();

	objects_.clear();

	renderDevice->SetDefaultFont(0);
	fontManager().releaseFont(pFont);

	delete environment_;
	RELEASE(tileMap_);

	__super::finit();
}

void EasyMap::Load(const char* worldName)
{
	vMap.load(worldName);
	vMap.WorldRender();

	tileMap_ = scene_->CreateMap(false);
	
	gb_VisGeneric->SetSilhouetteColor(0,Color4c(255,0,0));

	scene_->EnableReflection(true);

	environment_ = new Environment(scene_, tileMap_, true, false, true);
	if(environment_->fogOfWar()){
		pFogMap = environment_->fogOfWar()->CreateMap();
		environment_->fogOfWar()->SelectMap(pFogMap);
	}

	fieldDispatcher_ = new FieldDispatcher(vMap.H_SIZE, vMap.V_SIZE);
	scene_->AttachObj(fieldDispatcher_);

	XPrmIArchive spgArchive;
	if(spgArchive.open(setExtention(XBuffer() < vMap.getWorldsDir() < "\\" < worldName, "spg").c_str())){
		spgArchive.setFilter(SERIALIZE_WORLD_DATA);
		spgArchive.serialize(*environment_, "environment", 0);
		vMap.serializeRegion(spgArchive);
	}
	environment_->environmentTime()->SetTime(dayTime_);

	minimap()
		.water(true)
		.drawEvents(true)
		.viewZone(true)
		.zoneColor(Color4f(0.5f, 1.f, 0.5f))
		.scale(true)
		.rotScale(false)
		.canFog(true)
		.showFog(false)
		.showZones(true)
		.init(Vect2f(vMap.H_SIZE, vMap.V_SIZE), (XBuffer() < vMap.getWorldsDir() < "\\" < worldName < "\\map.tga").c_str());
	minimapBorder_ = GetTexLibrary()->GetElement2D("Resource\\UI\\Textures\\Map_Round.tga");
	minimap().mask(minimapBorder_);
	minimap().setPosition(UI_RenderBase::instance().relativeCoords(Recti(gb_RenderDevice->GetSizeX()-500, 0, 500, 500)));

	//createObject("C:\\Models\\A_HERO_Mammon.3DX", Vect2f(100, 100), true);
	//createObject("c:\\VistaEngine\\Resource\\TerrainData\\Models\\bush_leafy_002_5.3DX", Vect2f(100, 100), true);
	//createObject("Resource\\Models\\corona.3DX", Vect2f(100, 100), false);
	//createObject("Resource\\TerrainData\\Models\\fence_beton_001.3DX", Vect2f(100, 100), false);
	//createObject("Resource\\TerrainData\\Models\\fence_combine2_001.3DX", Vect2f(200, 100), false);
	createObject("Resource\\Models\\II_core_mobile.3DX", Vect2f(100, 100), true);

	//for(int y = 0; y < 100; y++)
	//	for(int x = 0; x < 100; x++)
	//		createObject("Resource\\TerrainData\\Models\\fence_beton_001.3DX", Vect2f(x*10, y*10), true);

	XPrmIArchive ia;
	if(ia.open(cfgName))
		serialize(ia);
}

void EasyMap::createObject(const char* name, const Vect2f& position, bool cobject3dx, float scale)
{
	c3dx* p;
	if(cobject3dx) 
		p = scene_->CreateObject3dx(name);
	else
		p = scene_->CreateSimply3dx(name);
	//p->SetSkinColor(Color4c::WHITE, 0);
	//p->setAttribute(ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE);
	
	MatXf mat = MatXf::ID;
	mat.trans() = To3D(position);
	p->SetScale(scale);
	p->SetPosition(mat);
	p->Update();

	scene_->CreateLogic3dx(name);

	Static3dxFile::convertFile(name);

	//vector<cSimply3dx*> debrieses;
	//scene_->CreateDebrisesDetached(p, debrieses);
	//vector<cSimply3dx*>::iterator i;
	//FOR_EACH(debrieses, i)
	//	scene_->AttachObj(*i);
	//p->Release();

	object_array.push_back(p);
}

void EasyMap::quant()
{
	Runtime3D::quant();

	double curtime=xclock();
	float dt=min(100.0,curtime-prevtime);
	CameraQuant(dt);
	prevtime=curtime;

	Circles::iterator circle;
	FOR_EACH(circles_, circle)
		circleManager_.addCircle(circle->position, circle->radius, circleManagerParam_);

	graphQuant(dt*0.001f);

	if(logic_speed!=3)
		logic_time+=dt*1e-3f;
	bool unlimit=logic_quant==0;
	if(logic_time>=logic_quant || unlimit)
	{
		logicQuant();

		logic_time-=logic_quant;
		if(unlimit)
			logic_time=0;
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
}

void EasyMap::logicQuant()
{
	if(pFogMap)
		pFogMap->quant();

	environment_->logicQuant();
}

void EasyMap::graphQuant(float dt)
{
	scene_->SetDeltaTime(dt*1000);

	float factor = clamp(fabsf(cameraAngles_.x)/(M_PI/2), 0, 1);
	Vect2f zPlane(environment->GetGameFrustrumZMin(), environment->GetGameFrustrumZMaxHorizontal()*factor + environment->GetGameFrustrumZMaxVertical()*(1 - factor));
	camera_->SetFrustum(0, 0,	0, &zPlane);

	Color4c& Color = environment->environmentTime()->GetCurFoneColor();
	gb_RenderDevice->Fill(Color.r,Color.g,Color.b);
	renderDevice->BeginScene();

	environment->environmentTime()->Draw();

	gb_RenderDevice3D->SetRenderState(D3DRS_FILLMODE,wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
	renderDevice->SetFont(pFont);

	Sectors::iterator si;
	FOR_EACH(sectors_, si)
		environment_->fieldOfViewMap()->add(To3D(si->position), si->psi, si->radius, si->sector, si->colorIndex);

	environment->graphQuant(dt, camera_);

	scene_->Draw(camera_);
	
	environment->drawPostEffects(dt, camera_);

	gb_RenderDevice3D->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	Runtime3D::DrawFps(256,0);

	minimapTest(dt);
	//textTest();

	renderDevice->SetFont(0);
//	gb_Camera->DebugDrawFrustum();

//	Sectors::iterator si;
//	FOR_EACH(sectors_, si)
//		si->debugDraw(camera_);

	gb_RenderDevice->FlushPrimitive2D();

	renderDevice->EndScene();
	renderDevice->Flush();
}

void EasyMap::serialize(Archive& ar)
{
	if(ar.isInput()){
		Objects::iterator oi;
		FOR_EACH(objects_, oi)
			environment_->fieldOfViewMap()->remove(oi->model);
	}

	ar.serialize(cameraPosition_, "cameraPosition", "cameraPosition");
	ar.serialize(cameraAngles_, "cameraAngles", "cameraAngles");
	
	if(ar.isOutput())
		dayTime_ = environment_->getTime();
	ar.serialize(dayTime_, "dayTime", "dayTime");
	if(ar.isInput() && environment_)
		environment_->environmentTime()->SetTime(dayTime_);

	ar.serialize(dayTimeSpeed_, "dayTimeSpeed", "dayTimeSpeed");
	ar.serialize(circleManagerParam_, "circleManagerParam", "circleManagerParam");
	ar.serialize(circles_, "circles", "circles");
	ar.serialize(objects_, "objects", "objects");
	ar.serialize(sectors_, "sectors", "sectors");
	ar.serialize(debug_stop_time, "debug_stop_time", "debug_stop_time");

	ar.serialize(terrainType_, "terrainType_", "terrainType_");

	ar.serialize(updateRay_, "updateRay_", "updateRay_");
	ar.serialize(rayStart_, "rayStart_", "rayStart_");
	ar.serialize(rayDir_, "rayDir_", "rayDir_");

	if(ar.isInput()){
		fieldDispatcher_->clear();
		fieldSources_.clear();
		Circles::iterator ci;
		FOR_EACH(circles_, ci)
			fieldSources_.push_back(FieldSource(ci->radius, ci->radius*1.5f, ci->position, Color4c(Color4f(0.6f, 0, 0, 1))));
		FieldSources::iterator fi;
		FOR_EACH(fieldSources_, fi)
			fieldDispatcher_->add(*fi);

		Objects::iterator oi;
		FOR_EACH(objects_, oi)
			environment_->fieldOfViewMap()->add(oi->model);
	}

	ar.serialize(minimapSymbol_, "minimapSymbol", "Символ на миникарту");
}

bool GetK(int key)
{
	return GetAsyncKeyState(key);
}

void EasyMap::CameraQuant(float dt)
{
    float fElapsedTime=dt/1000.0f;

    float fSpeed        = 200.0f*fElapsedTime;

	if(!GetK(VK_SHIFT)){
		if(GetK(VK_CONTROL)){
			float fAngularSpeed = fElapsedTime;
			if(GetK(VK_UP))     cameraAngles_.x+=-fAngularSpeed;
			if(GetK(VK_DOWN))   cameraAngles_.x+=+fAngularSpeed;
			if(GetK(VK_LEFT))   cameraAngles_.y+=-fAngularSpeed;
			if(GetK(VK_RIGHT))  cameraAngles_.y+=+fAngularSpeed;
		}
		else{
			if(GetK(VK_RIGHT))  cameraPosition_+=camera_->GetWorldI()*fSpeed;
			if(GetK(VK_LEFT))   cameraPosition_-=camera_->GetWorldI()*fSpeed;
			if(GetK(VK_UP))     cameraPosition_+=camera_->GetWorldK()*fSpeed;
			if(GetK(VK_DOWN))   cameraPosition_-=camera_->GetWorldK()*fSpeed;
			if(GetK(VK_PRIOR))	cameraPosition_+=camera_->GetWorldJ()*fSpeed;
			if(GetK(VK_NEXT))   cameraPosition_-=camera_->GetWorldJ()*fSpeed;
		}
	}
	else{
		Vect3f dv(0, 0, 0);
		if(GetK(VK_RIGHT))  dv+=camera_->GetWorldI()*fSpeed;
		if(GetK(VK_LEFT))   dv-=camera_->GetWorldI()*fSpeed;
		if(GetK(VK_UP))     dv+=camera_->GetWorldK()*fSpeed;
		if(GetK(VK_DOWN))   dv-=camera_->GetWorldK()*fSpeed;
		if(GetK(VK_PRIOR))	dv+=camera_->GetWorldJ()*fSpeed;
		if(GetK(VK_NEXT))   dv-=camera_->GetWorldJ()*fSpeed;
	}

	Vect3f Target;
	Target.x=cameraAngles_.x;
	Target.y=0;
	Target.z=M_PI/2 - cameraAngles_.y;

	MatXf m = MatXf::ID;
	Mat3f mx(Target.x, X_AXIS),my(Target.y, Y_AXIS),mz(Target.z, Z_AXIS);
	m.rot() = mx*my*mz;
	m *= MatXf(Mat3f::ID, -cameraPosition_);
	setCameraPosition(m);
}

extern bool	Option_shadowTSM;
extern float Option_reflectionOffset;

void EasyMap::onWheel(bool wheelUp)
{
	Vect2f mouse = mousePosRelative();
	mouse += Vect2f(0.5f, 0.5f);
	
	if(minimapTest_ && minimap().hitTest(mouse)){
		if(wheelUp)
			minimap().zoomIn(mouse);
		else
			minimap().zoomOut(mouse);
	}
}

void EasyMap::KeyDown(int key)
{
	switch(key){
	//case VK_F1:
	//	textTest_=!textTest_;
	//	break;
	//case VK_ADD:
	//	if(GetK(VK_SHIFT)){
	//		FontParam prm = pFont->param();
	//		uint8 size = min(pFont->size() + 1, 101);
	//		fontManager().releaseFont(pFont);
	//		pFont = fontManager().createFont("Scripts\\Resource\\fonts\\default.ttf", size, &prm);
	//	}
	//	else
	//		textScale_ *= 1.02f;
	//	break;
	//case VK_SUBTRACT:
	//	if(GetK(VK_SHIFT)){
	//		FontParam prm = pFont->param();
	//		uint8 size = max(pFont->size() - 1, 3);
	//		fontManager().releaseFont(pFont);
	//		pFont = fontManager().createFont("Scripts\\Resource\\fonts\\default.ttf", size, &prm);
	//	}
	//	else
	//		textScale_ /= 1.02f;
	//	break;
	//case VK_MULTIPLY:
	//	textScale_ = 4;
	//	break;

	case 'D':
		if(GetK(VK_CONTROL))
			Option_shadowTSM = !Option_shadowTSM;
		break;

	case 'N':
		if(GetK(VK_CONTROL))
			Option_reflectionOffset +=0.5f;
		else
			Option_reflectionOffset -=0.5f;
		break;

	case 'K':
		gb_RenderDevice->RestoreDeviceForce();
		break;

	case 'W':
		wireframe=!wireframe;
		break;
	case VK_F6:
		profiler_start_stop(PROFILER_REBUILD);
		break;
	case VK_F4:
		if(GetK(VK_SHIFT))
			gb_VisGeneric->editOption();
		else if(GetK(VK_CONTROL)){
			kdw::edit(Serializer(*environment, "", "", SERIALIZE_PRESET_DATA), "Scripts\\TreeControlSetups\\environmentState", kdw::IMMEDIATE_UPDATE | kdw::ONLY_TRANSLATED, g_hWnd);
		}
		else{
			if(kdw::edit(Serializer(*this), "Scripts\\TreeControlSetups\\easyMapEditState", kdw::IMMEDIATE_UPDATE | kdw::ONLY_TRANSLATED, g_hWnd)){
				XPrmOArchive oa(cfgName);
				serialize(oa);
			}
		}
		break;
	}	

	if(key == 'T' && GetK(VK_CONTROL)){
		float cur_time = environment->environmentTime()->GetTime(); 
		if (cur_time > 12 && cur_time < 24)
			environment->environmentTime()->SetTime(1);
		else
			environment->environmentTime()->SetTime(13);
	}
	
	if(key == 'T' && GetK(VK_SHIFT))
		debug_stop_time = !debug_stop_time;

	static cEffect* effect=0;
	if(key=='E')
	{
		if(effect)
		{
			//effect->SetParticleRate(0);
			effect->StopAndReleaseAfterEnd();
			effect=0;
		}
	}
	if(key=='F')
	{
		static bool enable=false;
		enable=!enable;
        //terScene->EnableFogOfWar(enable);
		FieldSources::iterator fi;
		FOR_EACH(fieldSources_, fi)
			if(enable)
				fieldDispatcher_->add(*fi);
			else
				fieldDispatcher_->remove(*fi);
	}

	if(key=='1')
	{
		if(environment_->water())
			environment_->water()->SetRainConstant(+0.03f);
		climate=CLIMATE_HOT;
	}
	if(key=='2')
	{
		if(environment_->water())
			environment_->water()->SetRainConstant(-0.003f);
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
			gb_VisGeneric->SetShadowType(true, 3);
		else
			gb_VisGeneric->SetShadowType(false, 3);
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
		gb_VisGeneric->setTileMapVertexLight(up);
	}

	if(key=='8')
	{
		gb_VisGeneric->SetTilemapDetail(!gb_VisGeneric->GetTilemapDetail());
	}

	if(key=='9')
	{
		scene_->EnableReflection(!scene_->IsReflection());
		water->setTechnique();
	}

	if(key==VK_ADD)
	{
		enviroment_water=min(enviroment_water+1,256);
		if(environment_->water())
			environment_->water()->SetEnvironmentWater(enviroment_water);
	}
	if(key==VK_SUBTRACT)
	{
		enviroment_water=max(enviroment_water-1,0);
		if(environment_->water())
			environment_->water()->SetEnvironmentWater(enviroment_water);
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

	if(key=='R')
	{
		updateRay_ = !updateRay_;
		//renderDevice->RestoreDeviceForce();
	}

	if(key==VK_F9)
	{
		renderDevice->ChangeSize(1024,768,RENDERDEVICE_MODE_WINDOW);
	}

	if(key == 'M')
		minimapTest_ = (minimapTest_ + 1) % 3;
	if(minimapTest_ && GetK(VK_SHIFT)){
		if(key == 'F')
			minimap().showFog(!minimap().isFogOfWar());
		if(key == 'Z')
			minimap().showZones(!minimap().isInstallZones());
		if(key == 'R')
			minimap().setRotate(cycle(minimap().rotationAngle() + 0.02f, 2*M_PI));
	}
}

Vect3f EasyMap::MousePos3D()
{
	Vect2f pos_in(mousePos().x/(float)renderDevice->GetSizeX()-0.5f,
		mousePos().y/(float)renderDevice->GetSizeY()-0.5f);

	Vect3f pos,dir;
	camera_->GetWorldRay(pos_in,pos,dir);
	Vect3f trace;
	//bool b=terScene->TraceSegment(pos,(pos+dir*2000),&trace);
	bool b=scene_->TraceDir(pos,dir,&trace);
	pos=trace;
	return pos;
}

void EasyMap::DrawMousePos()
{
	int dy=pFont->lineHeight();

	Vect3f pos=MousePos3D();

	renderDevice->SetDrawTransform(camera_);

	char str[256];
	sprintf(str,"%.2f,%.2f,%.2f",pos.x,pos.y,pos.z);
	int y=100;
	renderDevice->OutText(20,y,str,Color4f(1,1,0,1));
	y+=dy;

	if(environment_->water() && false)
	{
		float div=1<<cWater::grid_shift;
		Vect2f vel;
		int xx=round(pos.x/div);
		int yy=round(pos.y/div);
		if(xx<0)xx=0;
		if(yy<0)yy=0;
		environment_->water()->GetVelocity(vel,xx,yy);
		char* type="empty";
		cWater::OnePoint& cur=environment_->water()->Get(xx,yy);
		float z=cur.realHeight();
		float uz=cur.underground_z/float(1<<16);
		if(cur.type==cWater::TYPE::type_filled)
			type="filled";
		sprintf(str,"pos=%i,%i,z=%.2f,uz=%.2f,vel=%.2f,%.2f %s %i",xx,yy,z,uz,vel.x,vel.y,type,cur.ambient_type);
		renderDevice->OutText(20,y,str,Color4f(1,1,0,1));
		y+=dy;
	}

	renderDevice->FlushPrimitive3D();
}

void DrawSpecialString(int x,int y,vector<string>& data,int choose)
{
	for(int i=0;i<data.size();i++)
	{
		wstring out(a2w(data[i]));
		x = renderDevice->OutTextLine(x, y, *pFont, out.c_str(), &*out.end(),i==choose?Color4f(1,1,1,1):Color4f(1,1,0,1));
	}
}

void EasyMap::textTest()
{
	int dy=pFont->lineHeight();
	int y=dy;

	if(!textTest_)
	{
		renderDevice->DrawSprite(
			0, 0, pFont->texture()->GetWidth(), pFont->texture()->GetHeight(),
			0, 0, 1, 1,
			const_cast<cTexture*>(pFont->texture()),
			Color4c(255,255,255,255), 0, ALPHA_BLEND, 1);
		renderDevice->DrawRectangle(0, 0, pFont->texture()->GetWidth(), pFont->texture()->GetHeight(), Color4c::WHITE, true);
		y=pFont->texture()->GetHeight();
	}
	
	vector<string> data;
	data.push_back((XBuffer() < "size: " <= pFont->size() < "; ").c_str());
	data.push_back("DrawSpecialString:");
	data.push_back(" второй кусок");
	DrawSpecialString(10, y, data, 0);
	
	y+=dy;
	renderDevice->OutText(10,y,"Простой OutText",Color4f(1,1,0,1));
	
	y += dy;
	char buf[256];
	sprintf(buf, "%.3f OutText\n {вторая строка}", textScale_);
	renderDevice->OutText(10, y, buf, Color4f(1,1,0,1), ALIGN_TEXT_LEFT, ALPHA_BLEND, Vect2f(textScale_, textScale_));
}

EasyMap::Circle::Circle()
{
	position = Vect2f::ZERO;
	radius = 100;
}

void EasyMap::Circle::serialize(Archive& ar)
{
	ar.serialize(position, "position", "position");
	ar.serialize(radius, "radius", "radius");
}

EasyMap::Object::Object()
{
	position = Vect2f::ZERO;
	scale = 1;
}

void EasyMap::Object::serialize(Archive& ar)
{
	ar.serialize(position, "position", "position");
	ar.serialize(scale, "scale", "scale");
	ar.serialize(ModelSelector(name), "name", "name");
	if(ar.isInput()){
		if(!model)
			model = easyMap->scene()->CreateObject3dx(name.c_str());
		model->SetPosition(MatXf(Mat3f::ID, To3D(position)));
		model->SetScale(scale);
		model->Update();
	}
}

EasyMap::Sector::Sector()
{
	position = Vect2f::ZERO;
	psi = 0;
	radius = 100;
	sector = M_PI/4;
	colorIndex = 0;
}

void EasyMap::Sector::serialize(Archive& ar)
{
	ar.serialize(position, "position", "position");
	ar.serialize(RadianWrapper(psi), "psi", "psi");
	ar.serialize(radius, "radius", "radius");
	ar.serialize(RadianWrapper(sector), "sector", "sector");
	ar.serialize(colorIndex, "colorIndex", "colorIndex");
}

void EasyMap::Sector::debugDraw(Camera* camera)
{
	Vect3f point = To3D(position);
	Vect3d delta1(-sinf(psi - sector/2), cosf(psi - sector/2), 0);
	Vect3d delta2(-sinf(psi + sector/2), cosf(psi + sector/2), 0);

	gb_RenderDevice->DrawLine(point, point + delta1*radius, Color4c::RED);
	gb_RenderDevice->DrawLine(point, point + delta2*radius, Color4c::GREEN);
}

void EasyMap::minimapTest(float dt)
{
	if(!minimapTest_)
		return;

	static bool inClick1 = false;
	if(mouse_lbutton){
		if(inClick1)
			return;
		inClick1 = true;
	}
	else
		inClick1 = false;

	static bool inClick2 = false;
	if(mouse_rbutton){
		if(inClick2)
			return;
		inClick2 = true;
	}
	else
		inClick2 = false;

	Vect3f pos(MousePos3D());

	if(mouse_lbutton){
		UI_Minimap::PlaceZone zone(GetK(VK_SHIFT) ? UI_Minimap::PlaceZone::ERASE : UI_Minimap::PlaceZone::DRAW, Vect2f(pos.xi(), pos.yi()), 30.f, Color4c(255, 0, 0, 100));
		minimap().addZone(zone);
	}

	if(mouse_rbutton){
		UI_Minimap::PlaceZone zone(GetK(VK_SHIFT) ? UI_Minimap::PlaceZone::ERASE : UI_Minimap::PlaceZone::DRAW, Vect2f(pos.xi(), pos.yi()), 70.f, Color4c(0, 255, 0, 100));
		minimap().addZone(zone);
	}


	minimap().renderPlazeZones();

	if(!minimap().placeZones_)
		return;

	int pow = 0;
	renderDevice->DrawSprite(
		0, 0, minimap().pzSize_.x << pow, minimap().pzSize_.y << pow,
		0, 0, 1, 1,
		minimap().placeZones_,
		Color4c(255,255,255,255), 0, ALPHA_BLEND, 1);
	renderDevice->DrawRectangle(0, 0, minimap().pzSize_.x << pow, minimap().pzSize_.y << pow, Color4c::WHITE, true);

	if(minimapTest_ > 1)
		minimap().redraw();
}

bool isUnderEditor()
{
	return false;
}

const char* getLocDataPath()
{
	return "English";
}

Vect2f UI_Minimap::screen2minimap(const Vect2f& scr) const
{
	if(inited()){
		Vect3f pos, dir;
		easyMap->camera()->GetWorldRay(scr, pos, dir);

		dir *= pos.z / (dir.z >= 0 ? 0.0001f : -dir.z);
		Vect2f world(pos += dir);

		world /= worldSize_;
		world *= position_.size();
		world += position_.left_top();

		return world;
	}
	return Vect2f(0.f, 0.f);
}

void UI_Minimap::drawStartLocations(float alpha)
{
}

