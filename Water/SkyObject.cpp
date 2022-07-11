#include "stdafx.h"
#include "Water.h"
#include "SkyObject.h"
#include "Render\src\RenderCubemap.h"
#include "Render\src\Gradients.h"
#include "Render\Src\TileMap.h"
#include "Render\Src\Scene.h"
#include "Render\D3D\D3DRender.h"
#include "Render\3dx\Node3dx.h"
#include "Render\Src\VisGeneric.h"
#include "Serialization\Serialization.h"
#include "Serialization\ResourceSelector.h"
#include "Serialization\RangedWrapper.h"
#include "VistaRender\StreamInterpolation.h"
#include "Serialization\EnumDescriptor.h"
#include "Terra\vmap.h"
#include "DebugPrm.h"
#include "DebugUtil.h"
#include "Environment\Environment.h"

//cRenderSky потомок от cRenderCubemap отвечает за отражения неба в воде.

//Конструктор, в качестве параметра передается указатель на cSkyObj

BEGIN_ENUM_DESCRIPTOR(SkyElementType, "Тип элемента неба")
REGISTER_ENUM(SKY_ELEMENT_DAY, "Дневной")
REGISTER_ENUM(SKY_ELEMENT_NIGHT, "Ночной")
REGISTER_ENUM(SKY_ELEMENT_DAYNIGHT, "Круглосуточный")
END_ENUM_DESCRIPTOR(SkyElementType)

cSunMoonObj::cSunMoonObj()
: BaseGraphObject(SCENENODE_OBJECT)
{
	SunTexture = 0;
	MoonTexture = 0;
	isDay = true;
	scale = max(vMap.H_SIZE,vMap.V_SIZE)/1000.f;
}

cSunMoonObj::~cSunMoonObj()
{
	RELEASE(SunTexture);
	RELEASE(MoonTexture);
}


SunMoonAttribute::SunMoonAttribute()
{
	SunName = "Scripts\\resource\\Textures\\sun.tga";
	MoonName = "Scripts\\resource\\Textures\\moon.tga";
	sunSize = 100.f;
	moonSize = 100.f;
}

void SunMoonAttribute::serialize(Archive& ar)
{
	static ResourceSelector::Options textureOptions("*.tga", "Resource\\TerrainData\\Textures");
	ar.serialize(ResourceSelector(SunName, textureOptions), "sunTextureName", "Текстура Cолнца");
	ar.serialize(ResourceSelector(MoonName, textureOptions), "moonTextureName", "Текстура Луны");
	ar.serialize(sunSize, "sunSize", "Размер солнца");
	ar.serialize(moonSize, "sunSize", "Размер луны");
}

void cSunMoonObj::serialize(Archive& ar)
{
	attribute_.serialize(ar);
	if(ar.isInput())
		SetTextures();
}

void cSunMoonObj::setAttribute(const SunMoonAttribute& attribute)
{
    attribute_ = attribute;
	SetTextures();
}

void cSunMoonObj::Draw(Camera* camera)
{
	cTexture* texture = isDay?SunTexture:MoonTexture;
	if (!texture)
		return;
	gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE,FALSE);
	const MatXf& mat=camera->GetMatrix();
	cVertexBuffer<sVertexXYZDT1>* pBuf  = gb_RenderDevice->GetBufferXYZDT1();
	gb_RenderDevice3D->SetNoMaterial(isDay?ALPHA_ADDBLENDALPHA:ALPHA_BLEND,MatXf::ID,0,texture);
	sVertexXYZDT1* v = pBuf->Lock(4);

	Vect2f rot((isDay?attribute_.sunSize:attribute_.moonSize)*scale,0);
	Vect3f& pos = position_.trans();
	Vect3f sx,sy;
	Color4c color(255,255,255,255);
	mat.invXformVect(Vect3f(rot.x,-rot.y,0),sx);
	mat.invXformVect(Vect3f(rot.y,rot.x,0),sy);
	sy = -sy;
	v[0].pos.x=pos.x-sx.x-sy.x;	v[0].pos.y=pos.y-sx.y-sy.y;	v[0].pos.z=pos.z-sx.z-sy.z;
	v[0].diffuse=color;
	v[0].GetTexel().x = 0; v[0].GetTexel().y = 0;	//	(0,0);

	v[1].pos.x=pos.x-sx.x+sy.x;	v[1].pos.y=pos.y-sx.y+sy.y;	v[1].pos.z=pos.z-sx.z+sy.z;
	v[1].diffuse=color;
	v[1].GetTexel().x = 0; v[1].GetTexel().y = 1;//	(0,1);

	v[2].pos.x=pos.x+sx.x-sy.x; v[2].pos.y=pos.y+sx.y-sy.y; v[2].pos.z=pos.z+sx.z-sy.z; 
	v[2].diffuse=color;
	v[2].GetTexel().x = 1; v[2].GetTexel().y = 0;	//  (1,0);

	v[3].pos.x=pos.x+sx.x+sy.x;	v[3].pos.y=pos.y+sx.y+sy.y;	v[3].pos.z=pos.z+sx.z+sy.z;
	v[3].diffuse=color;
	v[3].GetTexel().x = 1;v[3].GetTexel().y = 1;//  (1,1);

	pBuf->Unlock(4);
	pBuf->DrawPrimitive(PT_TRIANGLESTRIP, 2);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE,TRUE);
}

void cSunMoonObj::SetTextures()
{
	RELEASE(SunTexture);
	RELEASE(MoonTexture);
	SunTexture = GetTexLibrary()->GetElement3D(attribute_.SunName.c_str());
	MoonTexture = GetTexLibrary()->GetElement3D(attribute_.MoonName.c_str());
}

cSkyCamera::cSkyCamera(cScene* scene)
:Camera(scene)
{
	enable_hdr_alpha=false;
}

void cSkyCamera::DrawScene()
{
	gb_RenderDevice->setCamera(this);

	DWORD old_colorwrite=gb_RenderDevice3D->GetRenderState(D3DRS_COLORWRITEENABLE);

	sunMoonObj->Draw(this);
	for(vector<OneObject>::iterator it=objects.begin();it!=objects.end();it++)
	{
		OneObject& p=*it;
		DWORD write=D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED;
		if(p.write_alpha && enable_hdr_alpha)
			write|=D3DCOLORWRITEENABLE_ALPHA;
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,write);
		p.obj->DrawAll(this);
	}

	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_colorwrite);
	objects.clear();
}

void cSkyCamera::AddObject(cObject3dx* object, bool write_alpha)
{
	OneObject p;
	p.obj=object;
	p.write_alpha=write_alpha;
	objects.push_back(p);
}

cRenderSky::cRenderSky(cSkyObj* pSkyObj_)
{
	pSkyObj = pSkyObj_;
}

//Отрисовка отражения с помощью камеры с указанным индексом
void cRenderSky::DrawOne(int i)
{
	pSkyObj->DrawSky(camera[i],true);
}

void SkyName::serialize(Archive& ar)
{
	static ModelSelector::Options    skyOptions("*.3dx", "Resource\\TerrainData\\Sky", "Will select location of 3DX model");
	ar.serialize(ModelSelector(Name, skyOptions), "name", "&Модель");
	ar.serialize(type,"SkyElemntType","Тип элемента");
}
//cSkyObj отвечает за загрузку и отрисовку неба

//Конструктор cSkyObj, в качестве параметров передается глобальная сцена и cEnvironmentTime для получения цвета тумана
cSkyObj::cSkyObj(cScene* pScene_, EnvironmentTime* pEnviromentTime)
:pWorldScene(pScene_)
{
	pFogCircle=0;
	pSkyScene=gb_VisGeneric->CreateScene();
	pNormalCamera=new cSkyCamera(pSkyScene);

	sky_elements_count = 0;
	time = pEnviromentTime;
	sunPosition.set(0,0,0);
	reflect_fone_color.set(0,0,0);
	SetSkyModel();
}

cSkyObj::~cSkyObj()
{
	delete pFogCircle;
	for (int index = 0; index < sky_elements.size(); index++)
	{
		RELEASE(sky_elements[index].object);
	}
	//RELEASE(sun.object);
	RELEASE(pNormalCamera);
	RELEASE(pSkyScene);
}

SkyObjAttribute::SkyObjAttribute() // CONVERSION
{
	sky_elements_names.push_back(SkyName());
	sky_elements_names.back().Name = ".\\Resource\\TerrainData\\Sky\\Sky_Clouds_Day_Default.3DX"; 
}

void SkyObjAttribute::serialize(Archive& ar)
{
	ar.serialize(sky_elements_names, "sky_elements", "Элементы неба");
}

void cSkyObj::serialize(Archive& ar)
{
	sunMoonObj.serialize(ar);

	attribute_.serialize(ar);
	
	if(ar.isInput())
		SetSkyModel();
}

void cSkyObj::setAttribute(const SkyObjAttribute& attribute)
{
	attribute_ = attribute;
	SetSkyModel();
}

void cSkyObj::setSunMoonAttribute(const SunMoonAttribute& attribute)
{
	sunMoonObj.setAttribute(attribute);
}

void cSkyObj::SetDay(bool is_day)
{
	cur_is_day = is_day;
	sunMoonObj.SetDay(is_day);
}

//Загрузка моделей неба
void cSkyObj::SetSkyModel()
{
	Se3f pos=Se3f::ID;
	pos.trans().set(vMap.H_SIZE/2,vMap.V_SIZE/2,0);

	if (sky_elements_count != 0)
	{
		for (int index = 0; index < sky_elements_count; index++)
		{
			RELEASE(sky_elements[index].object);
		}
	}

	sky_elements_count = attribute_.sky_elements_names.size();

	sky_elements.clear();
	//sky_elements.resize(sky_elements_count);
	for (int index = 0; index < sky_elements_count; index++)
	{
		if (attribute_.sky_elements_names[index].Name.size() != 0)
		{
			SkyElement sky_element;
			sky_element.object=0;
			sky_element.object=pSkyScene->CreateObject3dx(attribute_.sky_elements_names[index].Name.c_str());
			if(sky_element.object==0)
				continue;
			sky_element.object->SetPosition(Se3f::ID);
			sky_element.object->setAttribute(ATTRUNKOBJ_NOLIGHT);
			sky_element.object->clearAttribute(ATTRCAMERA_SHADOWMAP);
			sky_element.object->SetPosition(pos);
			sky_element.object->SetScale(CalcNormalScale());
			sky_element.anim_group = sky_element.object->GetAnimationGroup("main");
			sky_element.time = sky_element.object->GetChain(0)->time;
			sky_element.phase_cloud = 0;
			sky_element.type = attribute_.sky_elements_names[index].type;
			sky_elements.push_back(sky_element);
		}
	}
	sky_elements_count = sky_elements.size();
}

float cSkyObj::CalcNormalScale()
{
	static float d = 13000;//6850;
	float k_scale = (max(vMap.H_SIZE,vMap.V_SIZE)*2.5f)/d;
	return k_scale;
}
void cSkyObj::DrawSun(Camera* pGlobalCamera)
{
}

void cSkyObj::DrawSky(Camera* pGlobalCamera,bool hdr_alpha)
{
	pGlobalCamera->SetCopy(pNormalCamera);
	pNormalCamera->setAttribute(ATTRCAMERA_NOZWRITE);
	pNormalCamera->clearAttribute(ATTRCAMERA_NOCLEARTARGET);
	pNormalCamera->SetFoneColor(pGlobalCamera->GetFoneColor());

	pNormalCamera->SetHDRAlpha(hdr_alpha);

	if(pGlobalCamera->GetRenderTarget())
	{
		pNormalCamera->SetRenderTarget(pGlobalCamera->GetRenderTarget(),pGlobalCamera->GetZBuffer());
	}else
	{
		pNormalCamera->SetRenderTarget(pGlobalCamera->GetRenderSurface(),pGlobalCamera->GetZBuffer());
	}

	Vect2f zPlane(1e3f,1e5f);
	pNormalCamera->SetFrustum(0,0,0,&zPlane);

	cD3DRender* rd=gb_RenderDevice3D;
	DWORD old_fogenable=rd->GetRenderState(D3DRS_FOGENABLE);
	rd->SetRenderState(D3DRS_FOGENABLE,FALSE);

	vector<SkyElement>::iterator it;
	pNormalCamera->SetSunMoonObj(&sunMoonObj);
	FOR_EACH(sky_elements,it)
	{
		if(it->anim_group>=0)
			pNormalCamera->AddObject(it->object,pGlobalCamera->getAttribute(ATTRCAMERA_REFLECTION));
	}

	pSkyScene->Draw(pNormalCamera);
	//DrawSun(pGlobalCamera);
	

	rd->SetRenderState(D3DRS_FOGENABLE,old_fogenable);

	if(!hdr_alpha)///Непонятно - правильно ли?
	{
		rd->SetDrawTransform(pNormalCamera);
		DWORD old_fogenable=rd->GetRenderState(D3DRS_FOGENABLE);
		DWORD old_zwrite=rd->GetRenderState(D3DRS_ZWRITEENABLE);
		rd->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		rd->SetRenderState(D3DRS_FOGENABLE,FALSE);
		if(pFogCircle)
			pFogCircle->Draw(pNormalCamera);
		rd->SetRenderState(D3DRS_FOGENABLE,old_fogenable);
		rd->SetRenderState(D3DRS_ZWRITEENABLE, old_zwrite);
	}

	pNormalCamera->SetRenderTarget((IDirect3DSurface9*)0,0);
}

//Отрисовка неба
void cSkyObj::DrawSkyAndAnimate(Camera* pGlobalCamera)
{
	float dt=pWorldScene->GetDeltaTime();

	float dth = time->GetTime();
	float dayPhase = 0;
	float nightPhase = 1;
	if (dth>3&&dth<=9)
	{
		dayPhase = (dth-3)/6.f;
		nightPhase = 1-(dth-3)/6.f;
	}
	else
	if (dth>9&&dth<=15)
	{
		dayPhase = 1;
		nightPhase = 0;
	}
	else
	if (dth>15&&dth<=21)
	{
		dayPhase = 1-(dth-15)/6.f;
		nightPhase = (dth-15)/6.f;
	}

	vector<SkyElement>::iterator it;
	FOR_EACH(sky_elements,it)
	{
		if(it->anim_group>=0)
		{
			it->phase_cloud+=dt*0.001/it->time;

			if (it->phase_cloud>=1)
				it->phase_cloud-=1;
			it->object->SetAnimationGroupPhase(it->anim_group,it->phase_cloud);
			if (it->type == SKY_ELEMENT_DAY)
				it->object->SetOpacity(dayPhase);
			else
			if (it->type == SKY_ELEMENT_NIGHT)
				it->object->SetOpacity(nightPhase);
		}
	}

	DrawSky(pGlobalCamera,false);
	Camera* pReflection=pGlobalCamera->scene()->reflectionCamera();
	if(pReflection)
	{
		pReflection->setAttribute(ATTRCAMERA_NOCLEARTARGET);
		//pReflection->SetFoneColor(pGlobalCamera->GetFoneColor());
		pReflection->SetFoneColor(reflect_fone_color);
//		gb_RenderDevice->setCamera(pReflection);
		
		DrawSky(pReflection,true);
	}
}

void cSkyObj::AddSkyModel(const char* sky_model_name)
{
	SkyName name;
	name.Name=sky_model_name;
	attribute_.sky_elements_names.push_back(name);
}

/*
vb
02
13
ib
103
302

vb
03
14
25
ib
104
403
215
514
*/

#define FOG_CENTER
//Инициализация тумана
cFogCircleEX::cFogCircleEX(EnvironmentTime* time_)
{
	cD3DRender* rd=gb_RenderDevice3D;
	time = time_;

	//Две полосы одна - одна из прозрачного в непрозрачное, другая полностью непрозрачная.
	//И снизу шатёр из треугольников.

	size_vb = (hord_count+1)*3;
	size_ib = hord_count*4;
#ifdef FOG_CENTER
	size_vb++;
	size_ib+=hord_count;
#endif

	rd->CreateVertexBuffer(cborder_vb, size_vb,VType::declaration);
	rd->CreateIndexBuffer(cborder_ib, size_ib);

	SetHeight(2000);
}
cFogCircleEX::~cFogCircleEX()
{
}

void cFogCircleEX::SetHeight(int height_)
{
	height=height_;

	float radius = max(vMap.H_SIZE,vMap.V_SIZE)*2.5f*0.98f;
	float z0 = -height;
	float z1 = 0;
	float z2 = height;
	float da = (M_PI*2)/hord_count;
	Mat3f rot(da,Z_AXIS);

	cD3DRender* rd=gb_RenderDevice3D;
	Color4c color1(255, 255, 255, 255);
	Color4c color2(255, 255, 255, 0);
	VType* beg_vx = (VType*)rd->LockVertexBuffer(cborder_vb);
	sPolygon* beg_pt=rd->LockIndexBuffer(cborder_ib);

	VType* v = beg_vx;
	sPolygon* pt = beg_pt;
	
	Vect3f pos(radius, 0, 0);
	Vect3f center(vMap.H_SIZE/2, vMap.V_SIZE/2, 0);
	for(int i=0;i<3*hord_count; i+=3)
	{
		Vect3f p = pos+center;
		v->pos.set(p.x, p.y, z0);  	v->diffuse = color1; v++;
		v->pos.set(p.x, p.y, z1);  	v->diffuse = color1; v++;
		v->pos.set(p.x, p.y, z2);  	v->diffuse = color2; v++;
		pos = rot*pos;

		pt->set(i+1, i+0, i+4);	pt++;
		pt->set(i+4, i+0, i+3);	pt++;

		pt->set(i+2, i+1, i+5);	pt++;
		pt->set(i+5, i+1, i+4);	pt++;
	}
	Vect3f p = Vect3f(radius, 0,0)+center;
	v->pos.set(p.x, p.y, z0);  	v->diffuse = color1; v++;
	v->pos.set(p.x, p.y, z1);  	v->diffuse = color1; v++;
	v->pos.set(p.x, p.y, z2);  	v->diffuse = color2; v++;
#ifdef FOG_CENTER
	int last_point=v-beg_vx;
	v->pos.set(vMap.H_SIZE/2, vMap.V_SIZE/2, z0);  	v->diffuse = Color4c(255,255,255,255); v++;
	for(int i=0;i<3*hord_count; i+=3)
	{
		pt->set(last_point, i+3, i+0);	pt++;
	}
#endif

	xassert(v-beg_vx == size_vb);
	xassert(pt-beg_pt == size_ib);

	rd->UnlockVertexBuffer(cborder_vb);
	rd->UnlockIndexBuffer(cborder_ib);
}

//Отрисовка тумана
void cFogCircleEX::Draw(Camera* camera)
{
	if (cborder_vb.IsInit())
	{
		cD3DRender* rd=gb_RenderDevice3D;
		rd->SetNoMaterial(ALPHA_BLEND, MatXf::ID, 0, 0);

		DWORD old_colorarg1=rd->GetTextureStageState(0,D3DTSS_COLORARG1);
		rd->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TFACTOR);
		rd->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
		rd->SetRenderState(D3DRS_TEXTUREFACTOR,time->GetCurFogColor().RGBA());
		rd->DrawIndexedPrimitive(cborder_vb, 0, size_vb, cborder_ib, 0, size_ib);

		rd->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
		rd->SetTextureStageState( 0, D3DTSS_COLORARG1, old_colorarg1);
	}
}

/////////////////////////////////cEnvironmentTime/////////////////////////////////////////////
EnvironmentTimeColors::EnvironmentTimeColors()
{
	latitude_angle=180.0f/5;
	slant_angle=0;
	shadow_intensity = 0.5f;
	shadowDecay = 0.7f;

	KeysColor fone_color_day;
	KeysColor fone_color_night;
	fone_color_day.GetOrCreateKey(0)->Val().set(0.207843f, 0.333333f, 0.4f, 1.0f);
	fone_color_day.GetOrCreateKey(0.286519f)->Val().set(0.611765f, 0.756863f, 0.835294f, 1.0f);
	fone_color_day.GetOrCreateKey(0.5f)->Val().set(0.803923f, 0.952941f, 1.0f, 1.0f);
	fone_color_day.GetOrCreateKey(0.870423f)->Val().set(0.486275f, 0.627451f, 0.72549f, 1);
	fone_color_day.GetOrCreateKey(1.0f)->Val().set(0.207843f, 0.333333f, 0.4f, 1);

	fone_color_night.GetOrCreateKey(0.0f)->Val().set(0.207843f, 0.333333f, 0.4f, 1.0f);
	fone_color_night.GetOrCreateKey(0.189135f)->Val().set(0.0f, 0.0f, 0.0f, 1.0f);
	fone_color_night.GetOrCreateKey(0.810865f)->Val().set(0.0f,  0.0f, 0.0f, 1.0f);
	fone_color_night.GetOrCreateKey(1.0f)->Val().set(0.207843f, 0.333333f, 0.4f, 1.0f);

	mergeColor(fone_color,fone_color_day,fone_color_night);
	

	KeysColor reflect_sky_color_day;
	KeysColor reflect_sky_color_night;

	reflect_sky_color_day.GetOrCreateKey(0.0f)->Val().set(0.168627f, 0.196078f, 0.223529f, 1.f);
	reflect_sky_color_day.GetOrCreateKey(0.5f)->Val().set(0.490196f, 0.560784f, 0.67451f, 1.f);
	reflect_sky_color_day.GetOrCreateKey(0.995976f)->Val().set(0.168627f, 0.196078f, 0.223529f, 1.f);
	reflect_sky_color_day.GetOrCreateKey(1.0f)->Val().set(0.2f, 0.2f, 0.2f, 1.f);

	reflect_sky_color_night.GetOrCreateKey(0.0f)->Val()=Color4f(0,0,0,1);

	mergeColor(reflect_sky_color,reflect_sky_color_day,reflect_sky_color_night);


	KeysColor sun_color_day;
	KeysColor sun_color_night;

	sun_color_day.GetOrCreateKey(0.0f)->Val().set(0.517647f, 0.462745f, 0.4f, 1.0f);
	sun_color_day.GetOrCreateKey(0.286519f)->Val().set(1.f, 1.f, 1.f, 1.f);
	sun_color_day.GetOrCreateKey(0.5f)->Val().set(1.f, 1.f, 1.f, 1.f);
	sun_color_day.GetOrCreateKey(0.870423f)->Val().set(1.f, 1.f, 1.f, 1.f);
	sun_color_day.GetOrCreateKey(1.0f)->Val().set(0.517647f, 0.462745f, 0.4f, 1.f);

	sun_color_night.GetOrCreateKey(0)->Val().set(0.517647f, 0.462745f, 0.4f, 1.f);
	sun_color_night.GetOrCreateKey(0.28169f)->Val().set(0.227451f, 0.219608f, 0.282353f, 1.f);
	sun_color_night.GetOrCreateKey(0.77666f)->Val().set(0.227451f, 0.219608f, 0.27451f, 1.f);
	sun_color_night.GetOrCreateKey(1)->Val().set(0.517647f, 0.462745f, 0.4f, 1.f);

	mergeColor(sun_color,sun_color_day,sun_color_night);


	Color4f base_fog_color(Color4c(159, 157, 142, 255));
	fog_color.GetOrCreateKey(0.0f)->Val() = base_fog_color;
	fog_color.GetOrCreateKey(1.0f)->Val() = base_fog_color;

	shadow_color.GetOrCreateKey(0.0f)->Val().set(0.5f, 0.5f, 0.5f, 1.0f);
	shadow_color.GetOrCreateKey(1.0f)->Val().set(0.5f, 0.5f, 0.5f, 1.0f);

	circle_shadow_color.GetOrCreateKey(0.0f)->Val().set(0.5f, 0.5f, 0.5f, 1.0f);
	circle_shadow_color.GetOrCreateKey(1.0f)->Val().set(0.5f, 0.5f, 0.5f, 1.0f);
}

void EnvironmentTimeColors::mergeColor(KeysColor& out/*0.00-24.00*/,const KeysColor& day/*6.00-18.00*/,const KeysColor& night/*18.00-6.00*/)
{
	out.GetOrCreateKey(0)->Val()=night.Get(0);

	{//0.00-6.00
		for(int i=0;i<night.size();i++)
		{
			const KeyColor& k=night[i];
			if(k.time>0.5f)
			{
				float t=(k.time-0.5f)*0.5f; //(0.5..1)->(0..0.25f)
				out.GetOrCreateKey(t)->Val()=k.Val();
			}
		}
	}

	{//6.00-18.00
		for(int i=0;i<day.size();i++)
		{
			const KeyColor& k=day[i];
			float t=k.time*0.5f+0.25f; //(0..1)->(0.25..0.75f)
			out.GetOrCreateKey(t)->Val()=k.Val();
		}
	}

	{//18.00-0.00
		for(int i=0;i<night.size();i++)
		{
			const KeyColor& k=night[i];
			if(k.time<0.5f)
			{
				float t=k.time*0.5f+0.75f; //(0..0.5)->(0.75..1.0)
				out.GetOrCreateKey(t)->Val()=k.Val();
			}
		}
	}

	out.GetOrCreateKey(1)->Val()=night.Get(0);

	float old_time=out[0].time;
	for(int i=1;i<out.size();i++)
	{
		float time=out[i].time;
		xassert(time>old_time);
		xassert(time>=0 && time<=1);
	}

}

void EnvironmentTimeColors::serialize(Archive& ar)
{
	ar.serialize(static_cast<SkyGradient&>(fone_color),"fone_color","Цвет неба");
	ar.serialize(static_cast<SkyGradient&>(reflect_sky_color),"reflect_sky_color","Цвет отраженного неба в воде");
	ar.serialize(static_cast<SkyGradient&>(sun_color),"sun_color","Цвет солнца");
	ar.serialize(static_cast<SkyGradient&>(fog_color),"fog_color","Цвет тумана");
	ar.serialize(static_cast<SkyGradient&>(shadow_color),"shadow_color","Цвет теней (!!! нормальный серый около 0.5 )");
	ar.serialize(static_cast<SkyAlphaGradient&>(circle_shadow_color),"circle_shadow_color","Цвет теней кружками");

	ar.serialize(RangedWrapperf(shadow_intensity, 0.0f, 1.0f), "shadow_intensity", "Интенсивность теней");
	ar.serialize(RangedWrapperf(shadowDecay, 0.0f, 1.0f), "shadowDecay", "Ослабление теней с наклоном солнца");
	ar.serialize(shadowing, "shadowing", "Освещение поверхности");
	ar.serialize(objectShadowing, "objectShadowing", "Освещение объектов");
	ar.serialize(RangedWrapperf(latitude_angle, 0.0f, 70.0f), "latitude_angle", "Широта местности (0-экватор, 90-полюс)");
	ar.serialize(RangedWrapperf(slant_angle, -180.0f, 180.0f), "slant_angle", "Поворот солнца (-180..+180)");
}

EnvironmentTime::EnvironmentTime(cScene* pScene_)
:pScene(pScene_)
{
	skyObj_ = new cSkyObj(pScene_, this);
	pCubeRender=new cRenderSky(skyObj_);
	cur_sun_color.set(255,255,255);
	cur_fone_color.set(255,255,255);
	current_fog_color.set(255,255,255);
	day_time=14;
	time_angle=0;

	phase_cloud=0;
	iag_clouds=0;
	sun_radius=8000;

	dayTimeScale_ = 500.f;
	nightTimeScale_ = 1000.f;

	cur_reflect_sky_color.set(255,255,255,0);

	enableChangeReflectSkyColor = true;

	Vect3f center_pos(vMap.H_SIZE/2,vMap.V_SIZE/2,0);
	pCubeRender->Init(256,center_pos);
	is_day=true;
	prev_is_day=true;

	Se3f pos=Se3f::ID;
	pos.trans()=center_pos;
	sun_radius=7400*CalcNormalScale();

	pScene->SetSkyCubemap(GetCubeMap());
	SetTime(GetTime());
}

EnvironmentTime::~EnvironmentTime()
{
	if(pScene)
		pScene->SetSkyCubemap(0);
	delete skyObj_;
	delete pCubeRender;
}

float EnvironmentTime::CalcNormalScale()
{
	static float d = 6850;
	float k_scale = (max(vMap.H_SIZE,vMap.V_SIZE)*2.5f)/d;
	return k_scale;
}

void EnvironmentTime::Draw()
{
	pCubeRender->Animate(0);
	pCubeRender->Draw();
}

void EnvironmentTime::Save()
{
	pCubeRender->Save("cube.dds");
}

const Vect3f& EnvironmentTime::sunPosition() const
{
	return skyObj_->GetSunPosition().trans();
}

float EnvironmentTime::sunSize() const
{
	return skyObj_->SunSize();
}

cTexture* EnvironmentTime::GetCubeMap()
{
	return pCubeRender->GetCubeMap();
}

void fCommandHideSelfIllumination(XBuffer& stream)
{
	cScene* cur;
	stream.read(cur);
	bool hide;
	stream.read(hide);
	cur->HideSelfIllumination(hide);
}

void EnvironmentTime::setTimeScale(float dayTimeScale, float nightTimeScale)
{
	dayTimeScale_ = dayTimeScale;
	nightTimeScale_ = nightTimeScale;
}

void EnvironmentTime::logicQuant()
{
	float timeScale = isDay() ? dayTimeScale_ : nightTimeScale_;
	SetTime(fmodFast(day_time + logicTimePeriod*timeScale/(3600*1000.f), 24));

	log_var(isDay());
}

void EnvironmentTime::SetTime(float time, bool init)
{
	xassert(time>=0 && time<=24.0f);
	day_time=time;
	time_angle=(time/12.0f-1)*M_PI;

	prev_is_day=is_day;
	is_day=CheckIsDay();
	
	float light_angle=GetLightAngle();

	Mat3f latitude(Vect3f(1,0,0), G2R(latitude_angle));
	Mat3f longitude(Vect3f(0,1,0),light_angle);
	Mat3f slant(Vect3f(0,0,1), G2R(slant_angle));

	Vect3f light_vector = slant*latitude*longitude*Vect3f(0,0,-1);
	Vect3f light_vector_shadow;
	float light_angle_shadow = light_angle;
	if(fabsf(light_angle) > M_PI_4){
		light_angle_shadow = SIGN(light_angle)*(0.44444444f*(fabsf(light_angle) - M_PI_4) + M_PI_4);
		Mat3f longitude_s(Vect3f(0,1,0),light_angle_shadow);
		light_vector_shadow = slant*latitude*longitude_s*Vect3f(0,0,-1);
	}
	else
		light_vector_shadow = light_vector;
	
	MatXf trans(Mat3f::ID, Vect3f(vMap.H_SIZE/2,vMap.V_SIZE/2,0));
	MatXf distance(Mat3f::ID, Vect3f(0,0,sun_radius));
	MatXf mat = trans*MatXf(slant, Vect3f::ZERO)*MatXf(latitude, Vect3f::ZERO)*MatXf(longitude, Vect3f::ZERO)*distance;
	skyObj_->SetSunPosition(mat);

	float factor=time/24;

	cur_sun_color = sun_color.Get(factor);
	cur_sun_color.a = 255;

	cur_fone_color = fone_color.Get(factor);
	cur_fone_color.a = 0;

	if(enableChangeReflectSkyColor)
		cur_reflect_sky_color = reflect_sky_color.Get(factor);
	pCubeRender->SetFoneColor(cur_reflect_sky_color);
	if(skyObj_)
		skyObj_->SetReflectFoneColor(cur_reflect_sky_color);

	current_fog_color = fog_color.Get(factor);

	if(prev_is_day!=is_day || init){
		streamLogicCommand.set(fCommandHideSelfIllumination)<<pScene<<is_day;
		skyObj_->SetDay(is_day);
	}

	if(pScene->GetTileMap()){
		Color4f tileMapColor(cur_sun_color);
		tileMapColor.a = shadowing.ambient(tileMapColor);
		shadowing.scaleDiffuse(tileMapColor);
		pScene->GetTileMap()->SetDiffuse(tileMapColor);
	}

	Color4f sunAmbient(cur_sun_color);
	sunAmbient.r = sunAmbient.g = sunAmbient.b = objectShadowing.ambient(sunAmbient);

	Color4f sunDiffuse(cur_sun_color);
	objectShadowing.scaleDiffuse(sunDiffuse);
	pScene->SetSunColor(sunAmbient, sunDiffuse, Color4f(cur_sun_color));

	pScene->SetSunDirection(light_vector);
	pScene->SetSunShadowDir(light_vector_shadow);

	Color4f shadowColor = shadow_color.Get(factor);
	float shadowFactor = clamp(shadow_intensity*(1 - fabsf(time_angle)*shadowDecay/(M_PI_2)), 0.f, 1.f);
	float minColor = min(shadowColor.r, shadowColor.g, shadowColor.b);
	shadowFactor = (1.f - shadowFactor)/(minColor + 0.01f);
	shadowColor.r = min(shadowColor.r*shadowFactor, 1.0f); 
	shadowColor.g = min(shadowColor.g*shadowFactor, 1.0f); 
	shadowColor.b = min(shadowColor.b*shadowFactor, 1.0f); 
	shadowColor.a = 1;
	pScene->SetShadowIntensity(shadowColor);

	Color4f circle=circle_shadow_color.Get(factor);
	pScene->SetCircleShadowIntensity(circle);
}

bool EnvironmentTime::CheckIsDay()
{
	return fabsf(time_angle) < M_PI_2;
}

float EnvironmentTime::GetLightAngle()
{
	float angle=time_angle;
	if(!isDay())
		angle=-angle;

	if(angle>M_PI_2)
		angle-=M_PI;
	if(angle<-M_PI_2)
		angle+=M_PI;
	return angle;
}

Color4c EnvironmentTime::GetCurFogColor()
{
	return current_fog_color;
};

void EnvironmentTime::SetFogCircle(bool need)
{
	skyObj_->SetFogCircle(need,this);
}

void cSkyObj::SetFogCircle(bool need,EnvironmentTime* pEnviromentTime)
{
	if(need){
		if(!pFogCircle)
			pFogCircle=new cFogCircleEX(pEnviromentTime);
	}
	else if (pFogCircle){
		delete pFogCircle;
		pFogCircle = 0;
	}
}


void EnvironmentTime::setFogHeight(int height)
{
	skyObj_->setFogHeight(height);
}

void cSkyObj::setFogHeight(int height)
{
	if(pFogCircle)
		pFogCircle->SetHeight(height);
}

void EnvironmentTime::serialize(Archive& ar)
{
	if(ar.filter(SERIALIZE_PRESET_DATA)){
		ar.serialize(dayTimeScale_, "dayTimeScale", "Масштаб времени днем");
		ar.serialize(nightTimeScale_, "nightTimeScale", "Масштаб времени ночью");

		__super::serialize(ar);

		ar.serialize(*skyObj_, "Sky", "Небо");
	}

	if(ar.filter(SERIALIZE_WORLD_DATA)){
		ar.serialize(RangedWrapperf(day_time, 0.0f, 24.0f), "dayTime", "Время суток");
		if(ar.isInput())
			SetTime(day_time, true);
	}
}

void EnvironmentTime::DrawEnviroment(Camera* pGlobalCamera)
{
	pGlobalCamera->SetFoneColor(GetCurFoneColor());
	skyObj_->DrawSkyAndAnimate(pGlobalCamera);
}

