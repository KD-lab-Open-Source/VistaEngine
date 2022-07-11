#include "StdAfxRD.h"
#include "scene.h"
#include "TileMap.h"
#include "cCamera.h"
#include "TexLibrary.h"
#include "D3DRender.h"
#include "VisGeneric.h"
#include "FogOfWar.h"
#include "Render\3dx\Lib3dx.h"
#include "ClippingMesh.h"
#include "Terra\vmap.h"
#include "FileUtils\FileUtils.h"

bool cScene::is_sky_cubemap=true;

#ifdef C_CHECK_DELETE
static FILE* gb_objnotfree=0;
static void SaveKindObjNotFree(FILE* f,vector<cCheckDelete*>& data);
void WriteStack(FILE* f,cCheckDelete* p)
{
	string s;
	p->stack.GetStack(s);
	if(!s.empty())
		fprintf(f,s.c_str());
}
#endif

cScene::cScene()
{
	Size.set(0,0);
	dTime=0;
	dTimeInt=0;

	shadowEnabled_ = false;
	SetShadowIntensity(Color4f(0.5f,0.5f,0.5f,1));

	fixShadowBox.min.set(0,0,0);
	fixShadowBox.max.set(1,1,1);
	fixShaddowLightMatrix = MatXf::ID;

	tileMap_=0;
	TileNumber.set(0,0);
	tile_size=0;
	pFogOfWar=0;

	shadow_camera_offset.set(0, 0, 0);

	shadowCamera_ = new CameraShadowMap(this);
	lightCamera_ = new CameraPlanarLight(this,false);
	lightObjectsCamera_ = new CameraPlanarLight(this,true);
	mirageCamera_ = new CameraMirageMap(this);
	floatZBufferCamera_ = new Camera(this);
	pReflectionRenderTarget=0;
	reflectionCamera_=0;
	pReflectionZBuffer=0;
	sun_direction.set(-1,0,-1);
	sun_direction.normalize();
	separately_sun_dir_shadow=false;
	sun_dir_shadow = sun_direction;
	sun_ambient.set(1,1,1,1);
	sun_diffuse.set(1,1,1,1);
	sun_specular.set(1,1,1,1);
	pSkyCubemap=0;
	enable_fog_of_war=true;
	in_update_list=false;
	zreflection_=0;
	enable_reflection=false;
	prev_graph_logic_quant=0;

	hide_lights=false;
	hide_selfillumination=false;

	circle_shadow_intensity.set(255,255,255,255);
}

cScene::~cScene()
{
	RELEASE(pSkyCubemap);
	DeleteAutoObject();
	Size.set(0,0);
	
#ifdef C_CHECK_DELETE
	vector<cCheckDelete*> data;
	{
		for(int isimply=0;isimply<simply_objects.size();isimply++)
		{
			ListSimply3dx& ls=simply_objects[isimply];
			if(!ls.objects.empty())
				data.push_back(ls.pStatic);
		}
		for(int i=0;i<grid.size();i++)
			data.push_back(grid[i]);
		for(int i=0;i<GetNumberLight();i++)
			data.push_back(GetLight(i));
	}

	if(!data.empty())
	{
		if(!gb_objnotfree)
			gb_objnotfree=fopen("obj_notfree.txt","wt");
		if(gb_objnotfree)
		{
			fprintf(gb_objnotfree,"--------------current cScene stack\n");
			WriteStack(gb_objnotfree,this);

			SaveKindObjNotFree(gb_objnotfree,data);
			fflush(gb_objnotfree);
		}

		xassert(0 && "object not released see obj_notfree.txt");
	}
#endif

	RELEASE(shadowCamera_);
	RELEASE(lightCamera_);
	RELEASE(lightObjectsCamera_);
	RELEASE(mirageCamera_);
	RELEASE(floatZBufferCamera_);
	RELEASE(pReflectionRenderTarget);
	RELEASE(reflectionCamera_);
	RELEASE(pReflectionZBuffer);
	xassert(pFogOfWar==0);
}

static void SaveKindObjNotFree(FILE* f,vector<class cCheckDelete*>& data)
{
#ifdef C_CHECK_DELETE
	typedef StaticMap<string,int> idmap;
	idmap ids;
	vector<cCheckDelete*>::iterator it;
	FOR_EACH(data,it)
	{
		const type_info& ti=typeid(**it);
		ids[ti.name()]++;
	}

	fprintf(f,"-------------------------All object stat\n");
	{
		idmap::iterator it;
		FOR_EACH(ids,it)
		{
			idmap::value_type& v=*it;
			fprintf(f,"%3i - %s\n",v.second,v.first.c_str());
		}
	}

	bool first;

	first=true;
	FOR_EACH(data,it)
	{
		cCheckDelete* p=*it;
		cObject3dx* node=dynamic_cast<cObject3dx*>(p);
		if(node)
		{
			if(first)
			{
				fprintf(f,"-------------------------cObject3dx stat\n");
				first=false;
			}
			fprintf(f,"%s (%s)\n",node->GetFileName(),node->GetStatic()->is_logic?"logic":"graphics");
			WriteStack(f,p);
		}
	}

	first=true;
	FOR_EACH(data,it)
	{
		cCheckDelete* p=*it;
		cStatic3dx* node=dynamic_cast<cStatic3dx*>(p);
		if(node)
		{
			if(first)
			{
				fprintf(f,"-------------------------cStatic3dx stat\n");
				first=false;
			}
			fprintf(f,"%s (%s)\n",node->fileName(),node->is_logic?"logic":"graphics");
			WriteStack(f,p);
		}
	}

	first=true;
	FOR_EACH(data,it)
	{
		cCheckDelete* p=*it;
		cStaticSimply3dx* node=dynamic_cast<cStaticSimply3dx*>(p);
		if(node)
		{
			if(first)
			{
				fprintf(f,"-------------------------cStaticSimply3dx stat\n");
				first=false;
			}

			if(node->visibleGroupName.empty())
				fprintf(f,"%s\n",node->file_name.c_str());
			else
				fprintf(f,"%s %s\n",node->file_name.c_str(),node->visibleGroupName.c_str());
			WriteStack(f,p);
		}
	}

	first=true;
	FOR_EACH(data,it)
	{
		cCheckDelete* p=*it;
		cTexture* node=dynamic_cast<cTexture*>(p);
		if(node)
		{
			if(first)
			{
				fprintf(f,"-------------------------cTexture stat\n");
				first=false;
			}
			if(node->name())
				fprintf(f,"%s\n",node->name());
			else
				fprintf(f,"unknown name\n");
			WriteStack(f,p);
		}
	}

	first=true;
	FOR_EACH(data,it)
	{
		cCheckDelete* p=*it;
		cShaderStorageInternal* node=dynamic_cast<cShaderStorageInternal*>(p);
		if(node)
		{
			if(first)
			{
				fprintf(f,"-------------------------cShaderStorageInternal stat\n");
				first=false;
			}
			fprintf(f,"%s\n",node->GetFileName());
			WriteStack(f,p);
		}
	}

	first=true;
	FOR_EACH(data,it)
	{
		cCheckDelete* p=*it;
		cEffect* node=dynamic_cast<cEffect*>(p);
		if(node)
		{
			if(first)
			{
				fprintf(f,"-------------------------cEffect stat\n");
				first=false;
			}
			if(!node->check_file_name.empty())
				fprintf(f,"%s : %s\n",node->check_file_name.c_str(),node->check_effect_name.c_str());
			else
				fprintf(f,"unknown name\n");
			WriteStack(f,p);
		}
	}
#endif
}

void SaveKindObjNotFree()
{
#ifdef C_CHECK_DELETE
	if(!gb_objnotfree)
	{
		if(!cCheckDelete::GetDebugRoot())
		{
			DeleteFile("obj_notfree.txt");
			return;
		}

		gb_objnotfree=fopen("obj_notfree.txt","wt");
		if(!gb_objnotfree)
			return;
	}
	vector<cCheckDelete*> data;
	for(cCheckDelete* p=cCheckDelete::GetDebugRoot();p;p=p->GetDebugNext())
		data.push_back(p);

	fprintf(gb_objnotfree,"-----------finnaly stat\n");
	SaveKindObjNotFree(gb_objnotfree,data);
	fclose(gb_objnotfree);
	gb_objnotfree=0;
	xassert(0 && "Not all graph object is delete. See obj_notfree.txt");
#endif
}

void cScene::Compact()
{ 
	pLibrary3dx->Compact();
	pLibrarySimply3dx->Compact();
	GetTexLibrary()->Compact();
	RemoveEmptyStaticSimply3dx();
}

void cScene::Animate()
{
	start_timer_auto();

	MTAuto enter(lock_draw);

	// анимация объектов
	for(sGrid2d::iterator it=grid.begin();it!=grid.end();++it){
		BaseGraphObject* p=*it;
		if(p && p->getAttribute(ATTRUNKOBJ_DELETED)==0) 
			p->Animate(dTime);
	}
	if(tileMap_)
		tileMap_->Animate(dTime);

	// анимация источников света
	for(int i=0;i<GetNumberLight();i++)
		if(GetLight(i)&&GetLight(i)->getAttribute(ATTRUNKOBJ_DELETED)==0) 
			GetLight(i)->Animate(dTime);

	BuildTree();
	CaclulateLightAmbient();
}

void cScene::SetDeltaTime(float dTime_)
{ 
	dTime=dTime_;
	dTimeInt=round(dTime);
}

void cScene::UpdateLists(int cur_quant)
{
	in_update_list=true;
	mtUpdate(cur_quant);
	in_update_list=false;
}

void cScene::Draw(Camera* camera)
{
	start_timer_auto();

	MTAuto enter(lock_draw);

	RemoveEmptyStaticSimply3dx();

	//Неплохо бы автоматизировать этот процесс для всех child камер.
	if(shadowCamera_)
		shadowCamera_->ClearParent();

	//D3DSURFACE_DESC desc;
	//gb_RenderDevice3D->lpBackBuffer->GetDesc(&desc);
	//gb_RenderDevice3D->dtAdvance->CreateMirageMap(desc.Width,desc.Height);
	camera->SetSecondRT(gb_RenderDevice3D->GetAccessibleZBuffer());//Криво, для демы.

//	unsigned int fp=_controlfp(0,0);
//	_controlfp( _PC_24,  _MCW_PC ); 
	//PreDraw
	if(GetFogOfWar()){
		gb_RenderDevice3D->SetFogOfWar(true);
		gb_RenderDevice3D->fog_of_war_color=Color4f(GetFogOfWar()->GetFogColor());
	}

/*
Если используется вариант удаления через несколько логических квантов, то когда счетчик сбрасывается,
нужно удалить все предыдущие объекты в очереди на удаление.
*/
	int graph_logic_quant=gb_VisGeneric->GetGraphLogicQuant();
	if(graph_logic_quant>=prev_graph_logic_quant)
		UpdateLists(graph_logic_quant);
	else
		UpdateLists(INT_MAX);
	prev_graph_logic_quant=graph_logic_quant;
			
	xassert(camera->scene()==this);

	Animate();
	int i;

	camera->PreDrawScene();//Очистка буферов в основном.
	if(shadowCamera_)
		shadowCamera_->PreDrawScene();
	if(lightCamera_)
		lightCamera_->PreDrawScene();
	if(lightObjectsCamera_)
		lightObjectsCamera_->PreDrawScene();
	if(reflectionCamera_)
		reflectionCamera_->PreDrawScene();
	if(mirageCamera_)
		mirageCamera_->PreDrawScene();
	if(floatZBufferCamera_)
		floatZBufferCamera_->PreDrawScene();

	if(tileMap_){
		AddShadowCamera(camera);
		AddReflectionCamera(camera);
		AddMirageCamera(camera);
		if(gb_VisGeneric->GetFloatZBufferType())
			AddFloatZBufferCamera(camera);
		camera->EnableGridTest(TileNumber.x,TileNumber.y,tile_size);
		tileMap_->PreDraw(camera);
	}

	for(i=0;i<grid.size();i++){
		BaseGraphObject* obj=grid[i];
		if(obj && obj->getAttribute(ATTRUNKOBJ_IGNORE)==0) 
			obj->PreDraw(camera);
	}

	for(i=0;i<GetNumberLight();i++)
		if(GetLight(i)&&GetLight(i)->getAttribute(ATTRLIGHT_DIRECTION)==0) 
			GetLight(i)->PreDraw(camera);

	for(i=0;i<simply_objects.size();i++){
		cStaticSimply3dx* pStatic=simply_objects[i].pStatic;
		pStatic->pActiveSceneList=&simply_objects[i].objects;
		pStatic->setScene(this);
		pStatic->PreDraw(camera);
	}

	if(Option_shadowEnabled && shadowCamera_->GetParent() && tileMap_){
		if(Option_shadowTSM)
			fixShadowMapCameraTSM(camera, shadowCamera_);
		else
			FixShadowMapCamera(camera, shadowCamera_);
	}

	//Draw
	xassert(camera->scene()==this);

	Camera cameraToStore(this);
	Camera* cameraToDebug = 0;
	switch(Option_cameraDebug){
	case 1:
		cameraToDebug = shadowCamera_;
		break;
	case 2:
		cameraToDebug = reflectionCamera_;
		break;
	case 3:
		cameraToDebug = lightCamera_;
		break;
	case 4:
		cameraToDebug = lightObjectsCamera_;
		break;
	}
	if(cameraToDebug){
		camera->SetCopy(&cameraToStore);
		cameraToDebug->SetCopy(camera, false);
	}

	camera->DrawScene();

	if(cameraToDebug)
		cameraToStore.SetCopy(camera);

	gb_RenderDevice3D->SetFogOfWar(false);
	gb_RenderDevice->SetClipRect(0,0,gb_RenderDevice->GetSizeX(),gb_RenderDevice->GetSizeY());
	circle_shadow.clear();
}

bool ClampRay(Vect3f begin,Vect3f dir,
			  const sBox6f& box,
			  Vect3f& start,Vect3f& finish,
			  bool clamp_by_dir)
{
	start=finish=begin;
	float tmin,tmax;
	tmin=1;
	tmax=0;
	float dir_norm=dir.norm();
	const float eps=1e-4f;
	if(dir_norm<eps || dir_norm>1e6f)
	{
		xassert(0);
		return false;
	}

	dir/=dir_norm;

	//begin+dir*t=out
	//t=(out-begin)/dir

	bool initialized=false;
	float t1,t2;

	for(int i=0;i<3;i++)
	{
		xassert(box.min[i]<box.max[i]);

		if(fabsf(dir[i])>eps)
		{
			t1=(box.min[i]-begin[i])/dir[i];
			t2=(box.max[i]-begin[i])/dir[i];
			if(dir[i]<0)
				swap(t1,t2);

			if(initialized)
			{
				tmin=max(tmin,t1);
				tmax=min(tmax,t2);
			}else
			{
				tmin=t1;
				tmax=t2;
				initialized=true;
			}
		}else
		{
			int p=(int)begin[i];
			if(p<=box.min[i] || p>=box.max[i])
				return false;
		}
	}

	if(!initialized)
		return false;

	if(tmin<0)
		tmin=0;

	if(clamp_by_dir && tmax>dir_norm)
		tmax=dir_norm;

	if(tmax-tmin<eps)
		return false;

	start=begin+dir*tmin;
	finish=begin+dir*tmax;

	return true;
}

bool cScene::TraceDir(const Vect3f& pStart,const Vect3f& pDir,Vect3f *pTrace)
{
	return TraceUnified(pStart,pDir,pTrace,false);
}

bool cScene::TraceSegment(const Vect3f& pStart,const Vect3f& pFinish,Vect3f *pTrace)
{//Эта функция работает некорректно, так как в TraceUnified неправильное условие выхода из цикла.
	//xassert(0);
	return TraceUnified(pStart,pFinish-pStart,pTrace,true);
}

#define PREC_TRACE_RAY						17
bool cScene::TraceUnified(const Vect3f& in_start,const Vect3f& in_dir,Vect3f *pTrace,bool clamp_by_dir)
{
	start_timer_auto();

	if (!tileMap_){
		//xassert(TileMap);
		return false;
	}
	if(pTrace)
		pTrace->set(0,0,0);

	Vect3f start,finish;
	sBox6f box;
	box.min.set(0,0,0);
	box.max.set(float(vMap.H_SIZE - 1),float(vMap.V_SIZE - 1), 511.0f);

	Vect3f pStart,pFinish;
	if(!ClampRay(in_start,in_dir,
			  box,pStart,pFinish,clamp_by_dir))
			  return false;

	// Алгоритм прохода
	float dx = pFinish.x-pStart.x, dy = pFinish.y-pStart.y, dz = pFinish.z-pStart.z;
	float dxAbs = fabsf(dx), dyAbs = fabsf(dy), dzAbs=fabsf(dz);
	int dx_,dy_,dz_;
	if(dzAbs>dxAbs && dzAbs>dyAbs)
		dx_=round(dx*(1<<PREC_TRACE_RAY)/dzAbs),dy_=round(dy*(1<<PREC_TRACE_RAY)/dzAbs),dz_=((dz>=0)?1:-1)<<PREC_TRACE_RAY;
	else
	if(dxAbs>dyAbs)
		dy_=round(dy*(1<<PREC_TRACE_RAY)/dxAbs),dz_=round(dz*(1<<PREC_TRACE_RAY)/dxAbs),dx_=((dx>=0)?1:-1)<<PREC_TRACE_RAY;
	else
	if(dyAbs>FLT_EPS)
		dx_=round(dx*(1<<PREC_TRACE_RAY)/dyAbs),dz_=round(dz*(1<<PREC_TRACE_RAY)/dyAbs),dy_=((dy>=0)?1:-1)<<PREC_TRACE_RAY;
	else
	{
//		VISASSERT(0);
		if(pTrace) pTrace->set(pStart.x,pStart.y,vMap.getZf(round(pStart.x),round(pStart.y)));
		return true;
	}

	int xb_=round(pStart.x*(1<<PREC_TRACE_RAY)),yb_=round(pStart.y*(1<<PREC_TRACE_RAY)),zb_=round(pStart.z*(1<<PREC_TRACE_RAY));
	// Переводим размеры в fixed-point формат
	int z_size = 512<<PREC_TRACE_RAY; // +1 чтобы корректно учитывать сравнение с округленным
	if (!clamp_by_dir)
	{// Луч
		int x_size = vMap.H_SIZE<<PREC_TRACE_RAY; 
		int y_size = vMap.V_SIZE<<PREC_TRACE_RAY;
		for(;xb_>=0 && xb_<x_size && yb_>=0 && yb_<y_size && zb_<z_size;
			xb_+=dx_,yb_+=dy_,zb_+=dz_)
			// Предполагается, что камера находится над миром
			if(vMap.getZ(xb_>>PREC_TRACE_RAY,yb_>>PREC_TRACE_RAY)>=(zb_>>PREC_TRACE_RAY))
			{
				int ix=xb_>>PREC_TRACE_RAY,iy=yb_>>PREC_TRACE_RAY;
				if(pTrace) pTrace->set(float(ix),float(iy),vMap.getZf(ix,iy));
				return true;
			}
	}
	else
	{// Отрезок
		int xe_=round(pFinish.x*(1<<PREC_TRACE_RAY)),ye_=round(pFinish.y*(1<<PREC_TRACE_RAY)),ze_=round(pFinish.z*(1<<PREC_TRACE_RAY));
		int x_le = min(xb_, xe_), x_ri = max(xb_, xe_);
		int y_le = min(yb_, ye_), y_ri = max(yb_, ye_);
		for(;xb_>=x_le && xb_<=x_ri && yb_>=y_le && yb_<=y_ri && zb_<z_size;
			xb_+=dx_,yb_+=dy_,zb_+=dz_)
			// Предполагается, что камера находится над миром
			if(vMap.getZ((xb_>>PREC_TRACE_RAY),(yb_>>PREC_TRACE_RAY))>=(zb_>>PREC_TRACE_RAY))
			{
				int ix=xb_>>PREC_TRACE_RAY,iy=yb_>>PREC_TRACE_RAY;
				if(pTrace) pTrace->set(float(ix),float(iy),vMap.getZf(ix,iy));
				return true;
			}
	}

	return false;
}

void cScene::AttachObjReal(BaseGraphObject *obj)
{
	if(obj->GetKind()==KIND_LIGHT)
		UnkLightArray.Attach(obj);
	else if(obj->GetKind()==KIND_SIMPLY3DX)
		AttachSimply3dx((cSimply3dx*)obj);
	else
		grid.Attach(obj);

	obj->setScene(this);
}

void cScene::DetachObjReal(BaseGraphObject *UObj)
{
	if(UObj->GetKind()==KIND_SIMPLY3DX){
		DetachSimply3dx((cSimply3dx*)UObj);
		return;
	}

	if(UObj==pFogOfWar)
		pFogOfWar=0;

	if(UObj==tileMap_){
		RELEASE(tileMap_);
	}
	else if(UObj->GetKind()==KIND_LIGHT)
		UnkLightArray.Detach((cUnkLight*)UObj);
	else
		grid.Detach(UObj);
}

//////////////////////////////////////////////////////////////////////////////////////////
cUnkLight* cScene::CreateLight(int Attribute,cTexture *pTexture)
{
	cUnkLight* pLight=CreateLightDetached(Attribute,pTexture);
	AttachObj(pLight);
	return pLight;
}

cUnkLight* cScene::CreateLight(int Attribute, const char* TextureName)
{
	cUnkLight* pLight=CreateLightDetached(Attribute,TextureName);
	AttachObj(pLight);
	return pLight;
}

cUnkLight* cScene::CreateLightDetached(int Attribute,cTexture *pTexture)
{
	cUnkLight *Light=new cUnkLight();
	Light->setAttribute(Attribute);

	if(pTexture) 
		Light->SetTexture(0,pTexture);
	Light->setScene(this);
	return Light;
}

cUnkLight* cScene::CreateLightDetached(int Attribute, const char* TextureName)
{ // реализация cUnkLight
	xassert(!(Attribute&ATTRLIGHT_DIRECTION));//Теперь глобальный источник света задаётся при помощи функции SetSun
	cUnkLight *Light=new cUnkLight();
	Light->setAttribute(Attribute);

	if(TextureName) 
		//Light->SetTexture(0,GetTexLibrary()->GetElement(TextureName));
		Light->SetTexture(0,GetTexLibrary()->GetElement3D(TextureName));
	Light->setScene(this);
	return Light;
}
//////////////////////////////////////////////////////////////////////////////////////////
cTileMap* cScene::CreateMap(bool isTrueColor)
{
	xassert(tileMap_==0);
	MTG();

	cScene::Size=Vect2i((int)vMap.H_SIZE, (int)vMap.V_SIZE);
	tileMap_=new cTileMap(this, isTrueColor);

	TileNumber=tileMap_->tileNumber();
	tile_size=tileMap_->tileSize().x;
	return tileMap_;
}
 
FogOfWar* cScene::CreateFogOfWar()
{
	xassert(pFogOfWar==0);
	pFogOfWar=new FogOfWar();
	AttachObj(pFogOfWar);
	return pFogOfWar;
}
//////////////////////////////////////////////////////////////////////////////////////////
cObject3dx* cScene::CreateObject3dx(const char* fname,const char *TexturePath,bool interpolated)
{
	cObject3dx *pObject=CreateObject3dxDetached(fname,TexturePath,interpolated);
	if(pObject)
		AttachObj(pObject);
	return pObject;
}

//#include "J:\Program Files\Intel\VTune\Analyzer\Include\VtuneApi.h"
//#pragma comment(lib,"J:\\Program Files\\Intel\\VTune\\Analyzer\\Lib\\VtuneApi.lib")

cObject3dx* cScene::CreateObject3dxDetached(const char* fname,const char *TexturePath,bool interpolated)
{
	start_timer_auto();

//	if(gb_VisGeneric->GetLogicQuant()>50)
//		VTResume();

	cStatic3dx *pStatic=pLibrary3dx->GetElement(fname,TexturePath,false);
	if(pStatic==0) return 0;
	cObject3dx *pObject=new cObject3dx(pStatic,interpolated);
	pObject->setScene(this);
	pObject->putAttribute(ATTR3DX_HIDE_LIGHTS,hide_lights);
	pObject->EnableSelfIllumination(!hide_selfillumination);

//	if(gb_VisGeneric->GetLogicQuant()>50)
//		VTPause();

	return pObject;
}

cObject3dx* cScene::CreateLogic3dx(const char* fname, bool interpolated)
{
	start_timer_auto();

	cStatic3dx *pStatic=pLibrary3dx->GetElement(fname,0,true);
	if(pStatic==0) 
		return 0;
	cObject3dx *pObject=new cObject3dx(pStatic,interpolated);
	return pObject;
}

cPlane* cScene::CreatePlaneObj()
{
	cPlane *PlaneObj=new cPlane;
	AttachObj(PlaneObj);
	return PlaneObj;
}

cEffect* cScene::CreateEffectDetached(EffectKey& el,c3dx* models,bool auto_delete_after_life)
{
//	mem_check();
	if (el.GetNeedTilemap() && !GetTileMap()) 
		return 0;
	cEffect* UObj=new cEffect();
	UObj->SetAutoDeleteAfterLife(auto_delete_after_life);
	UObj->setScene(this);
	UObj->Init(el,models);
//	AttachObj(UObj);
//	mem_check();
	return UObj;
}

//////////////////////////////////////////////////////////////////////////////////////////

Camera* cScene::CreateCamera()
{
	return new Camera(this);
}

void cScene::SetShadowIntensity(const Color4f& f)
{
	shadow_intensity=f;
}

void cScene::CreateShadowmap()
{
	gb_RenderDevice3D->SetAdvance(true);
	gb_RenderDevice3D->deleteRenderTargets();

	int width = 256<<(Option_ShadowSizePower-1);

	shadowEnabled_ = Option_shadowEnabled;

	if(!gb_RenderDevice3D->createRenderTargets(width)){
		gb_VisGeneric->SetShadowType(false, 0);
		gb_RenderDevice3D->createRenderTargets(width);
	}

	float SizeLightMap=vMap.H_SIZE;
	float focus=1/SizeLightMap;

	GetTexLibrary()->Compact();
}

Vect2f cScene::CalcZMinZMaxShadowReciver()
{
	Vect2f objectz=shadowCamera_->CalcZMinZMaxShadowReciver();
/*	//Падение уже исправили. Падало из-за того что вызывалось когда реально не было теней.
	//Но так как это слишком мелкий баг правит, то закомментарили на всяк случай. 
	for(vector<ListSimply3dx>::iterator it=simply_objects.begin();it!=simply_objects.end();it++)
	{
		it->pStatic->AddZMinZMaxShadowReciver(shadowCamera_->GetMatrix(),objectz);
	}
*/
	return objectz;
}

void cScene::FixShadowMapCamera(Camera* camera, Camera* shadowCamera)
{
	start_timer_auto();
	sBox6f box = fixShadowBox;
	MatXf LightMatrix = fixShaddowLightMatrix;

	Vect2f boxz=tileMap_->CalcZ(shadowCamera);
	Vect2f objectz=CalcZMinZMaxShadowReciver();
	boxz.x=min(boxz.x,objectz.x);
	boxz.y=max(boxz.y,objectz.y);

	box.min.z+=boxz.x;//-camera->GetZPlane().x;
	box.max.z=box.min.z+(boxz.y-boxz.x);

	box = fixShadowBox; // !!!

	Vect3f PosLight((box.min.x+box.max.x)*0.5f,(box.min.y+box.max.y)*0.5f,box.min.z);
	LightMatrix.trans()=-PosLight;

	Vect2f Focus(1/(box.max.x-box.min.x),1/(box.max.y-box.min.y));
	shadowCamera->SetFrustum(&Vect2f(0.5f,0.5f),&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),
		&Focus, &Vect2f(0,box.max.z-box.min.z));
	shadowCamera->SetPosition(LightMatrix);
}

#define DET2(a, b, c, d) ((a) * (d) - (b) * (c))

void intersect(Vect2f& i, const Vect2f& g0, const Vect2f& g1, const Vect2f& h0, const Vect2f& h1) 
{
	float a, b;

	i[0] = i[1] = 1.0f / DET2(g0[0] - g1[0], g0[1] - g1[1], h0[0] - h1[0], h0[1] - h1[1]);

	a = DET2(g0[0], g0[1], g1[0], g1[1]);
	b = DET2(h0[0], h0[1], h1[0], h1[1]);

	i[0] *=	DET2(a, g0[0] - g1[0], b, h0[0] - h1[0]);
	i[1] *=	DET2(a, g0[1] - g1[1], b, h0[1] - h1[1]);
}

void map_Trapezoid_To_Square(Mat4f& N_T, const Vect2f& t0, const Vect2f& t1, const Vect2f& t2, const Vect2f& t3) 
{
	float a, b, c, d;

	//M1 = R * T1
	a = 0.5f * (t2[0] - t3[0]);
	b = 0.5f * (t2[1] - t3[1]);

	Mat3f TR(a,  b, a * a + b * b,
	         b, -a, a * b - b * a,
	         0.0f, 0.0f, 1.0f);

	//M2 = T2 * M1 = T2 * R * T1
	Vect2f i;
	intersect(i, t0, t3, t1, t2);

	TR[0][2] = -dot(TR[0], i);
	TR[1][2] = -dot(TR[1], i);

	//M1 = H * M2 = H * T2 * R * T1
	a = dot(TR[0], t2) + TR[0][2];
	b = dot(TR[1], t2) + TR[1][2];
	c = dot(TR[0], t3) + TR[0][2];
	d = dot(TR[1], t3) + TR[1][2];

	a = -(a + c) / (b + d);

	TR[0][0] += TR[1][0] * a;
	TR[0][1] += TR[1][1] * a;
	TR[0][2] += TR[1][2] * a;

	//M2 = S1 * M1 = S1 * H * T2 * R * T1
	a = 1.0f / (dot(TR[0], t2) + TR[0][2]);
	b = 1.0f / (dot(TR[1], t2) + TR[1][2]);

	TR[0][0] *= a; TR[0][1] *= a; TR[0][2] *= a;
	TR[1][0] *= b; TR[1][1] *= b; TR[1][2] *= b;

	//M1 = N * M2 = N * S1 * H * T2 * R * T1
	TR[2][0] = TR[1][0]; TR[2][1] = TR[1][1]; TR[2][2] = TR[1][2];
	TR[1][2] += 1.0f;

	//M2 = T3 * M1 = T3 * N * S1 * H * T2 * R * T1
	a = dot(TR[1], t0) + TR[1][2];
	b = dot(TR[2], t0) + TR[2][2];
	c = dot(TR[1], t2) + TR[1][2];
	d = dot(TR[2], t2) + TR[2][2];

	a = -0.5f * (a / b + c / d);

	TR[1][0] += TR[2][0] * a;
	TR[1][1] += TR[2][1] * a;
	TR[1][2] += TR[2][2] * a;

	//M1 = S2 * M2 = S2 * T3 * N * S1 * H * T2 * R * T1
	a = dot(TR[1], t0) + TR[1][2];
	b = dot(TR[2], t0) + TR[2][2];

	TR[1] *= -b/a; 

	N_T[0][0] = TR[0][0]; N_T[1][0] = TR[0][1]; N_T[2][0] =  0.0f; N_T[3][0] = TR[0][2];
	N_T[0][1] = TR[1][0]; N_T[1][1] = TR[1][1]; N_T[2][1] =  0.0f; N_T[3][1] = TR[1][2];
	N_T[0][2] =     0.0f; N_T[1][2] =     0.0f; N_T[2][2] =  1.0f; N_T[3][2] =     0.0f;
	N_T[0][3] = TR[2][0]; N_T[1][3] = TR[2][1]; N_T[2][3] =  0.0f; N_T[3][3] = TR[2][2];
}

void line(const Vect2f& a, const Vect2f& b, const Color4c& c)
{
	gb_RenderDevice->DrawLine(500 + a.x*20, 500 + a.y*20, 500 + b.x*20, 500 + b.y*20, c);
}

float buildTrapezoid(const Vect2f& origin, const Vect2f& yAxis, const Vect2f f[8], Vect2f t[4], bool show)
{
	if(show){
		line(f[0], f[1], Color4c::WHITE);
		line(f[1], f[3], Color4c::WHITE);
		line(f[2], f[3], Color4c::WHITE);
		line(f[0], f[2], Color4c::WHITE);

		line(f[4], f[5], Color4c::WHITE);
		line(f[5], f[7], Color4c::WHITE);
		line(f[6], f[7], Color4c::WHITE);
		line(f[4], f[6], Color4c::WHITE);
	}

	float yMin = FLT_INF, yMax = -FLT_INF;
	for(int i = 0; i < 8; i++){
		float y = yAxis.dot(f[i] - origin);
		yMin = min(yMin, y);
		yMax = max(yMax, y);
	}

	if(yMax > 0.2f) // Сильное отодвигание зоны фокуса (near-plane вписана в far-plane или неудачная конфигурация)
		return FLT_INF;

	Vect2f pf = origin + yAxis*yMin;
	Vect2f pn = origin + yAxis*yMax;

	float E = -0.6f;
	float L = yMax - yMin;
	float d = Option_trapezoidFactor*L;
	float n = (L*d*(1.f + E))/(L - 2*d - L*E);
	Vect2f q = origin + yAxis*(yMax + n);

	Vect2f xAxis(-yAxis.y, yAxis.x);

	float tangentMax = 0;
	for(int i = 0; i < 8; i++){
		float x = xAxis.dot(f[i] - q);
		float y = yAxis.dot(f[i] - q);
		if(fabsf(y) > FLT_EPS)
			tangentMax = max(tangentMax, fabsf(x/y));
	}

	float y1 = n;
	float x1 = tangentMax*y1;
	float y_1 = L + n;
	float x_1 = tangentMax*y_1;

	t[0] = q + xAxis*x_1 - yAxis*y_1;
	t[1] = q - xAxis*x_1 - yAxis*y_1;
	t[2] = q - xAxis*x1 - yAxis*y1;
	t[3] = q + xAxis*x1 - yAxis*y1;

	if(show){
		line(pn, pf, Color4c::YELLOW);
		line(pn, q, Color4c::GREEN);
		line(t[0], t[1], Color4c::RED);
		line(t[1], t[2], Color4c::RED);
		line(t[2], t[3], Color4c::RED);
		line(t[3], t[0], Color4c::RED);
	}

	return (t[0].distance(t[1]) + t[2].distance(t[3]))*pn.distance(pf);
}

void cScene::fixShadowMapCameraTSM(Camera* camera, Camera* shadowCamera)
{
	start_timer_auto();

	Vect2f boxz = tileMap_->CalcZ(camera);
	Vect2f objectz = camera->CalcZMinZMaxShadowReciver();
	boxz.x = min(boxz.x,objectz.x);
	boxz.y = max(boxz.y,objectz.y);
	boxz.y = max(boxz.y,1600.f);

	Vect2f zPlaneOld;
	camera->GetFrustum(0, 0, 0, &zPlaneOld);
	camera->SetFrustum(0, 0, 0, &boxz); 

	Vect3f frustrumPoints[8];
	camera->GetFrustumPoint(
		frustrumPoints[0],
		frustrumPoints[1],
		frustrumPoints[2],
		frustrumPoints[3],
		frustrumPoints[4],
		frustrumPoints[5],
		frustrumPoints[6],
		frustrumPoints[7]
		);
	camera->SetFrustum(0, 0, 0, &zPlaneOld);

	Vect2f f[8];
	sBox6f frustrumBox;
	for(int i = 0; i < 8; i++){
		shadowCamera->matViewProj.xformCoord(frustrumPoints[i]);
		f[i] = frustrumPoints[i];
		frustrumBox.addPoint(frustrumPoints[i]);
	}

	start_timer(1);
	Vect2f pn = (f[0] + f[1] + f[2] + f[3])/4;
	Vect2f pf = (f[4] + f[5] + f[6] + f[7])/4;
	Vect2f yAxis = pn - pf;
	if(yAxis.norm2() < 0.1f){ // Отсекает совсем вырожденный случай, который падает
		FixShadowMapCamera(camera, shadowCamera);
		return;
	}
	Vect2f origin = pn;
	yAxis.normalize(1);

	Vect2f t[4];
	float angle = 0;
	float delta = M_PI/32;
	float area, areaMin = buildTrapezoid(origin, yAxis, f, t, false);
	for(float a = -M_PI/4; a < M_PI/4; a += delta)
		if(areaMin > (area = buildTrapezoid(origin, Mat2f(a)*yAxis, f, t, false))){
			angle = a;
			areaMin = area;
		}

	while(delta > M_PI/100){
		if(areaMin > (area = buildTrapezoid(origin, Mat2f(angle + delta)*yAxis, f, t, false))){
			angle += delta;
			areaMin = area;
		}
		else if(areaMin > (area = buildTrapezoid(origin, Mat2f(angle - delta)*yAxis, f, t, false))){
			angle -= delta;
			areaMin = area;
		}
		delta /= 2;
	}

	float trapezoidArea = buildTrapezoid(origin, Mat2f(angle)*yAxis, f, t, Option_showDebugTSM);
	float boxArea = (frustrumBox.max.x - frustrumBox.min.x)*(frustrumBox.max.y - frustrumBox.min.y);
	float k = boxArea/trapezoidArea;

	stop_timer(1);

	if(k < 0.2f){ // Трапеция слишком плохо описывает frustrum (по площади)
		FixShadowMapCamera(camera, shadowCamera);
		return;
	}

		 
	Mat4f T;
	map_Trapezoid_To_Square(T, t[0], t[1], t[2], t[3]);

	shadowCamera->matViewProj = shadowCamera->matViewProj*T;
}

void cScene::CalcShadowMapCamera(Camera* camera, Camera *shadowCamera)
{
	if(!shadowCamera->GetParent())
		return;
	Vect3f LightDirection(0,0,-1);
	GetLightingShadow(LightDirection);
	MatXf LightMatrix;
	LightMatrix.rot().zrow() = LightDirection;
	LightMatrix.rot().xrow().cross(Vect3f::I, LightDirection);
	LightMatrix.rot().xrow().normalize();
	LightMatrix.rot().yrow().cross(LightDirection, LightMatrix.rot().xrow());
	LightMatrix.trans() = Vect3f::ZERO;

	//const float bound_z=300.0f;
	const float bound_z=1000.0f;

	sBox6f box;
	ClippingMesh(tileMap_->zMax()).calcVisBox(camera,tileMap_->tileNumber(),tileMap_->tileSize(),Mat4f(LightMatrix), box);

	box.min.z-=bound_z;
	box.max.z+=bound_z;

	Vect3f PosLight((box.min.x+box.max.x)*0.5f,(box.min.y+box.max.y)*0.5f,box.min.z);
	LightMatrix.trans()=-PosLight;

	Vect2f Focus(1/(box.max.x-box.min.x),1/(box.max.y-box.min.y));
	shadowCamera->SetFrustum(&Vect2f(0.5f,0.5f),&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),
		&Focus, &Vect2f(0,box.max.z-box.min.z));
	shadowCamera->SetPosition(LightMatrix);

//С одной стороны эта камера должна быть посчитана до того момента
//когда начнёт определяться, какие объекты видимы. С другой стороны она должна быть
//посчитанна позже, так как вызывается CalculateZMinMax
	fixShadowBox = box;
	fixShaddowLightMatrix = LightMatrix;
}

void cScene::AddPlanarCamera(Camera* camera, bool light, bool toObjects)
{
	sBox6f box;
	ClippingMesh(tileMap_->zMax()).calcVisBox(camera,tileMap_->tileNumber(),tileMap_->tileSize(),Mat4f::ID,box);

	int mask = 255;
	box.min.x = round(box.min.x) & ~mask;
	box.min.y = round(box.min.y) & ~mask;
	box.max.x = round(box.max.x) | mask;
	box.max.y = round(box.max.y) | mask;

	Vect4f planarTransform(box.min.x, box.min.y, 1/(box.max.x-box.min.x), 1/(box.max.y-box.min.y));
	if(light && !toObjects)
		gb_RenderDevice3D->setPlanarTransform(planarTransform);

	Camera* planarCamera = light ? (toObjects ? lightObjectsCamera_ : lightCamera_) : shadowCamera_;
	Vect3f PosLight;
	PosLight.x = (box.max.x + box.min.x)*0.5f;
	PosLight.y = (box.max.y + box.min.y)*0.5f;
	PosLight.z = 5000;

	Vect3f vShadow(0,0,-1);
	MatXf LightMatrix;
	LightMatrix.rot().xrow().cross(vShadow,Vect3f(0,1,0));
	LightMatrix.rot().yrow()=Vect3f(0,-1,0);
	LightMatrix.rot().zrow()=vShadow;
	LightMatrix.trans()=LightMatrix.rot().xform( -PosLight );

	camera->SetCopy(planarCamera);
	camera->AttachChild(planarCamera);

	Vect2f Focus(planarTransform.z, planarTransform.w);
	planarCamera->setAttribute(ATTRCAMERA_SHADOW|ATTRUNKOBJ_NOLIGHT);
	planarCamera->clearAttribute(ATTRCAMERA_PERSPECTIVE | ATTRCAMERA_SHOWCLIP | ATTRCAMERA_WRITE_ALPHA);
	planarCamera->SetRenderTarget(light ? (toObjects? gb_RenderDevice3D->GetLightMapObjects() : gb_RenderDevice3D->GetLightMap()) : gb_RenderDevice3D->GetShadowMap(), 0); 
	planarCamera->SetFrustum(&Vect2f(0.5f,0.5f), &sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),
						   &Focus, &Vect2f(10,1e6f));
	
	planarCamera->SetPosition(LightMatrix);
	planarCamera->Attach(SCENENODE_OBJECT,tileMap_); // рисовать источники света							   
}

void cScene::AddShadowCamera(Camera* camera)
{
	gb_RenderDevice3D->SetAdvance(true);
	if((Option_shadowEnabled && gb_RenderDevice3D->GetShadowMap()==0) || (!Option_shadowEnabled && gb_RenderDevice3D->GetLightMap() == 0)){
		CreateShadowmap();
	}
	else{
		bool change_shadowmap=false;
		if(gb_RenderDevice3D->GetShadowMap() &&
		  (256 << (Option_ShadowSizePower - 1) != gb_RenderDevice3D->GetShadowMap()->GetWidth() || 
		   shadowEnabled_ != Option_shadowEnabled))
			change_shadowmap = true;

		if(!Option_ShadowSizePower && shadowEnabled_ != Option_shadowEnabled)
			change_shadowmap = true;

		if(change_shadowmap)
			CreateShadowmap();
	}

	if(!IsIntensityShadow() || !Option_shadowEnabled) {
		AddPlanarCamera(camera, true, false);
		AddPlanarCamera(camera, true, true);
	} 
	else if(gb_RenderDevice3D->GetShadowMap()) { // shadow
		if(Option_shadowEnabled) {
			AddLightCamera(camera);
			if(Option_shadowEnabled){
				AddPlanarCamera(camera, true, false);
				AddPlanarCamera(camera, true, true);
			}
		} 
		else 
			AddPlanarCamera(camera, false, false);
	}
}

void cScene::AddLightCamera(Camera* camera)
{
	camera->setAttribute(ATTRCAMERA_ZMINMAX);
	camera->SetCopy(shadowCamera_);
	camera->AttachChild(shadowCamera_);
	shadowCamera_->setAttribute(ATTRCAMERA_SHADOWMAP|ATTRUNKOBJ_NOLIGHT);
	shadowCamera_->clearAttribute(ATTRCAMERA_PERSPECTIVE|ATTRCAMERA_ZMINMAX|ATTRCAMERA_SHOWCLIP);
	shadowCamera_->SetRenderTarget(gb_RenderDevice3D->GetShadowMap(), gb_RenderDevice3D->GetZBufferShadowMap());

//	Vect2f z=CalcZ(camera);

//	if(Option_ShadowType!=SHADOW_MAP_PERSPECTIVE)
//		z.x=30.0f;

//	Vect2f zplane=camera->GetZPlane();
//	camera->SetZPlaneTemp(z);

	CalcShadowMapCamera(camera, shadowCamera_);
	
	shadowCamera_->Attach(SCENENODE_OBJECT, tileMap_);

//	camera->SetZPlaneTemp(zplane);
}

void cScene::AddMirageCamera(Camera* camera)
{
	if(!gb_RenderDevice3D->GetMirageMap()){
		D3DSURFACE_DESC desc;
		gb_RenderDevice3D->backBuffer_->GetDesc(&desc);
		gb_RenderDevice3D->CreateMirageMap(desc.Width,desc.Height);
	}
	camera->SetCopy(mirageCamera_);
	//camera->AttachChild(mirageCamera_);
	mirageCamera_->setAttribute(ATTRCAMERA_MIRAGE);
	mirageCamera_->SetRenderTarget(gb_RenderDevice3D->GetMirageMap(), gb_RenderDevice3D->zBuffer_);
	mirageCamera_->SetFoneColor(Color4c(128,128,128,0));
	mirageCamera_->setAttribute(ATTRCAMERA_NOCLEARTARGET);
	mirageCamera_->Update();
}


void cScene::setZReflection(float zreflection, float weight)
{
	average(zreflection_, zreflection, weight);
}

void cScene::AddReflectionCamera(Camera* camera)
{
	if(reflectionCamera_)
	{ 
		DWORD clear=reflectionCamera_->getAttribute(ATTRCAMERA_NOCLEARTARGET);
		camera->SetCopy(reflectionCamera_);
		reflectionCamera_->setAttribute(clear);

		camera->AttachChild(reflectionCamera_);
		if(pReflectionRenderTarget)
			reflectionCamera_->SetRenderTarget(pReflectionRenderTarget,pReflectionZBuffer);
		reflectionCamera_->setAttribute(ATTRCAMERA_REFLECTION|ATTRCAMERA_CLEARZBUFFER);
		reflectionCamera_->setAttribute(ATTRCAMERA_WRITE_ALPHA);
		reflectionCamera_->SetFoneColor(Color4c(0,0,0,0));

		MatXf matrix = camera->GetMatrix();
		matrix.trans() += matrix.rot().zcol()*Option_reflectionOffset;
		MatXf reflection_matrix;
		Plane ReflectionPlane(0,0,-1, zreflection_);
		ReflectionPlane.reflectionMatrix(matrix, reflection_matrix);
		reflectionCamera_->SetPosition(reflection_matrix);
	}

}
void cScene::AddFloatZBufferCamera(Camera* camera)
{
	if(!gb_RenderDevice3D->GetFloatMap()){
		D3DSURFACE_DESC desc;
		gb_RenderDevice3D->backBuffer_->GetDesc(&desc);
		if (!gb_RenderDevice3D->CreateFloatTexture(desc.Width,desc.Height))
			return;
		//gb_RenderDevice3D->CreateFloatTexture(512,512);
	}
	camera->SetCopy(floatZBufferCamera_);
	floatZBufferCamera_->SetFrustum(&Vect2f(0.5f,0.5f), &sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),
		0, 0);
	camera->AttachChild(floatZBufferCamera_);
	floatZBufferCamera_->SetRenderTarget(gb_RenderDevice3D->GetFloatMap(), gb_RenderDevice3D->GetFloatZBuffer());
	floatZBufferCamera_->setAttribute(ATTRCAMERA_FLOAT_ZBUFFER);
	floatZBufferCamera_->Attach(SCENENODE_OBJECT, tileMap_);
	floatZBufferCamera_->Update();

}

void cScene::deleteManagedResource()
{
	DeleteReflectionSurface();
}

void cScene::restoreManagedResource()
{
	CreateReflectionSurface();
}

void cScene::CreateReflectionSurface()
{
	if(reflectionCamera_)
		DeleteReflectionSurface();

	if(!enable_reflection)
		return;

	int xysize=1024;	
	HRESULT hr=gb_RenderDevice3D->D3DDevice_->CreateDepthStencilSurface(xysize, xysize, 
		D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, TRUE, &pReflectionZBuffer, 0);
	if(FAILED(hr))
	{	
		xassert(0);
		RELEASE(pReflectionZBuffer);
		return;
	}

	pReflectionRenderTarget=GetTexLibrary()->CreateRenderTexture(xysize,xysize,TEXTURE_RENDER32,false);

	reflectionCamera_=CreateCamera();
	reflectionCamera_->SetRenderTarget(pReflectionRenderTarget,pReflectionZBuffer);
	reflectionCamera_->Update();
}

void cScene::DeleteReflectionSurface()
{
	RELEASE(pReflectionRenderTarget);
	RELEASE(reflectionCamera_);
	RELEASE(pReflectionZBuffer);
}

void cScene::EnableReflection(bool enable)
{
	if(!gb_RenderDevice3D->IsPS20())
		enable = false;

	enable_reflection = enable;
	if(!enable){
		DeleteReflectionSurface();
		return;
	}

	if(reflectionCamera_)
		return;

	CreateReflectionSurface();
}

void cScene::DeleteAutoObject()
{
	MTG();
	UpdateLists(INT_MAX);

	for(int i=0;i<grid.size();i++){
		BaseGraphObject* obj=grid[i];
		if(obj){
			cEffect* eff = dynamic_cast<cEffect*>(obj);
			if(eff && eff->IsAutoDeleteAfterLife() && eff->Release()<=0)
				i--;
		}
	}
	UpdateLists(INT_MAX);
}

void cScene::SetSunShadowDir(const Vect3f& direction)
{
	separately_sun_dir_shadow = true;
	sun_dir_shadow = direction;
}

void cScene::SetSunDirection(const Vect3f& direction)
{
	sun_direction = direction;
	sun_direction.normalize();
}

void cScene::SetSunColor(const Color4f& ambient, const Color4f& diffuse, const Color4f& specular)
{
	sun_ambient=ambient;
	sun_diffuse=diffuse;
	sun_specular=specular;
}

void cScene::GetLighting(Vect3f *LightDirection)
{
	*LightDirection=sun_direction;
}

void cScene::GetLightingShadow(Vect3f& LightDirection)
{
	if(separately_sun_dir_shadow)
		LightDirection = sun_dir_shadow;
	else
		LightDirection = sun_direction;
}

void cScene::GetLighting(Color4f &Ambient,Color4f &Diffuse,Color4f &Specular,Vect3f &LightDirection)
{
	Ambient=sun_ambient;
	Diffuse=sun_diffuse;
	Specular=sun_specular;
	GetLighting(&LightDirection);
}

void cScene::SetSkyCubemap(cTexture* pTexture)
{
	RELEASE(pSkyCubemap);
	if(pTexture)
	{
		pTexture->AddRef();
		pSkyCubemap=pTexture;
	}
}

void cScene::EnableFogOfWar(bool enable)
{
	enable_fog_of_war=enable;
}

void cScene::AddCircleShadow(const Vect3f& pos,float r,Color4c intensity)
{
	CircleShadow cs;
	cs.pos=pos;
	cs.radius=r;
	cs.color=intensity;
	circle_shadow.push_back(cs);

}

void cScene::HideAllObjectLights(bool hide)
{
//	MTAuto lock(gb_VisGeneric->GetLockResetDevice());
	MTG();
	hide_lights=hide;
	sGrid2d::iterator it;
	FOR_EACH(grid,it)
	{
		int kind=(*it)->GetKind();
		if(kind==KIND_OBJ_3DX)
			(*it)->putAttribute(ATTR3DX_HIDE_LIGHTS,hide);
	}
}

void cScene::HideSelfIllumination(bool hide)
{
//	MTAuto lock(gb_VisGeneric->GetLockResetDevice());
	MTG();
	hide_selfillumination=hide;
	sGrid2d::iterator it;
	FOR_EACH(grid,it)
	{
		int kind=(*it)->GetKind();
		if(kind==KIND_OBJ_3DX)
		{
			cObject3dx* p=(cObject3dx*)(*it);
			p->EnableSelfIllumination(!hide);
		}
	}
}

vector<ListSimply3dx>& cScene::GetAllSimply3dxList()
{
	return simply_objects;
}

void cScene::GetAllObject3dx(vector<cObject3dx*>& objectsList)
{
	sGrid2d::iterator it;
	FOR_EACH(grid,it)
	{
		int kind=(*it)->GetKind();
		if(kind==KIND_OBJ_3DX)
			objectsList.push_back((cObject3dx*)*it);
	}
}

void cScene::GetAllEffects(vector<cEffect*>& effectsList)
{
	sGrid2d::iterator it;
	FOR_EACH(grid,it)
	{
		int kind=(*it)->GetKind();
		if(kind==KIND_EFFECT)
			effectsList.push_back((cEffect*)*it);
	}
}

bool cScene::DebugInUpdateList()
{
	return in_update_list;
}

cSimply3dx* cScene::CreateSimply3dx(const char* fname,const char* visible_group,const char *TexturePath)
{
	start_timer_auto();

	cStaticSimply3dx* pStatic=pLibrarySimply3dx->GetElement(fname,visible_group,TexturePath);
	if(pStatic==0)
		return 0;
	cSimply3dx* pSimply=new cSimply3dx(pStatic);
	AttachObj(pSimply);
	return pSimply;
}

cSimply3dx* cScene::CreateSimply3dxDetached(const char* fname,const char* visible_group,const char *TexturePath)
{
	start_timer_auto();

	cStaticSimply3dx* pStatic=pLibrarySimply3dx->GetElement(fname,visible_group,TexturePath);
	if(pStatic==0)
		return 0;
	cSimply3dx* pSimply=new cSimply3dx(pStatic);
	pSimply->setScene(this);
	return pSimply;
}

void cScene::AttachSimply3dx(cSimply3dx* pSimply)
{
	pSimply->setScene(this);

	ListSimply3dx* lib=0;
	for(int i=0;i<simply_objects.size();i++)
	{
		ListSimply3dx& cur=simply_objects[i];
		if(cur.pStatic==pSimply->GetStatic())
		{
			lib=&cur;
			break;
		}
	}

	if(lib==0)
	{
		simply_objects.push_back(ListSimply3dx());
		simply_objects.back().pStatic=pSimply->GetStatic();
		lib=&simply_objects.back();
	}

	lib->objects.push_back(pSimply);
}

void cScene::DetachSimply3dx(cSimply3dx* pObj)
{
	for(int ilib=0;ilib<simply_objects.size();ilib++)
	{
		ListSimply3dx& cur=simply_objects[ilib];
		if(cur.pStatic==pObj->GetStatic())
		{
			vector<cSimply3dx*>::iterator it=find(cur.objects.begin(),cur.objects.end(),pObj);
			if(it!=cur.objects.end())
			{
				cur.objects.erase(it);
				RELEASE(pObj);
			}else
			{
				xassert(0);
			}

			return;
		}
	}
	
	xassert(0);
}

void cScene::RemoveEmptyStaticSimply3dx()
{//При удалении StaticSimply3dx в массиве simply_objects получается потерянная ссылка.
	for(int i=0;i<simply_objects.size();)
	{
		ListSimply3dx& cur=simply_objects[i];
		if(cur.objects.empty())
		{
			simply_objects.erase(simply_objects.begin()+i);
			continue;
		}

		i++;
	}

}

void cScene::AttachObj(BaseGraphObject *UnkObj)
{
	MTAuto mtauto(critial_attach);
	xassert(!UnkObj->getAttribute(ATTRUNKOBJ_ATTACHED));
	UnkObj->setAttribute(ATTRUNKOBJ_ATTACHED);
#ifdef _DEBUG
	vector<AddData>::iterator it;
	BaseGraphObject* pFind=0;
	FOR_EACH(add_list,it)
	if(it->object==UnkObj)
		pFind=it->object;
	xassert(pFind==0);
#endif
	UnkObj->setScene(this);
	AddData data;
	data.object=UnkObj;
	if(gb_VisGeneric->GetUseLogicQuant())
	{
		if(MT_IS_GRAPH())
			data.quant=gb_VisGeneric->GetGraphLogicQuant();
		else
			data.quant=gb_VisGeneric->GetLogicQuant()+1;//Добавляем на следующий квант.
	}else
	{
		data.quant=0;
	}

	add_list.push_back(data);
}

void cScene::DetachObj(BaseGraphObject *UnkObj)
{
	UnkObj->AddRef();

	MTAuto mtauto(critial_attach);
	int quant=0;
	if(gb_VisGeneric->GetUseLogicQuant())
	{
		quant=gb_VisGeneric->GetLogicQuant()+3;//Удаляем через квант.
	}

	if(erase_list.empty() || erase_list.back().quant!=quant)
	{
		sErase e;
		e.quant=quant;
		erase_list.push_back(e);
	}

	erase_list.back().erase_list.push_back(UnkObj);
}

void cScene::mtUpdate(int cur_quant)
{
	bool attach_on_next_logic_quant=false;
	MTAuto mtauto(critial_attach);

	list<sErase>::iterator it_erase;

	FOR_EACH(erase_list,it_erase)
	{
		sErase& e=*it_erase;

		if(e.quant<=cur_quant)
		{
			list<BaseGraphObject*>::iterator itl;
			for(itl=e.erase_list.begin();itl!=e.erase_list.end();)
			{
				vector<AddData>::iterator it_delete;
				BaseGraphObject* pFind=0;
				FOR_EACH(add_list,it_delete)
				if(it_delete->object==*itl)
				{
					pFind=it_delete->object;
					break;
				}

				if(pFind)
				{
					BaseGraphObject* obj=*itl;
					if(false)
					{
						obj->Release();//Удаляем объект еще не добавленный в список.
					}else
					{
						xassert(obj->GetRef()==1);
						xassert(obj->getAttribute(ATTRUNKOBJ_DELETED));
						obj->DecRef();
						delete obj;
					}
					
					add_list.erase(it_delete);
					itl=e.erase_list.erase(itl);
				}else
				{
					itl++;
				}
			}
		}
	}

	if(attach_on_next_logic_quant)
	{
		vector<AddData>::iterator itl;
		for(itl=add_list.begin();itl<add_list.end();)
		{
			if(itl->quant<=cur_quant)
			{
				AttachObjReal(itl->object);
				itl=add_list.erase(itl);
			}else
			{
				itl++;
			}
		}
	}else
	{
		vector<AddData>::iterator itl;
		FOR_EACH(add_list,itl)
		{
			AttachObjReal(itl->object);
		}
		add_list.clear();
	}

	for(it_erase=erase_list.begin();it_erase!=erase_list.end();)
	{
		sErase& e=*it_erase;
		if(e.quant<=cur_quant)
		{
			list<BaseGraphObject*>::iterator itl;
			FOR_EACH(e.erase_list,itl)
			{
				DetachObjReal(*itl);
			}

			it_erase=erase_list.erase(it_erase);
		}else
		{
			it_erase++;
		}
	}

	int erase_lists=0;
	int erase_object=0;
	FOR_EACH(erase_list,it_erase)
	{
		sErase& e=*it_erase;
		erase_lists++;
		erase_object+=e.erase_list.size();
	}

	if(cur_quant==INT_MAX)
		xassert(erase_list.empty());
}

void cScene::BuildTree()
{
	if(!tileMap_)
	{
		//Если нет карты - использовать сферические источники света.
		tree.clear();

		bool is_spherical=false;
		for(int i=0;i<GetNumberLight();i++)
		{
			cUnkLight* ULight=GetLight(i);
			if(ULight && ULight->getAttribute(ATTRLIGHT_SPHERICAL_OBJECT|ATTRLIGHT_SPHERICAL_TERRAIN) && !ULight->getAttribute(ATTRLIGHT_IGNORE))
			{
				is_spherical=true;
			}
		}

		if(is_spherical)
		{
			vector<cObject3dx*> nodes;
			sGrid2d::iterator it;
			FOR_EACH(grid,it)
			{
				int kind=(*it)->GetKind();
				if(kind==KIND_OBJ_3DX)
					nodes.push_back((cObject3dx*)*it);
			}

			tree.build(nodes);
		}
	}
}

struct SceneLightProcParam
{
	cUnkLight* light;
	float radius;
	float inv_radius;
	Vect3f pos;
	Color4f color;
};

static void SceneLightProc(cObject3dx* obj,void* param)
{
	SceneLightProcParam& p=*(SceneLightProcParam*)param;
	int kind=obj->GetKind();
	xassert(kind==KIND_OBJ_3DX);
	cObject3dx* node=obj;
	node->AddLight(p.light);
}

void cScene::CaclulateLightAmbient()
{
	if(tileMap_)
		return;

	for(int i=0;i<GetNumberLight();i++)
	{
		cUnkLight* ULight=GetLight(i);
		if(ULight && ULight->getAttribute(ATTRLIGHT_SPHERICAL_OBJECT|ATTRLIGHT_SPHERICAL_TERRAIN) && !ULight->getAttribute(ATTRLIGHT_IGNORE))
		{
			SceneLightProcParam p;
			p.light=ULight;
			p.pos=ULight->GetPos();
			p.radius=ULight->GetRadius();
			if(p.radius<5)
				continue;
			p.inv_radius=1/p.radius;
			p.color=ULight->GetDiffuse();
			p.color*=2;

			tree.find(Vect2i(p.pos.x,p.pos.y), float(p.radius), SceneLightProc, &p);
		}
	}
}

Color4f cScene::GetPlainLitColor()
{
	Color4f color(GetTileMap()->GetDiffuse());
	float diffuse_n=-GetSunDirection().z;
	color.r=color.r*diffuse_n+color.a;
	color.g=color.g*diffuse_n+color.a;
	color.b=color.b*diffuse_n+color.a;
	return color;
}

bool cScene::CreateDebrisesDetached(const c3dx* model,vector<cSimply3dx*>& debrises)
{
	cStatic3dx* pStatic=pLibrary3dx->GetElement(model->GetFileName(),0,false);
	if(pStatic==0)
		return false;
	debrises.resize(pStatic->debrises.size());
	for(int i=0; i<debrises.size(); i++)
	{
		pStatic->debrises[i]->AddRef();
		debrises[i] = new cSimply3dx(pStatic->debrises[i]);
		Mats m;
		m.mult(model->GetPositionMats(),pStatic->debrises[i]->debrisPos);
		debrises[i]->SetPosition(m.se());
		debrises[i]->SetScale(m.scale());
		debrises[i]->setScene(this);
		debrises[i]->SetDiffuseTexture(model->GetDiffuseTexture(pStatic->debrises[i]->num_material));
	}
	RELEASE(pStatic);
	return debrises.size()>0;
}

