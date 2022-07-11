#include "StdAfxRD.h"
#include "scene.h"
#include "TileMap.h"
#include "Trail.h"
#include "ObjLibrary.h"
#include "cZPlane.h"
#include "CChaos.h"
#include "FogOfWar.h"
//#include "..\client\Silicon.h"
#include <typeinfo.h>
#include "..\3dx\Lib3dx.h"
#include "..\3dx\Simply3dx.h"
#include "..\shader\shaders.h"
#include "StaticMap.h"

bool cScene::is_sky_cubemap=true;

#ifdef C_CHECK_DELETE
static FILE* gb_objnotfree=NULL;
static void SaveKindObjNotFree(FILE* f,vector<cCheckDelete*>& data);
void WriteStack(FILE* f,cCheckDelete* p)
{
	string s;
	p->stack.GetStack(s);
	if(!s.empty())
		fprintf(f,s.c_str());
}
#endif

//#include <crtdbg.h>
//void mem_check()
//{
//#ifdef _DEBUG
//	xassert(_CrtCheckMemory()) ;
//#endif
//}

class FunctorMapZ:public FunctorGetZ
{
	class TerraInterface* terra;
	int terra_x,terra_y;
public:
	FunctorMapZ(cScene* pScene)
	{
		init(pScene);
	}
	void init(cScene* pScene)
	{
		cTileMap* pTileMap=pScene->GetTileMap();
		if(pTileMap)
		{
			VISASSERT(pTileMap);
			terra=pTileMap->GetTerra();
			terra_x=terra->SizeX();
			terra_y=terra->SizeY();
		}else
		{
			terra=NULL;
			terra_x=terra_y=0;
		}
	}
	float GetZ(float pos_x,float pos_y)
	{
		float out_z;
		int x=round(pos_x),y=round(pos_y);
		if(terra && x>=0 && x<terra_x && y>=0 && y<terra_y)
			out_z=(float)terra->GetZ(x,y);
		else
			out_z=0;
		return out_z;
	}
};

cScene::cScene()
{
	Size.set(0,0);
	dTime=0;
	dTimeInt=0;

	lightmap_type=0;
	SetShadowIntensity(sColor4f(0.5f,0.5f,0.5f,1));

	TileMap=NULL;
	TileNumber.set(0,0);
	tile_size=0;
	pFogOfWar=NULL;

	planar_node_param.x=0;
	planar_node_param.y=0;
	planar_node_param.z=1;
	planar_node_param.w=1;

	shadow_camera_offset.set(0, 0, 0);

	shadow_draw_node = new cCameraShadowMap(this);
	light_draw_node = new cCameraPlanarLight(this,false);
	light_objects_draw_node = new cCameraPlanarLight(this,true);
	mirage_draw_node = new cCameraMirageMap(this);
	floatZBufferCamera = new cCamera(this);
	pReflectionRenderTarget=NULL;
	ReflectionDrawNode=NULL;
	pReflectionZBuffer=NULL;
	disable_tilemap_visible_test=false;
	sun_direction.set(-1,0,-1);
	sun_direction.Normalize();
	separately_sun_dir_shadow=false;
	sun_dir_shadow = sun_direction;
	sun_ambient.set(1,1,1,1);
	sun_diffuse.set(1,1,1,1);
	sun_specular.set(1,1,1,1);
	pSkyCubemap=NULL;
	enable_fog_of_war=true;
	in_update_list=false;
	zreflection=0;
	enable_reflection=false;
	prev_graph_logic_quant=0;

	reflection_copy=new cCamera(this);
	reflection_bound_box.min.set(-1,-1,-1);//-1
	reflection_bound_box.max.set(+1,+1,+1);//+1

	hide_lights=false;
	hide_selfillumination=false;

	TerraFunctor=new FunctorMapZ(this);
	WaterFunctor=TerraFunctor;
	WaterFunctor->AddRef();

	circle_shadow_intensity.set(255,255,255,255);
}
cScene::~cScene()
{
	RELEASE(TerraFunctor);
	RELEASE(WaterFunctor);
	RELEASE(pSkyCubemap);
	DeleteAutoObject();
	Size.set(0,0);
	GetTilemapDetailTextures().Resize(0);

	
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

	RELEASE(reflection_copy);

	RELEASE(shadow_draw_node);
	RELEASE(light_draw_node);
	RELEASE(light_objects_draw_node);
	RELEASE(mirage_draw_node);
	RELEASE(floatZBufferCamera);
	RELEASE(pReflectionRenderTarget);
	RELEASE(ReflectionDrawNode);
	RELEASE(pReflectionZBuffer);
	VISASSERT(pFogOfWar==NULL);
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
			fprintf(f,"%s (%s)\n",node->file_name.c_str(),node->is_logic?"logic":"graphics");
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
			if(node->GetName())
				fprintf(f,"%s\n",node->GetName());
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
	gb_objnotfree=NULL;
	if(cVisGeneric::IsAssertEnabled())
		MessageBox(NULL,"Not all graph object is delete. See obj_notfree.txt","cVisGeneric::ErrorMessage()",MB_OK|MB_TOPMOST|MB_ICONSTOP);
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
//	if(dTime<1e-6f) return;
	MTAuto enter(lock_draw);

	// анимация объектов
	for(sGrid2d::iterator it=grid.begin();it!=grid.end();++it)
	{
		cBaseGraphObject* p=*it;
		if(p && p->GetAttr(ATTRUNKOBJ_DELETED)==0) 
		{
			p->Animate(dTime);
		}
	}
	if(TileMap)
		TileMap->Animate(dTime);

	// анимация источников света
	for(int i=0;i<GetNumberLight();i++)
		if(GetLight(i)&&GetLight(i)->GetAttr(ATTRUNKOBJ_DELETED)==0) 
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

void cScene::Draw(cCamera *DrawNode)
{
	MTAuto enter(lock_draw);
//	mem_check();
	RemoveEmptyStaticSimply3dx();

	//Неплохо бы автоматизировать этот процесс для всех child камер.
	if(shadow_draw_node)shadow_draw_node->ClearParent();

	//D3DSURFACE_DESC desc;
	//gb_RenderDevice3D->lpBackBuffer->GetDesc(&desc);
	//gb_RenderDevice3D->dtAdvance->CreateMirageMap(desc.Width,desc.Height);
	DrawNode->SetSecondRT(gb_RenderDevice3D->dtAdvance->GetAccessibleZBuffer());//Криво, для демы.

//	unsigned int fp=_controlfp(0,0);
//	_controlfp( _PC_24,  _MCW_PC ); 
	//PreDraw
	if(GetFogOfWar())
	{
		gb_RenderDevice3D->SetFogOfWar(true);
		sColor4f fog_color(GetFogOfWar()->GetFogColor());
		gb_RenderDevice3D->fog_of_war_color=*(D3DXVECTOR4*)&fog_color;
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
			
	VISASSERT(DrawNode->GetScene()==this);
	Animate();
	int i;

	DrawNode->PreDrawScene();//Очистка буферов в основном.
	if(shadow_draw_node)
		shadow_draw_node->PreDrawScene();
	if(light_draw_node)
		light_draw_node->PreDrawScene();
	if(light_objects_draw_node)
		light_objects_draw_node->PreDrawScene();
	if(ReflectionDrawNode)
		ReflectionDrawNode->PreDrawScene();
	if(mirage_draw_node)
		mirage_draw_node->PreDrawScene();
	if(floatZBufferCamera)
		floatZBufferCamera->PreDrawScene();

	if(TileMap)
	{
		AddShadowCamera(DrawNode);
		AddReflectionCamera(DrawNode);//temp bug
		AddMirageCamera(DrawNode);
		if (gb_VisGeneric->GetFloatZBufferType())
			AddFloatZBufferCamera(DrawNode);
		TileMap->PreDraw(DrawNode);
		if(!disable_tilemap_visible_test)
			DrawNode->EnableGridTest(TileNumber.x,TileNumber.y,tile_size);
	}

	for(i=0;i<grid.size();i++)
	{
		cBaseGraphObject* obj=grid[i];
		if(obj&&obj->GetAttr(ATTRUNKOBJ_IGNORE)==0) 
		{
			obj->PreDraw(DrawNode);
		}
	}

	for(i=0;i<GetNumberLight();i++)
		if(GetLight(i)&&GetLight(i)->GetAttr(ATTRLIGHT_DIRECTION)==0) 
			GetLight(i)->PreDraw(DrawNode);

	for(i=0;i<simply_objects.size();i++)
	{
		cStaticSimply3dx* pStatic=simply_objects[i].pStatic;
		pStatic->pActiveSceneList=&simply_objects[i].objects;
		pStatic->SetScene(this);
		pStatic->PreDraw(DrawNode);
	}


	if(TileMap)
		FixShadowMapCamera(DrawNode, shadow_draw_node);

	//Draw
	VISASSERT(DrawNode->GetScene()==this);

	DrawNode->DrawScene();
//	_clearfp();
//	_controlfp(fp,0xFFFFFFFFul);
	gb_RenderDevice3D->SetFogOfWar(false);
	gb_RenderDevice->SetClipRect(0,0,gb_RenderDevice->GetSizeX(),gb_RenderDevice->GetSizeY());
	circle_shadow.clear();

/*
	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,
													  //D3DCOLORWRITEENABLE_BLUE |
		                                              //D3DCOLORWRITEENABLE_GREEN |
													  D3DCOLORWRITEENABLE_RED |
													  D3DCOLORWRITEENABLE_ALPHA);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
	sColor4c c(255,255,255,255);
	for(int i=0;i<256;i++)
		gb_RenderDevice->DrawSprite(0,0,512,512,0,0,1,1,NULL,c);
*/
/*
	float f=gb_VisGeneric->GetInterpolationFactor();
	char str[256];
	sprintf(str,"%.3f",f);
	gb_RenderDevice->OutText(128,128,str,sColor4f(255,255,255));
*/
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
	xassert(0);
	return TraceUnified(pStart,pFinish-pStart,pTrace,true);
}

#define PREC_TRACE_RAY						17
bool cScene::TraceUnified(const Vect3f& in_start,const Vect3f& in_dir,Vect3f *pTrace,bool clamp_by_dir)
{
	if (!TileMap)
	{
		//xassert(TileMap);
		return false;
	}
	if(pTrace)pTrace->set(0,0,0);

	TerraInterface* terra=TileMap->GetTerra();
	Vect3f start,finish;
	sBox6f box;
	box.min.set(0,0,0);
	box.max.set(float(terra->SizeX()-1),float(terra->SizeY()-1),255.0f);

	Vect3f pStart,pFinish;
	if(!ClampRay(in_start,in_dir,
			  box,pStart,pFinish,clamp_by_dir))
			  return false;

	// Алгоритм прохода
	float dx = pFinish.x-pStart.x, dy = pFinish.y-pStart.y, dz = pFinish.z-pStart.z;
	float dxAbs=ABS(dx),dyAbs=ABS(dy),dzAbs=ABS(dz);
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
		if(pTrace) pTrace->set(pStart.x,pStart.y,(float)terra->GetZ(round(pStart.x),round(pStart.y)));
		return true;
	}

	int xb_=round(pStart.x*(1<<PREC_TRACE_RAY)),yb_=round(pStart.y*(1<<PREC_TRACE_RAY)),zb_=round(pStart.z*(1<<PREC_TRACE_RAY));
	// Переводим размеры в fixed-point формат
	int z_size = 256<<PREC_TRACE_RAY; // +1 чтобы корректно учитывать сравнение с округленным
	if (!clamp_by_dir)
	{// Луч
		int x_size = terra->SizeX()<<PREC_TRACE_RAY; 
		int y_size = terra->SizeY()<<PREC_TRACE_RAY;
		for(;xb_>=0 && xb_<x_size && yb_>=0 && yb_<y_size && zb_<z_size;
			xb_+=dx_,yb_+=dy_,zb_+=dz_)
			// Предполагается, что камера находится над миром
			if(terra->GetZ((xb_>>PREC_TRACE_RAY),(yb_>>PREC_TRACE_RAY))>=(zb_>>PREC_TRACE_RAY))
			{
				int ix=xb_>>PREC_TRACE_RAY,iy=yb_>>PREC_TRACE_RAY;
				if(pTrace) pTrace->set(float(ix),float(iy),(float)terra->GetZ(ix,iy));
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
			if(terra->GetZ((xb_>>PREC_TRACE_RAY),(yb_>>PREC_TRACE_RAY))>=(zb_>>PREC_TRACE_RAY))
			{
				int ix=xb_>>PREC_TRACE_RAY,iy=yb_>>PREC_TRACE_RAY;
				if(pTrace) pTrace->set(float(ix),float(iy),(float)terra->GetZ(ix,iy));
				return true;
			}
	}

	return false;
}

void cScene::AttachObjReal(cBaseGraphObject *obj)
{
	if(obj->GetKind()==KIND_LIGHT)
	{
		UnkLightArray.Attach(obj);
	}else
	if(obj->GetKind()==KIND_SIMPLY3DX)
	{
		AttachSimply3dx((cSimply3dx*)obj);
	}else
	{
		grid.Attach(obj);
	}

	obj->SetScene(this);
}
void cScene::DetachObjReal(cBaseGraphObject *UObj)
{
	if(UObj->GetKind()==KIND_SIMPLY3DX)
	{
		DetachSimply3dx((cSimply3dx*)UObj);
		return;
	}

	if(UObj==pFogOfWar)
		pFogOfWar=NULL;

	if(UObj==TileMap)
	{
		TileMap->terra->postInit(0);
		RELEASE(TileMap);
	}else
	if(UObj->GetKind()==KIND_LIGHT)
	{
		UnkLightArray.Detach((cUnkLight*)UObj);
	}else
	{
		grid.Detach(UObj);
	}
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
	Light->SetAttr(Attribute);

	if(pTexture) 
		Light->SetTexture(0,pTexture);
	Light->SetScene(this);
	return Light;
}

cUnkLight* cScene::CreateLightDetached(int Attribute, const char* TextureName)
{ // реализация cUnkLight
	xassert(!(Attribute&ATTRLIGHT_DIRECTION));//Теперь глобальный источник света задаётся при помощи функции SetSun
	cUnkLight *Light=new cUnkLight();
	Light->SetAttr(Attribute);

	if(TextureName) 
		//Light->SetTexture(0,GetTexLibrary()->GetElement(TextureName));
		Light->SetTexture(0,GetTexLibrary()->GetElement3D(TextureName));
	Light->SetScene(this);
	return Light;
}
//////////////////////////////////////////////////////////////////////////////////////////
cTileMap* cScene::CreateMap(TerraInterface* terra,int zeroplastnumber)
{
	VISASSERT(TileMap==NULL);
	MTG();

	cScene::Size=Vect2i(terra->SizeX(),terra->SizeY());
	TileMap=new cTileMap(this,terra);
	terra->postInit(TileMap);
	TileMap->SetBuffer(cScene::Size,zeroplastnumber);

	TileNumber=TileMap->GetTileNumber();
	tile_size=TileMap->GetTileSize().x;
	GetTilemapDetailTextures().Resize(zeroplastnumber+1);
	TerraFunctor->init(this);
	return TileMap;
}
 
cFogOfWar* cScene::CreateFogOfWar()
{
	VISASSERT(pFogOfWar==NULL);
	VISASSERT(GetTileMap());
	pFogOfWar=new cFogOfWar(GetTileMap()->GetTerra());
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
	pObject->SetScene(this);
	pObject->PutAttribute(ATTR3DX_HIDE_LIGHTS,hide_lights);
	pObject->EnableSelfIllumination(!hide_selfillumination);

//	if(gb_VisGeneric->GetLogicQuant()>50)
//		VTPause();

	return pObject;
}

cObject3dx* cScene::CreateLogic3dx(const char* fname, bool interpolated)
{
	start_timer_auto();

	cStatic3dx *pStatic=pLibrary3dx->GetElement(fname,NULL,true);
	if(pStatic==0) return 0;
	cObject3dx *pObject=new cObject3dx(pStatic,interpolated);
	return pObject;
}

//////////////////////////////////////////////////////////////////////////////////////////

cChaos* cScene::CreateChaos(Vect2f size,LPCSTR str_tex0,LPCSTR str_tex1,LPCSTR str_bump,int tile,bool enable_bump)
{
	cChaos* p=new cChaos(size,str_tex0,str_tex1,str_bump,tile,enable_bump);
	AttachObj(p);
	return p;
}

cPlane* cScene::CreatePlaneObj()
{
	cPlane *PlaneObj=new cPlane;
	AttachObj(PlaneObj);
	return PlaneObj;
}

//////////////////////////////////////////////////////////////////////////////////////////
/*
cTrail* cScene::CreateTrail(const char* TextureName,float TimeLife)
{ 
	cTrail *UObj=new cTrail(TimeLife);
	//UObj->SetTexture(0,GetTexLibrary()->GetElement(TextureName));
	UObj->SetTexture(0,GetTexLibrary()->GetElement3D(TextureName));
	AttachObj(UObj);
	return UObj;
}
*/
cEffect* cScene::CreateEffectDetached(EffectKey& el,c3dx* models,bool auto_delete_after_life)
{
//	mem_check();
	if (el.GetNeedTilemap() && !GetTileMap()) 
		return NULL;
	cEffect* UObj=new cEffect();
	UObj->SetAutoDeleteAfterLife(auto_delete_after_life);
	UObj->SetScene(this);
	UObj->Init(el,models);
//	AttachObj(UObj);
//	mem_check();
	return UObj;
}

//////////////////////////////////////////////////////////////////////////////////////////
/*
cBaseGraphObject* cScene::CreateElasticSphere(const char* TextureName1, const char* TextureName2)
{
	ElasticSphere *UObj=new ElasticSphere;
	//UObj->SetTexture(0,GetTexLibrary()->GetElement(TextureName1));
	//UObj->SetTexture(1,GetTexLibrary()->GetElement(TextureName2));
	UObj->SetTexture(0,GetTexLibrary()->GetElement3D(TextureName1));
	UObj->SetTexture(1,GetTexLibrary()->GetElement3D(TextureName2));
	AttachObj(UObj);
	return UObj;
}
*/
cCamera* cScene::CreateCamera()
{
	return new cCamera(this);
}

void cScene::SetShadowIntensity(const sColor4f& f)
{
	shadow_intensity=f;
}

cTexture* cScene::GetLightMap()
{
	if(CheckLightMapType() && IsIntensityShadow())
		return gb_RenderDevice3D->dtAdvance->GetLightMap();
	return gb_RenderDevice3D->dtFixed->GetLightMap();
}
cTexture* cScene::GetLightMapObjects()
{
	if(CheckLightMapType() && IsIntensityShadow())
		return gb_RenderDevice3D->dtAdvance->GetLightMapObjects();
	return gb_RenderDevice3D->dtFixed->GetLightMapObjects();
}

cTexture* cScene::GetShadowMap()
{
	if(CheckLightMapType())
		return gb_RenderDevice3D->dtAdvance->GetShadowMap();
	return gb_RenderDevice3D->dtFixed->GetShadowMap();
}

LPDIRECT3DSURFACE9 cScene::GetZBuffer()
{
	if(CheckLightMapType())
		return gb_RenderDevice3D->dtAdvance->GetZBuffer();
	return gb_RenderDevice3D->dtFixed->GetZBuffer();
}

int cScene::CheckLightMapType()
{
	return Option_ShadowType;
}

void cScene::CreateShadowmap()
{
	gb_RenderDevice3D->SetAdvance(true);
	gb_RenderDevice3D->dtFixed->DeleteShadowTexture();
	gb_RenderDevice3D->dtAdvance->DeleteShadowTexture();

	int width=256<<(Option_ShadowSizePower-1);

	lightmap_type = CheckLightMapType();

	gb_RenderDevice3D->dtFixed->CreateShadowTexture(width);

	if(Option_ShadowSizePower>0)
	{
		DrawType* draw=gb_RenderDevice3D->dtFixed;
		if(lightmap_type)
			draw=gb_RenderDevice3D->dtAdvance;

		if(!draw->CreateShadowTexture(width))
		{
			gb_VisGeneric->SetShadowType((eShadowType)(int)Option_ShadowType,0);
		}
	}

	float SizeLightMap=(float)TileMap->terra->SizeX();
	float focus=1/SizeLightMap;

	GetTexLibrary()->Compact();
}

Vect2f cScene::CalcZMinZMaxShadowReciver()
{
	Vect2f objectz=shadow_draw_node->CalcZMinZMaxShadowReciver();
/*	//Падение уже исправили. Падало из-за того что вызывалось когда реально не было теней.
	//Но так как это слишком мелкий баг правит, то закомментарили на всяк случай. 
	for(vector<ListSimply3dx>::iterator it=simply_objects.begin();it!=simply_objects.end();it++)
	{
		it->pStatic->AddZMinZMaxShadowReciver(shadow_draw_node->GetMatrix(),objectz);
	}
*/
	return objectz;
}

void cScene::FixShadowMapCamera(cCamera *DrawNode, cCamera *shadow_draw_node)
{
	start_timer_auto();
	if(!Option_IsShadowMap)
		return;
	if(!shadow_draw_node->GetParent())
		return;
	if(Option_ShadowTSM)
	{
		CalcShadowMapCameraTSM(DrawNode,shadow_draw_node);
		return;
	}
	sBox6f box=fix_shadow.box;
	MatXf LightMatrix=fix_shadow.light_matrix;

	Vect2f boxz=TileMap->CalcZ(shadow_draw_node);
	//Vect2f objectz=DrawNode->CalculateZMinMax(shadow_draw_node->GetMatrix());
	Vect2f objectz=CalcZMinZMaxShadowReciver();
	boxz.x=min(boxz.x,objectz.x);
	boxz.y=max(boxz.y,objectz.y);

	box.min.z+=boxz.x;//-DrawNode->GetZPlane().x;
	box.max.z=box.min.z+(boxz.y-boxz.x);

	Vect3f PosLight((box.min.x+box.max.x)*0.5f,(box.min.y+box.max.y)*0.5f,box.min.z);
	LightMatrix.trans()=-PosLight;

	Vect2f Focus(1/(box.max.x-box.min.x),1/(box.max.y-box.min.y));
	shadow_draw_node->SetFrustum(&Vect2f(0.5f,0.5f),&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),
		&Focus, &Vect2f(0,box.max.z-box.min.z));
	shadow_draw_node->SetPosition(LightMatrix);
}

void cScene::CalcShadowMapCamera(cCamera *DrawNode, cCamera *shadow_draw_node)
{
	if(!shadow_draw_node->GetParent())
		return;
	Vect3f LightDirection(0,0,-1);
	DrawNode->GetScene()->GetLightingShadow(&LightDirection);
	MatXf LightMatrix;
	LightMatrix.rot().zrow()=LightDirection;
	LightMatrix.rot().xrow().cross(Vect3f(1,0,0),LightMatrix.rot().zrow());
	LightMatrix.rot().xrow().Normalize();
	LightMatrix.rot().yrow().cross(LightMatrix.rot().zrow(),LightMatrix.rot().xrow());

	//const float bound_z=300.0f;
	const float bound_z=1000.0f;

	Mat3f LightMatrixInv;
	sBox6f box;
	LightMatrixInv.invert(LightMatrix.rot());
	calcVisMap(DrawNode,TileMap->GetTileNumber(),TileMap->GetTileSize(),LightMatrix.rot(),box);

	Vect2f prevz(box.min.z,box.max.z);
	box.min.z-=bound_z;
	box.max.z+=bound_z;

	Vect3f PosLight((box.min.x+box.max.x)*0.5f,(box.min.y+box.max.y)*0.5f,box.min.z);
	LightMatrix.trans()=-PosLight;

	Vect2f Focus(1/(box.max.x-box.min.x),1/(box.max.y-box.min.y));
	shadow_draw_node->SetFrustum(&Vect2f(0.5f,0.5f),&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),
		&Focus, &Vect2f(0,box.max.z-box.min.z));
	shadow_draw_node->SetPosition(LightMatrix);

//С одной стороны эта камера должна быть посчитана до того момента
//когда начнёт определяться, какие объекты видимы. С другой стороны она должна быть
//посчитанна позже, так как вызывается CalculateZMinMax
	fix_shadow.box=box;
	fix_shadow.light_matrix=LightMatrix;
}


void cScene::AddPlanarCamera(cCamera *DrawNode,bool light,bool toObjects)
{
	sBox6f box;
	calcVisMap(DrawNode,TileMap->GetTileNumber(),TileMap->GetTileSize(),Mat3f::ID,box);
	float dist_eps=1;
	if(box.max.x-box.min.x<dist_eps)
		box.max.x=box.min.x+dist_eps;
	if(box.max.y-box.min.y<dist_eps)
		box.max.y=box.min.y+dist_eps;

	planar_node_param.x=box.min.x;
	planar_node_param.y=box.min.y;
	planar_node_param.z=1/(box.max.x-box.min.x);
	planar_node_param.w=1/(box.max.y-box.min.y);

	
	cCamera* PlanarNode = light ? (toObjects ? light_objects_draw_node:light_draw_node) : shadow_draw_node;
	Vect3f PosLight;
	PosLight.x = (box.max.x+box.min.x)*0.5f;
	PosLight.y = (box.max.y+box.min.y)*0.5f;
	PosLight.z = 5000;

	Vect3f vShadow(0,0,-1);
	MatXf LightMatrix;
	LightMatrix.rot().xrow().cross(vShadow,Vect3f(0,1,0));
	LightMatrix.rot().yrow()=Vect3f(0,-1,0);
	LightMatrix.rot().zrow()=vShadow;
	LightMatrix.trans()=LightMatrix.rot().xform( -PosLight );

	DrawNode->SetCopy(PlanarNode);
	DrawNode->AttachChild(PlanarNode);

	Vect2f Focus(planar_node_param.z,planar_node_param.w);
	PlanarNode->SetAttribute(ATTRCAMERA_SHADOW|ATTRUNKOBJ_NOLIGHT);
	PlanarNode->ClearAttribute(ATTRCAMERA_PERSPECTIVE);
	PlanarNode->ClearAttribute(ATTRCAMERA_SHOWCLIP);
	PlanarNode->SetRenderTarget(light ? (toObjects? GetLightMapObjects() : GetLightMap()) : GetShadowMap(), NULL); 
	PlanarNode->SetFrustum(&Vect2f(0.5f,0.5f), &sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),
						   &Focus, &Vect2f(10,1e6f));
	PlanarNode->ClearAttribute(ATTRCAMERA_WRITE_ALPHA);
	
	PlanarNode->SetPosition(LightMatrix);
	PlanarNode->Attach(SCENENODE_OBJECT,TileMap); // рисовать источники света							   
}

void cScene::AddShadowCamera(cCamera* DrawNode)
{
	gb_RenderDevice3D->SetAdvance(true);
	if((CheckLightMapType() && GetShadowMap()==0) || (!CheckLightMapType() && GetLightMap()==0))
	{
		CreateShadowmap();
	}else 
	{
		bool change_shadowmap=false;
		if(GetShadowMap())
		if( (256<<(Option_ShadowSizePower-1))!=GetShadowMap()->GetWidth() || 
			lightmap_type!=CheckLightMapType())
		{
			change_shadowmap=true;
		}

		if(!Option_ShadowSizePower && lightmap_type!=CheckLightMapType())
			change_shadowmap=true;

		if(change_shadowmap)
			CreateShadowmap();
	}

	if(!IsIntensityShadow() || !Option_IsShadowMap) {
		AddPlanarCamera(DrawNode,true);
		AddPlanarCamera(DrawNode,true,true);
	} else if(GetShadowMap()) { // shadow
		if(Option_IsShadowMap) {
			AddLightCamera(DrawNode);
			if(Option_ShadowType == SHADOW_MAP_SELF)
			{
				AddPlanarCamera(DrawNode, true);
				AddPlanarCamera(DrawNode,true,true);
			}
		} else {
			AddPlanarCamera(DrawNode, false);
		}
	}
}

void cScene::AddLightCamera(cCamera *DrawNode)
{
	start_timer_auto();
	DrawNode->SetAttribute(ATTRCAMERA_ZMINMAX);
	DrawNode->SetCopy(shadow_draw_node);
	DrawNode->AttachChild(shadow_draw_node);
	shadow_draw_node->SetAttribute(ATTRCAMERA_SHADOWMAP|ATTRUNKOBJ_NOLIGHT);
	shadow_draw_node->ClearAttribute(ATTRCAMERA_PERSPECTIVE|ATTRCAMERA_ZMINMAX|ATTRCAMERA_SHOWCLIP);
	shadow_draw_node->SetRenderTarget(GetShadowMap(),GetZBuffer());

//	Vect2f z=CalcZ(DrawNode);

//	if(Option_ShadowType!=SHADOW_MAP_PERSPECTIVE)
//		z.x=30.0f;

//	Vect2f zplane=DrawNode->GetZPlane();
//	DrawNode->SetZPlaneTemp(z);

	CalcShadowMapCamera(DrawNode, shadow_draw_node);//Calc frustrum to TSM
	
	shadow_draw_node->Attach(SCENENODE_OBJECT, TileMap);

//	DrawNode->SetZPlaneTemp(zplane);
}
void cScene::AddMirageCamera(cCamera* DrawNode)
{
	if (!gb_RenderDevice3D->dtFixed->GetMirageMap())
	{
		D3DSURFACE_DESC desc;
		gb_RenderDevice3D->lpBackBuffer->GetDesc(&desc);
		gb_RenderDevice3D->dtFixed->CreateMirageMap(desc.Width,desc.Height);
	}
	DrawNode->SetCopy(mirage_draw_node);
	//DrawNode->AttachChild(mirage_draw_node);
	mirage_draw_node->SetAttribute(ATTRCAMERA_MIRAGE);
	mirage_draw_node->SetRenderTarget(gb_RenderDevice3D->dtFixed->GetMirageMap(),gb_RenderDevice3D->lpZBuffer);
	mirage_draw_node->SetFoneColor(sColor4c(128,128,128,0));
	mirage_draw_node->SetAttribute(ATTRCAMERA_NOCLEARTARGET);
	mirage_draw_node->Update();
}


void cScene::AddReflectionCamera(cCamera *DrawNode)
{
	if(ReflectionDrawNode)
	{ // reflection
		//float h_zeroplast=34;
		
		sPlane4f ReflectionPlane(0,0,-1,zreflection);

		DWORD clear=ReflectionDrawNode->GetAttr(ATTRCAMERA_NOCLEARTARGET);
		DrawNode->SetCopy(ReflectionDrawNode);
		ReflectionDrawNode->SetAttr(clear);

		DrawNode->AttachChild(ReflectionDrawNode);
		if(pReflectionRenderTarget)
			ReflectionDrawNode->SetRenderTarget(pReflectionRenderTarget,pReflectionZBuffer);
		ReflectionDrawNode->SetAttribute(ATTRCAMERA_REFLECTION|ATTRCAMERA_CLEARZBUFFER);
		ReflectionDrawNode->SetAttr(ATTRCAMERA_WRITE_ALPHA);
		ReflectionDrawNode->SetFoneColor(sColor4c(0,0,0,0));
		MatXf reflection_matrix;
		ReflectionPlane.GetReflectionMatrix(DrawNode->GetMatrix(),reflection_matrix);

		ReflectionDrawNode->GetMatrix()=reflection_matrix;
		ReflectionDrawNode->SetHReflection(int(zreflection));
		ReflectionDrawNode->Update();

		ReflectionDrawNode->SetCopy(reflection_copy);
		CMatrix offset_matrix;
		offset_matrix.identify();
		if(reflection_bound_box.max.y>reflection_bound_box.min.y+0.1f)
		{
			float mx=2.0f/(reflection_bound_box.max.x-reflection_bound_box.min.x);
			float my=2.0f/(reflection_bound_box.max.y-reflection_bound_box.min.y);
			float dx=-1-reflection_bound_box.min.x*mx;
			float dy=-1-reflection_bound_box.min.y*my;
			offset_matrix.set( 
				mx, 0, 0, 0,
				0, my, 0, 0,
				0, 0, 1, 0,
				dx,  dy, 0,   1 );
		}

		ReflectionDrawNode->matViewProj=ReflectionDrawNode->matViewProj*offset_matrix;
		//matViewProj=... Немножко хак, предполагается что нигде больше ReflectionDrawNode->Update() не вызывается.
		//Требуется переделка функции видимости для этого случая.
	}

}
void cScene::AddFloatZBufferCamera(cCamera* DrawNode)
{
	if (!gb_RenderDevice3D->dtFixed->GetFloatMap())
	{
		D3DSURFACE_DESC desc;
		gb_RenderDevice3D->lpBackBuffer->GetDesc(&desc);
		if (!gb_RenderDevice3D->dtFixed->CreateFloatTexture(desc.Width,desc.Height))
			return;
		//gb_RenderDevice3D->dtAdvance->CreateFloatTexture(512,512);
	}
	DrawNode->SetCopy(floatZBufferCamera);
	floatZBufferCamera->SetFrustum(&Vect2f(0.5f,0.5f), &sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),
		NULL, NULL);
	DrawNode->AttachChild(floatZBufferCamera);
	floatZBufferCamera->SetRenderTarget(gb_RenderDevice3D->dtFixed->GetFloatMap(),gb_RenderDevice3D->dtFixed->GetFloatZBuffer());
	floatZBufferCamera->SetAttribute(ATTRCAMERA_FLOAT_ZBUFFER);
	floatZBufferCamera->Attach(SCENENODE_OBJECT, TileMap);
	floatZBufferCamera->Update();

}

void cScene::DeleteDefaultResource()
{
	DeleteReflectionSurface();
}

void cScene::RestoreDefaultResource()
{
	CreateReflectionSurface();
}

void cScene::CreateReflectionSurface()
{
	if(ReflectionDrawNode)
		DeleteReflectionSurface();

	if(!enable_reflection)
		return;

	int xysize=1024;	
	HRESULT hr=gb_RenderDevice3D->lpD3DDevice->CreateDepthStencilSurface(xysize, xysize, 
		D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, TRUE, &pReflectionZBuffer, NULL);
	if(FAILED(hr))
	{	
		xassert(0);
		RELEASE(pReflectionZBuffer);
		return;
	}

	pReflectionRenderTarget=GetTexLibrary()->CreateRenderTexture(xysize,xysize,TEXTURE_RENDER32,false);

	ReflectionDrawNode=CreateCamera();
	ReflectionDrawNode->SetRenderTarget(pReflectionRenderTarget,pReflectionZBuffer);
	ReflectionDrawNode->Update();
}

void cScene::DeleteReflectionSurface()
{
	RELEASE(pReflectionRenderTarget);
	RELEASE(ReflectionDrawNode);
	RELEASE(pReflectionZBuffer);
}

void cScene::EnableReflection(bool enable)
{
	if(!gb_RenderDevice3D->IsPS20())
		enable=false;

	enable_reflection=enable;
	if(!enable)
	{
		DeleteReflectionSurface();
		return;
	}

	if(ReflectionDrawNode)
		return;

	CreateReflectionSurface();
}

void cScene::DisableTileMapVisibleTest()
{
	VISASSERT(!TileMap && "This function must called before create tilemap");
	disable_tilemap_visible_test=true;
}

void cScene::DeleteAutoObject()
{
	MTG();
	UpdateLists(INT_MAX);

	for(int i=0;i<grid.size();i++)
	{
		cBaseGraphObject* obj=grid[i];
		if(obj) 
		{
			cEffect* eff=dynamic_cast<cEffect*>(obj);
			if(eff)
			if(eff->IsAutoDeleteAfterLife())
			{
				if(eff->Release()<=0)
					i--;
			}
		}
	}
	UpdateLists(INT_MAX);
}

void cScene::SetSun(Vect3f direction,sColor4f ambient,sColor4f diffuse,sColor4f specular)
{
	sun_direction=direction;
	sun_direction.Normalize();
	SetSunColor(ambient,diffuse,specular);
}
void cScene::SetSunShadowDir(Vect3f direction)
{
	separately_sun_dir_shadow=true;
	sun_dir_shadow = direction;
}

void cScene::SetSunDirection(Vect3f direction)
{
	sun_direction=direction;
	sun_direction.Normalize();
}

void cScene::SetSunColor(sColor4f ambient,sColor4f diffuse,sColor4f specular)
{
	sun_ambient=ambient;
	sun_diffuse=diffuse;
	sun_specular=specular;
/*  Ни в коем слуячае не резать значения. Они могут быть больше 1.
	sun_ambient.r=clamp(sun_ambient.r, 0.0f, 1.0f);
	sun_ambient.g=clamp(sun_ambient.g, 0.0f, 1.0f);
	sun_ambient.b=clamp(sun_ambient.b, 0.0f, 1.0f);

	sun_diffuse.r=clamp(sun_diffuse.r, 0.0f, 1.0f);
	sun_diffuse.g=clamp(sun_diffuse.g, 0.0f, 1.0f);
	sun_diffuse.b=clamp(sun_diffuse.b, 0.0f, 1.0f);

	sun_specular.r=clamp(sun_specular.r, 0.0f, 1.0f);
	sun_specular.g=clamp(sun_specular.g, 0.0f, 1.0f);
	sun_specular.b=clamp(sun_specular.b, 0.0f, 1.0f);
*/
}

void cScene::GetLighting(Vect3f *LightDirection)
{
	*LightDirection=sun_direction;
}
void cScene::GetLightingShadow(Vect3f *LightDirection)
{
	if(separately_sun_dir_shadow)
		*LightDirection=sun_dir_shadow;
	else
		*LightDirection=sun_direction;
}

void cScene::GetLighting(sColor4f &Ambient,sColor4f &Diffuse,sColor4f &Specular,Vect3f &LightDirection)
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

void cScene::AddCircleShadow(const Vect3f& pos,float r,sColor4c intensity)
{
	CircleShadow cs;
	cs.pos=pos;
	cs.radius=r;
	cs.color=intensity;
	circle_shadow.push_back(cs);

}

///////////Для теста
#define DW_AS_FLT(DW) (*(FLOAT*)&(DW))
#define FLT_AS_DW(F) (*(DWORD*)&(F))
#define FLT_SIGN(F) ((FLT_AS_DW(F) & 0x80000000L))
#define ALMOST_ZERO(F) ((FLT_AS_DW(F) & 0x7f800000L)==0)
#define IS_SPECIAL(F)  ((FLT_AS_DW(F) & 0x7f800000L)==0x7f800000L)

////////////////
void calcVisMap(cCamera *DrawNode, Vect2i TileNumber,Vect2i TileSize,const D3DXMATRIX& direction,sBox6f& box);
Vect2f debug_zminmax(100,1000);
void cScene::CalcShadowMapCameraTSM(cCamera *DrawNode, cCamera* ShadowDrawNode)
{
    float m_fTSM_Delta = 0.4f;//0.52f;//Может быть 0..1, влияет на каком растоянии получается более точные тени.
    //  get the near and the far plane (points) in eye space.
    D3DXVECTOR3 frustumPnts[8];

    //Обязательно надо порезать Frustum по zmin,zmax для shadow reciever
	Vect2f zminmax=CalcZMinZMaxShadowReciver();
	zminmax.y+=500;
	Vect2f old_zplane=DrawNode->GetZPlane();
	MatXf old_pos=DrawNode->GetPosition();

	{
		Vect2f zminmax_tilemap=TileMap->CalcZ(DrawNode);
		zminmax_tilemap.x=max(zminmax_tilemap.x,old_zplane.x);
		zminmax_tilemap.y=min(zminmax_tilemap.y,old_zplane.y);

		zminmax.x=min(zminmax_tilemap.x,zminmax.x);
		zminmax.y=max(zminmax_tilemap.y,zminmax.y);
	}

	DrawNode->SetPosition(MatXf::ID);
	DrawNode->SetFrustum(0,0,0,&zminmax);//Для теста + дублирует последний код.
	debug_zminmax=zminmax;
	DrawNode->GetFrustumPoint(// autocomputes all the extrema points
		*(Vect3f*)&frustumPnts[0],
		*(Vect3f*)&frustumPnts[1],
		*(Vect3f*)&frustumPnts[2],
		*(Vect3f*)&frustumPnts[3],
		*(Vect3f*)&frustumPnts[4],
		*(Vect3f*)&frustumPnts[5],
		*(Vect3f*)&frustumPnts[6],
		*(Vect3f*)&frustumPnts[7]
		);
	DrawNode->SetPosition(old_pos);
	DrawNode->SetFrustum(0,0,0,&old_zplane);

	D3DXVECTOR3 frustumPntsView[8];
	for(int i=0;i<8;i++)
		frustumPntsView[i]=frustumPnts[i];

	//frustumPnts сейчас в DrawNode->matView пространстве.

    //   we need to transform the eye into the light's post-projective space.
    //   however, the sun is a directional light, so we first need to find an appropriate
    //   rotate/translate matrix, before constructing an ortho projection.
    //   this matrix is a variant of "light space" from LSPSMs, with the Y and Z axes permuted

	Vect3f LightDirection(0,0,-1);
	DrawNode->GetScene()->GetLightingShadow(&LightDirection);
	D3DXVECTOR3 m_lightDir=*(Vect3f*)&LightDirection;
	m_lightDir=-m_lightDir;

    D3DXVECTOR3 leftVector, upVector, viewVector;
    const D3DXVECTOR3 eyeVector( 0.f, 0.f, -1.f );  //  eye is always -Z in eye space

    //  code copied straight from BuildLSPSMProjectionMatrix
    D3DXVec3TransformNormal( &upVector, &m_lightDir, DrawNode->matView);  // lightDir is defined in eye space, so xform it
    D3DXVec3Cross( &leftVector, &upVector, &eyeVector );
    D3DXVec3Normalize( &leftVector, &leftVector );
    D3DXVec3Cross( &viewVector, &upVector, &leftVector );

    D3DXMATRIX lightSpaceBasis;  
    lightSpaceBasis._11 = leftVector.x; lightSpaceBasis._12 = viewVector.x; lightSpaceBasis._13 = -upVector.x; lightSpaceBasis._14 = 0.f;
    lightSpaceBasis._21 = leftVector.y; lightSpaceBasis._22 = viewVector.y; lightSpaceBasis._23 = -upVector.y; lightSpaceBasis._24 = 0.f;
    lightSpaceBasis._31 = leftVector.z; lightSpaceBasis._32 = viewVector.z; lightSpaceBasis._33 = -upVector.z; lightSpaceBasis._34 = 0.f;
    lightSpaceBasis._41 = 0.f;          lightSpaceBasis._42 = 0.f;          lightSpaceBasis._43 = 0.f;        lightSpaceBasis._44 = 1.f;

    //  rotate the view frustum into light space
    D3DXVec3TransformCoordArray( frustumPnts, sizeof(D3DXVECTOR3), frustumPnts, sizeof(D3DXVECTOR3), &lightSpaceBasis, sizeof(frustumPnts)/sizeof(D3DXVECTOR3) );

    //  build an off-center ortho projection that translates and scales the eye frustum's 3D AABB to the unit cube
	sBox6f frustumBox;
	frustumBox.SetBox((Vect3f*)frustumPnts,sizeof(frustumPnts) / sizeof(D3DXVECTOR3));

    //  also - transform the shadow caster bounding boxes into light projective space.  we want to translate along the Z axis so that
    //  all shadow casters are in front of the near plane.

	sBox6f casterBox;//Ограничивает создающих тени.
	D3DXMATRIX pView_lightSpaceBasis;

	D3DXMatrixMultiply( &pView_lightSpaceBasis, DrawNode->matView, &lightSpaceBasis );
	calcVisMap(DrawNode,TileMap->GetTileNumber(),TileMap->GetTileSize(),pView_lightSpaceBasis,casterBox);

	casterBox=ShadowDrawNode->CalcShadowReciverInSpace(pView_lightSpaceBasis);

    float min_z = min( casterBox.min.z, frustumBox.min.z );//Неправильно считается поэтому minz такой код (-200).
    float max_z = max( casterBox.max.z, frustumBox.max.z );
//	float min_z = casterBox.min.z;
//	float max_z = casterBox.max.z;
//	float min_z = frustumBox.min.z;
//	float max_z = frustumBox.max.z;

    if ( min_z <= 0.f )
    {
		float min_z_offset=1;
        D3DXMATRIX lightSpaceTranslate;
        D3DXMatrixTranslation( &lightSpaceTranslate, 0.f, 0.f, -min_z + min_z_offset );
        max_z = -min_z + max_z + min_z_offset;
        min_z = min_z_offset;
        D3DXMatrixMultiply ( &lightSpaceBasis, &lightSpaceBasis, &lightSpaceTranslate );
        D3DXVec3TransformCoordArray( frustumPnts, sizeof(D3DXVECTOR3), frustumPnts, sizeof(D3DXVECTOR3), &lightSpaceTranslate, sizeof(frustumPnts)/sizeof(D3DXVECTOR3) );
        frustumBox.SetBox((Vect3f*)frustumPnts, sizeof(frustumPnts)/sizeof(D3DXVECTOR3) );
    }

//	max_z=(max_z-min_z)*1e-2f+min_z;
	D3DXMatrixMultiply( &pView_lightSpaceBasis, DrawNode->matView, &lightSpaceBasis );

    D3DXMATRIX lightSpaceOrtho;
    D3DXMatrixOrthoOffCenterLH( &lightSpaceOrtho, frustumBox.min.x, frustumBox.max.x, frustumBox.min.y, frustumBox.max.y, 
		min_z, max_z );//Про min_z, max_z подумать здесь.

//	D3DXMatrixMultiply( &pView_lightSpaceBasis, &pView_lightSpaceBasis, &lightSpaceOrtho );
//	sBox6f casterBoxOrto=ShadowDrawNode->CalcShadowReciverInSpace(pView_lightSpaceBasis);

	//  transform the view frustum by the new matrix
    D3DXVec3TransformCoordArray( frustumPnts, sizeof(D3DXVECTOR3), frustumPnts, sizeof(D3DXVECTOR3), &lightSpaceOrtho, sizeof(frustumPnts)/sizeof(D3DXVECTOR3) );


    D3DXVECTOR2 centerPts[2];
    //  near plane
    centerPts[0].x = 0.25f * (frustumPnts[4].x + frustumPnts[5].x + frustumPnts[6].x + frustumPnts[7].x);
    centerPts[0].y = 0.25f * (frustumPnts[4].y + frustumPnts[5].y + frustumPnts[6].y + frustumPnts[7].y);
    //  far plane
    centerPts[1].x = 0.25f * (frustumPnts[0].x + frustumPnts[1].x + frustumPnts[2].x + frustumPnts[3].x);
    centerPts[1].y = 0.25f * (frustumPnts[0].y + frustumPnts[1].y + frustumPnts[2].y + frustumPnts[3].y);

    D3DXVECTOR2 centerOrig = (centerPts[0] + centerPts[1])*0.5f;

    D3DXMATRIX trapezoid_space;

    D3DXMATRIX xlate_center(           1.f,           0.f, 0.f, 0.f,
                                        0.f,           1.f, 0.f, 0.f,
                                        0.f,           0.f, 1.f, 0.f,
                                -centerOrig.x, -centerOrig.y, 0.f, 1.f );

    float half_center_len = D3DXVec2Length( &D3DXVECTOR2(centerPts[1] - centerOrig) );
    float x_len = centerPts[1].x - centerOrig.x;
    float y_len = centerPts[1].y - centerOrig.y;

    float cos_theta = x_len / half_center_len;
    float sin_theta = y_len / half_center_len;

    D3DXMATRIX rot_center( cos_theta, -sin_theta, 0.f, 0.f,
                            sin_theta,  cos_theta, 0.f, 0.f,
                                    0.f,        0.f, 1.f, 0.f,
                                    0.f,        0.f, 0.f, 1.f );

    //  this matrix transforms the center line to y=0.
    //  since Top and Base are orthogonal to Center, we can skip computing the convex hull, and instead
    //  just find the view frustum X-axis extrema.  The most negative is Top, the most positive is Base
    //  Point Q (trapezoid projection point) will be a point on the y=0 line.
    D3DXMatrixMultiply( &trapezoid_space, &xlate_center, &rot_center );
    D3DXVec3TransformCoordArray( frustumPnts, sizeof(D3DXVECTOR3), frustumPnts, sizeof(D3DXVECTOR3), &trapezoid_space, sizeof(frustumPnts)/sizeof(D3DXVECTOR3) );

    sBox6f frustumAABB2D;
	frustumAABB2D.SetBox((Vect3f*)frustumPnts,sizeof(frustumPnts) / sizeof(D3DXVECTOR3));

    float x_scale = max( fabsf(frustumAABB2D.max.x), fabsf(frustumAABB2D.min.x) );
    float y_scale = max( fabsf(frustumAABB2D.max.y), fabsf(frustumAABB2D.min.y) );

    x_scale = 1.f/x_scale;
    y_scale = 1.f/y_scale;

    //  maximize the area occupied by the bounding box
    D3DXMATRIX scale_center( x_scale, 0.f, 0.f, 0.f,
                                0.f, y_scale, 0.f, 0.f,
                                0.f,     0.f, 1.f, 0.f,
                                0.f,     0.f, 0.f, 1.f );

    D3DXMatrixMultiply( &trapezoid_space, &trapezoid_space, &scale_center );

    //  scale the frustum AABB up by these amounts (keep all values in the same space)
    frustumAABB2D.min.x *= x_scale;
    frustumAABB2D.max.x *= x_scale;
    frustumAABB2D.min.y *= y_scale;
    frustumAABB2D.max.y *= y_scale;

    //  compute eta.
    float lambda = frustumAABB2D.max.x - frustumAABB2D.min.x;
    float delta_proj = m_fTSM_Delta * lambda; //focusPt.x - frustumAABB2D.minPt.x;

    const float xi = -0.6f;  // 80% line

    float eta = (lambda*delta_proj*(1.f+xi)) / (lambda*(1.f-xi)-2.f*delta_proj);

    //  compute the projection point a distance eta from the top line.  this point is on the center line, y=0
    D3DXVECTOR2 projectionPtQ( frustumAABB2D.max.x + eta, 0.f );

    //  find the maximum slope from the projection point to any point in the frustum.  this will be the
    //  projection field-of-view
    float max_slope = -1e32f;
    float min_slope =  1e32f;

    for ( int i=0; i < sizeof(frustumPnts)/sizeof(D3DXVECTOR3); i++ )
    {
        D3DXVECTOR2 tmp( frustumPnts[i].x*x_scale, frustumPnts[i].y*y_scale );
        float x_dist = tmp.x - projectionPtQ.x;
        if ( !(ALMOST_ZERO(tmp.y) || ALMOST_ZERO(x_dist)))
        {
            max_slope = max(max_slope, tmp.y/x_dist);
            min_slope = min(min_slope, tmp.y/x_dist);
        }
    }

    float xn = eta;
    float xf = lambda + eta;

    D3DXMATRIX ptQ_xlate(-1.f, 0.f, 0.f, 0.f,
                            0.f, 1.f, 0.f, 0.f,
                            0.f, 0.f, 1.f, 0.f,
                            projectionPtQ.x, 0.f, 0.f, 1.f );
    D3DXMatrixMultiply( &trapezoid_space, &trapezoid_space, &ptQ_xlate );

    //  this shear balances the "trapezoid" around the y=0 axis (no change to the projection pt position)
    //  since we are redistributing the trapezoid, this affects the projection field of view (shear_amt)
    float shear_amt = (max_slope + fabsf(min_slope))*0.5f - max_slope;

	if(true)
	{//Идея shear пока находится выше моего понимания. Просто уменьшаем, чтобы влезло.
		shear_amt=0;
		max_slope=max(max_slope,fabsf(min_slope));
	}else
	{
		max_slope = max_slope + shear_amt;
	}

//	float y_move=(max_slope+min_slope)/2;
    D3DXMATRIX trapezoid_shear( 1.f, shear_amt, 0.f, 0.f,
                                0.f,       1.f, 0.f, 0.f,
                                0.f,       0.f, 1.f, 0.f,
                                0.f,       0.f, 0.f, 1.f );

    D3DXMatrixMultiply( &trapezoid_space, &trapezoid_space, &trapezoid_shear );

	frustumBox.SetBox((Vect3f*)frustumPnts, sizeof(frustumPnts)/sizeof(D3DXVECTOR3) );//Для теста, последующая строчка иначе странно смотрится
    float z_aspect = (frustumBox.max.z-frustumBox.min.z) / (frustumAABB2D.max.y-frustumAABB2D.min.y);
//	float z_aspect=2.0f/max_slope;//НЕ верно, должно явно от frustumBox зависеть.
    
    //  perform a 2DH projection to 'unsqueeze' the top line.
    //D3DXMATRIX trapezoid_projection(  xf/(xf-xn),   0.f,		   0.f, 1.f,
    //                                  0.f,			1.f/max_slope, 0.f, 0.f,
    //                                  0.f,			0.f,1.f/(z_aspect*max_slope), 0.f,
    //                                -xn*xf/(xf-xn), 0.f,		   0.f, 0.f );
    D3DXMATRIX trapezoid_projection(  xf/(xf-xn),   0.f,		   0.f, 1.f,
                                      0.f,			1.f/max_slope, 0.f, 0.f,
                                      0.f,			0.f,		   0.26f, 0.f,
                                    -xn*xf/(xf-xn), 0.f,		   0.f, 0.f );

	//xn,xf - явно считаются неправильно, используется сотая часть z буфера!

    D3DXMatrixMultiply( &trapezoid_space, &trapezoid_space, &trapezoid_projection );

    //  the x axis is compressed to [0..1] as a result of the projection, so expand it to [-1,1]
    D3DXMATRIX biasedScaleX( 2.f, 0.f, 0.f, 0.f,
                                0.f, 1.f, 0.f, 0.f,
                                0.f, 0.f, 1.f, 0.f,
                            -1.f, 0.f, 0.f, 1.f );
    D3DXMatrixMultiply( &trapezoid_space, &trapezoid_space, &biasedScaleX );

    D3DXMatrixMultiply( &trapezoid_space, &lightSpaceOrtho, &trapezoid_space );
    D3DXMatrixMultiply( &trapezoid_space, &lightSpaceBasis, &trapezoid_space );

	D3DXVECTOR3 frustumPntsProj[8];
	D3DXVec3TransformCoordArray( frustumPntsProj, sizeof(D3DXVECTOR3), frustumPntsView, sizeof(D3DXVECTOR3), &trapezoid_space, sizeof(frustumPnts)/sizeof(D3DXVECTOR3) );
    sBox6f frustumProj;
	frustumProj.SetBox((Vect3f*)frustumPntsProj,sizeof(frustumPnts) / sizeof(D3DXVECTOR3));

	{
		D3DXMATRIX trapezoid_0;
		D3DXMatrixMultiply( &trapezoid_0, &trapezoid_projection, &biasedScaleX );
		D3DXVECTOR4 out_pos;
		D3DXVECTOR3 in0(0,0,0);
		D3DXVec3Transform(&out_pos, &in0, &trapezoid_0);

		int k=0;
	}

	//Мы пытались фокусировать вначале, но это криво несколько.
	//Надо возможно раскомментарить этот код!
    // now, focus on shadow receivers.
    if ( false )
    {
		D3DXMATRIX trapezoid_view;
		D3DXMatrixMultiply(&trapezoid_view, DrawNode->matView, &trapezoid_space );
		sBox6f rcvrBox=DrawNode->CalcShadowReciverInSpace(trapezoid_view);
        //  never shrink the box, only expand it.
        rcvrBox.max.x = min( 1.f, rcvrBox.max.x );
        rcvrBox.min.x = max(-1.f, rcvrBox.min.x );
        rcvrBox.max.y = min( 1.f, rcvrBox.max.y );
        rcvrBox.min.y = max(-1.f, rcvrBox.min.y );
        float boxWidth  = rcvrBox.max.x - rcvrBox.min.x;
        float boxHeight = rcvrBox.max.y - rcvrBox.min.y;
        
        //  the receiver box is degenerate, this will generate specials (and there shouldn't be any shadows, anyway).
        if (false &&  (!(ALMOST_ZERO(boxWidth) || ALMOST_ZERO(boxHeight))) )
        {
            //  the divide by two's cancel out in the translation, but included for clarity
            float boxX = (rcvrBox.max.x+rcvrBox.min.x) / 2.f;
            float boxY = (rcvrBox.max.y+rcvrBox.min.y) / 2.f;
            D3DXMATRIX trapezoidUnitCube( 2.f/boxWidth,                 0.f, 0.f, 0.f,
                                                0.f,       2.f/boxHeight, 0.f, 0.f,
                                                0.f,                 0.f, 1.f, 0.f,
                                    -2.f*boxX/boxWidth, -2.f*boxY/boxHeight, 0.f, 1.f );
            D3DXMatrixMultiply( &trapezoid_space, &trapezoid_space, &trapezoidUnitCube );
        }
    }

//    D3DXMatrixMultiply( &trapezoid_space, &lightSpaceBasis, &lightSpaceOrtho);

    D3DXMatrixMultiply( ShadowDrawNode->matViewProj, DrawNode->matView, &trapezoid_space );
	
	ShadowDrawNode->UpdateViewProjScr();
}

ContainerMiniTextures& cScene::GetTilemapDetailTextures()
{
	return MiniTextures;
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
			(*it)->PutAttr(ATTR3DX_HIDE_LIGHTS,hide);
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
	pSimply->SetScene(this);
	return pSimply;
}

void cScene::AttachSimply3dx(cSimply3dx* pSimply)
{
	pSimply->SetScene(this);

	ListSimply3dx* lib=NULL;
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

void cScene::AttachObj(cBaseGraphObject *UnkObj)
{
	MTAuto mtauto(critial_attach);
	xassert(!UnkObj->GetAttr(ATTRUNKOBJ_ATTACHED));
	UnkObj->SetAttr(ATTRUNKOBJ_ATTACHED);
#ifdef _DEBUG
	vector<AddData>::iterator it;
	cBaseGraphObject* pFind=NULL;
	FOR_EACH(add_list,it)
	if(it->object==UnkObj)
		pFind=it->object;
	xassert(pFind==NULL);
#endif
	UnkObj->SetScene(this);
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

void cScene::DetachObj(cBaseGraphObject *UnkObj)
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
/*
		if(!erase_list.empty())
		{
			int back_quant=erase_list.back().quant;
			VISASSERT(back_quant<quant);
			if(!(back_quant<quant))
			{
				sErase& back_list=erase_list.back();
				list<cBaseGraphObject*>::iterator it;
				FOR_EACH(back_list.erase_list,it)
				{
					cBaseGraphObject* obj=*it;
					const char* file_name=NULL;
					if(c3dx* node=dynamic_cast<c3dx*>(obj))
					{
						file_name=node->GetFileName();
					}

					if(file_name)
						kdMessage("3d", XBuffer() < "Объект удалён слишком поздно:" < file_name);
				}
			}
		}
*/
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
			list<cBaseGraphObject*>::iterator itl;
			for(itl=e.erase_list.begin();itl!=e.erase_list.end();)
			{
				vector<AddData>::iterator it_delete;
				cBaseGraphObject* pFind=NULL;
				FOR_EACH(add_list,it_delete)
				if(it_delete->object==*itl)
				{
					pFind=it_delete->object;
					break;
				}

				if(pFind)
				{
					cBaseGraphObject* obj=*itl;
					if(false)
					{
						obj->Release();//Удаляем объект еще не добавленный в список.
					}else
					{
						xassert(obj->GetRef()==1);
						xassert(obj->GetAttr(ATTRUNKOBJ_DELETED));
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
			list<cBaseGraphObject*>::iterator itl;
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
		VISASSERT(erase_list.empty());
}

void cScene::BuildTree()
{
	if(!TileMap)
	{
		//Если нет карты - использовать сферические источники света.
		tree.clear();

		bool is_spherical=false;
		for(int i=0;i<GetNumberLight();i++)
		{
			cUnkLight* ULight=GetLight(i);
			if(ULight && ULight->GetAttr(ATTRLIGHT_SPHERICAL_OBJECT|ATTRLIGHT_SPHERICAL_TERRAIN) && !ULight->GetAttr(ATTRLIGHT_IGNORE))
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
	sColor4f color;
};

static void SceneLightProc(cObject3dx* obj,void* param)
{
	SceneLightProcParam& p=*(SceneLightProcParam*)param;
	int kind=obj->GetKind();
	VISASSERT(kind==KIND_OBJ_3DX);
	cObject3dx* node=obj;
	node->AddLight(p.light);
}

void cScene::CaclulateLightAmbient()
{
	if(TileMap)return;

	for(int i=0;i<GetNumberLight();i++)
	{
		cUnkLight* ULight=GetLight(i);
		if(ULight && ULight->GetAttr(ATTRLIGHT_SPHERICAL_OBJECT|ATTRLIGHT_SPHERICAL_TERRAIN) && !ULight->GetAttr(ATTRLIGHT_IGNORE))
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

sColor4f cScene::GetPlainLitColor()
{
	sColor4f color(GetTileMap()->GetDiffuse());
	float diffuse_n=-GetSunDirection().z;
	color.r=color.r*diffuse_n+color.a;
	color.g=color.g*diffuse_n+color.a;
	color.b=color.b*diffuse_n+color.a;
	return color;
}

bool cScene::CreateDebrisesDetached(const c3dx* model,vector<cSimply3dx*>& debrises)
{
	cStatic3dx* pStatic=pLibrary3dx->GetElement(model->GetFileName(),NULL,false);
	if(pStatic==NULL)
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
		debrises[i]->SetScene(this);
		debrises[i]->SetDiffuseTexture(model->GetDiffuseTexture(pStatic->debrises[i]->num_material));
	}
	RELEASE(pStatic);
	return debrises.size()>0;
}

UNLOAD_ERROR UnloadSharedResource(const char* file_name,bool logging)
{
	UNLOAD_ERROR ret=UNLOAD_E_NOTFOUND;
	if(file_name && file_name[0])
	{
		string norm_name;
		normalize_path(file_name,norm_name);
		const char* dot3dx=".3DX";
		int size_dot3dx=strlen(dot3dx);
		const char* end_3dx=norm_name.c_str()+norm_name.size()-size_dot3dx;
		if(norm_name.size()>size_dot3dx && strcmp(end_3dx,dot3dx)==0)
		{
			pLibrary3dx->Unload(file_name,true);
			ret=pLibrary3dx->Unload(file_name,false);
		}else
			ret=GetTexLibrary()->Unload(file_name);
	}
	if(logging)
	{
		if(ret==UNLOAD_E_NOTFOUND)
			kdMessage("3d", XBuffer() < "File not found" < file_name);
	}
	return ret;
}

void cScene::SelectWaterFunctorZ(FunctorGetZ* pnew)
{
	RELEASE(WaterFunctor);
	WaterFunctor=pnew;
	WaterFunctor->AddRef();
}
