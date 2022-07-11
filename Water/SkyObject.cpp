#include "stdafx.h"
#include "Water.h"
#include "SkyObject.h"
#include "..\Render\inc\TerraInterface.inl"
#include "..\Render\inc\IVisD3D.h"
#include "..\Render\src\RenderCubemap.h"
#include "..\Render\src\Gradients.h"
#include "..\Util\Serialization\Serialization.h"
#include "ResourceSelector.h"
#include "RangedWrapper.h"
#include "..\ht\StreamInterpolation.h"

//cRenderSky потомок от cRenderCubemap отвечает за отражения неба в воде.

//Конструктор, в качестве параметра передается указатель на cSkyObj

BEGIN_ENUM_DESCRIPTOR(SkyElementType, "Тип элемента неба")
REGISTER_ENUM(SKY_ELEMENT_DAY, "Дневной")
REGISTER_ENUM(SKY_ELEMENT_NIGHT, "Ночной")
REGISTER_ENUM(SKY_ELEMENT_DAYNIGHT, "Круглосуточный")
END_ENUM_DESCRIPTOR(SkyElementType)


cSunMoonObj::cSunMoonObj()
: cBaseGraphObject(SCENENODE_OBJECT)
{
	SunName;
	MoonName;
	SunTexture = NULL;
	MoonTexture = NULL;
	isDay = true;
	sunSize = 100.f;
	moonSize = 100.f;
	scale = max(vMap.H_SIZE,vMap.V_SIZE)/1000.f;
	SunName = "Scripts\\resource\\Textures\\sun.tga";
	MoonName = "Scripts\\resource\\Textures\\moon.tga";
}

cSunMoonObj::~cSunMoonObj()
{
	RELEASE(SunTexture);
	RELEASE(MoonTexture);
}

void cSunMoonObj::serialize(Archive& ar)
{
	ar.serialize(ResourceSelector(SunName, ResourceSelector::TEXTURE_OPTIONS), "sunTextureName", "Текстура Cолнца");
	ar.serialize(ResourceSelector(MoonName, ResourceSelector::TEXTURE_OPTIONS), "moonTextureName", "Текстура Луны");
	ar.serialize(sunSize, "sunSize", "Размер солнца");
	ar.serialize(moonSize, "sunSize", "Размер луны");
	SetTextures();
}

void cSunMoonObj::Draw(cCamera* camera)
{
	cTexture* texture = isDay?SunTexture:MoonTexture;
	if (!texture)
		return;
	gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE,FALSE);
	MatXf& mat=camera->GetMatrix();
	cVertexBuffer<sVertexXYZDT1>* pBuf  = gb_RenderDevice->GetBufferXYZDT1();
	gb_RenderDevice3D->SetNoMaterial(isDay?ALPHA_ADDBLENDALPHA:ALPHA_BLEND,MatXf::ID,0,texture);
	sVertexXYZDT1* v = pBuf->Lock(4);

	Vect2f rot((isDay?sunSize:moonSize)*scale,0);
	Vect3f& pos = position_.trans();
	Vect3f sx,sy;
	sColor4c color(255,255,255,255);
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
	SunTexture = GetTexLibrary()->GetElement3D(SunName.c_str());
	MoonTexture = GetTexLibrary()->GetElement3D(MoonName.c_str());
}

cSkyCamera::cSkyCamera(cScene* scene)
:cCamera(scene)
{
	enable_hdr_alpha=false;
}

void cSkyCamera::DrawScene()
{
	gb_RenderDevice->SetDrawNode(this);

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
	pSkyObj->DrawSky(pCamera[i],true);
}

//Сериализация строки содержащей путь к объекту неба
void SkyNameOld::serialize(Archive& ar)
{
	ar.serialize(ModelSelector(Name, ModelSelector::SKY_OPTIONS), "sky_model_element", "&Элемент модели неба");
}
void SkyName::serialize(Archive& ar)
{
	ar.serialize(ModelSelector(Name, ModelSelector::SKY_OPTIONS), "name", "&Модель");
	ar.serialize(type,"SkyElemntType","Тип элемента");
}
//cSkyObj отвечает за загрузку и отрисовку неба

//Конструктор cSkyObj, в качестве параметров передается глобальная сцена и cEnvironmentTime для получения цвета тумана
cSkyObj::cSkyObj(cScene* pScene_, cEnvironmentTime* pEnviromentTime)
:pWorldScene(pScene_)
{
	//sun.object = NULL;

//	pFogCircle=new cFogCircleEX(pEnviromentTime);
	pFogCircle=NULL;
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

void cSkyObj::serialize(Archive& ar)
{
	sunMoonObj.serialize(ar);
	
	ar.serialize(sky_elements_names, "sky_elements", "Элементы неба");
	
	if(ar.isInput())
		SetSkyModel();
}

void cSkyObj::SetDay(bool is_day)
{
	cur_is_day = is_day;
	sunMoonObj.SetDay(is_day);
	//sun.object->SetVisibilityGroup0(cur_is_day?"day":"night");
	//for (int index = 0; index < sky_elements_count; index++)
	//	if (sky_elements[index].object->GetVisibilityGroupIndex0(cur_is_day?"day":"night") >-1)
	//		sky_elements[index].object->SetVisibilityGroup0(cur_is_day?"day":"night");
	//	else{
	//		XBuffer buf;
	//		buf < "В элементе неба \"" < sky_elements[index].object->GetFileName() < "\"\n";
	//		buf < "отсутствует группа видимости " < (cur_is_day?"day":"night");
	//		kdWarning("&sky", buf);
	//	}
	
}
//Загрузка моделей неба
void cSkyObj::SetSkyModel()
{
	Se3f pos=Se3f::ID;
	pos.trans().set(vMap.H_SIZE/2,vMap.V_SIZE/2,0);

	//if (sun.object != NULL)
	//	RELEASE(sun.object);

	//if(sun_name.Name.empty())
	//	sun_name.Name = "Scripts\\Resource\\balmer\\sun.3DX";

	//sun.object=NULL;
	//sun.object=pSkyScene->CreateObject3dx(sun_name.Name.c_str());
	//sun.object->SetPosition(Se3f::ID);
	//sun.object->SetAttr(ATTRUNKOBJ_NOLIGHT);
	//sun.object->SetPosition(pos);
	//sun.object->SetScale(CalcNormalScale());
	//sun.anim_group = sun.object->GetAnimationGroup("main");
	//sun.time = sun.object->GetChain(0)->time;
	//sun.phase_cloud = 0;

	if (sky_elements_count != 0)
	{
		for (int index = 0; index < sky_elements_count; index++)
		{
			RELEASE(sky_elements[index].object);
		}
	}

	sky_elements_count = sky_elements_names.size();

	sky_elements.clear();
	//sky_elements.resize(sky_elements_count);
	for (int index = 0; index < sky_elements_count; index++)
	{
		if (sky_elements_names[index].Name.size() != 0)
		{
			SkyElement sky_element;
			sky_element.object=NULL;
			sky_element.object=pSkyScene->CreateObject3dx(sky_elements_names[index].Name.c_str());
			if(sky_element.object==NULL)
				continue;
			sky_element.object->SetPosition(Se3f::ID);
			sky_element.object->SetAttr(ATTRUNKOBJ_NOLIGHT);
			sky_element.object->ClearAttr(ATTRCAMERA_SHADOWMAP);
//			sky_element.object->SetAlwaysVisible();
			sky_element.object->SetPosition(pos);
			sky_element.object->SetScale(CalcNormalScale());
			sky_element.anim_group = sky_element.object->GetAnimationGroup("main");
			sky_element.time = sky_element.object->GetChain(0)->time;
			sky_element.phase_cloud = 0;
			sky_element.type = sky_elements_names[index].type;
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
void cSkyObj::DrawSun(cCamera* pGlobalCamera)
{
}

void cSkyObj::DrawSky(cCamera* pGlobalCamera,bool hdr_alpha)
{
	pGlobalCamera->SetCopy(pNormalCamera);
	pNormalCamera->SetAttr(ATTRCAMERA_NOZWRITE);
	pNormalCamera->ClearAttr(ATTRCAMERA_NOCLEARTARGET);
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
	//pNormalCamera->AddObject(sun.object,true);
	//pNormalCamera->pos = sun.object->GetPosition().trans();
	pNormalCamera->SetSunMoonObj(&sunMoonObj);
	FOR_EACH(sky_elements,it)
	{
		if(it->anim_group>=0)
			pNormalCamera->AddObject(it->object,pGlobalCamera->GetAttribute(ATTRCAMERA_REFLECTION));
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
//		DWORD ols_zenable=rd->GetRenderState(D3DRS_ZENABLE);
//		rd->SetRenderState(D3DRS_ZENABLE,D3DZB_FALSE);
		if(pFogCircle)
			pFogCircle->Draw(pNormalCamera);
//		rd->SetRenderState(D3DRS_ZENABLE,ols_zenable);
		rd->SetRenderState(D3DRS_FOGENABLE,old_fogenable);
		rd->SetRenderState(D3DRS_ZWRITEENABLE, old_zwrite);
	}

	pNormalCamera->SetRenderTarget((IDirect3DSurface9*)NULL,NULL);
}

//Отрисовка неба
void cSkyObj::DrawSkyAndAnimate(cCamera* pGlobalCamera)
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
	//if (cur_is_day)
	//	phase = (dth-6)/12;
	//else
	//	{
	//		if (dth>6)
	//			phase = (dth-12)/12;
	//		else
	//			phase = (dth+6)/12;
	//	}

	//if (sun.phase_cloud>=1)
	//	sun.phase_cloud-=1;
	//sun.object->SetAnimationGroupPhase(sun.anim_group,sun.phase_cloud);

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
	cCamera* pReflection=pGlobalCamera->GetScene()->GetReflectionCamera();
	if(pReflection)
	{
		pReflection->SetAttr(ATTRCAMERA_NOCLEARTARGET);
		//pReflection->SetFoneColor(pGlobalCamera->GetFoneColor());
		pReflection->SetFoneColor(reflect_fone_color);
//		gb_RenderDevice->SetDrawNode(pReflection);
		
		DrawSky(pReflection,true);
	}
}

void cSkyObj::AddSkyModel(const char* sky_model_name)
{
	SkyName name;
	name.Name=sky_model_name;
	sky_elements_names.push_back(name);
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
cFogCircleEX::cFogCircleEX(cEnvironmentTime* time_)
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
	sColor4c color1(255, 255, 255, 255);
	sColor4c color2(255, 255, 255, 0);
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
	v->pos.set(vMap.H_SIZE/2, vMap.V_SIZE/2, z0);  	v->diffuse = sColor4c(255,255,255,255); v++;
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
void cFogCircleEX::Draw(cCamera *pCamera)
{
	if (cborder_vb.IsInit())
	{
		cD3DRender* rd=gb_RenderDevice3D;
//		rd->SetDrawNode(pCamera);
		rd->SetNoMaterial(ALPHA_BLEND, MatXf::ID, 0, NULL);

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

static void MergeColor(CKeyColor& out/*0.00-24.00*/,const CKeyColor& day/*6.00-18.00*/,const CKeyColor& night/*18.00-6.00*/)
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

////////////////////////////cEnvironmentTime
EnvironmentTimeColors::EnvironmentTimeColors()
{
	{
		CKeyColor fone_color_day;
		CKeyColor fone_color_night;
		fone_color_day.GetOrCreateKey(0)->Val().set(0.207843f, 0.333333f, 0.4f, 1.0f);
		fone_color_day.GetOrCreateKey(0.286519f)->Val().set(0.611765f, 0.756863f, 0.835294f, 1.0f);
		fone_color_day.GetOrCreateKey(0.5f)->Val().set(0.803923f, 0.952941f, 1.0f, 1.0f);
		fone_color_day.GetOrCreateKey(0.870423f)->Val().set(0.486275f, 0.627451f, 0.72549f, 1);
		fone_color_day.GetOrCreateKey(1.0f)->Val().set(0.207843f, 0.333333f, 0.4f, 1);


		fone_color_night.GetOrCreateKey(0.0f)->Val().set(0.207843f, 0.333333f, 0.4f, 1.0f);
		fone_color_night.GetOrCreateKey(0.189135f)->Val().set(0.0f, 0.0f, 0.0f, 1.0f);
		fone_color_night.GetOrCreateKey(0.810865f)->Val().set(0.0f,  0.0f, 0.0f, 1.0f);
		fone_color_night.GetOrCreateKey(1.0f)->Val().set(0.207843f, 0.333333f, 0.4f, 1.0f);

		MergeColor(fone_color,fone_color_day,fone_color_night);
	}
	

	{
		CKeyColor reflect_sky_color_day;
		CKeyColor reflect_sky_color_night;

		reflect_sky_color_day.GetOrCreateKey(0.0f)->Val().set(0.168627f, 0.196078f, 0.223529f, 1.f);
		reflect_sky_color_day.GetOrCreateKey(0.5f)->Val().set(0.490196f, 0.560784f, 0.67451f, 1.f);
		reflect_sky_color_day.GetOrCreateKey(0.995976f)->Val().set(0.168627f, 0.196078f, 0.223529f, 1.f);
		reflect_sky_color_day.GetOrCreateKey(1.0f)->Val().set(0.2f, 0.2f, 0.2f, 1.f);

		reflect_sky_color_night.GetOrCreateKey(0.0f)->Val()=sColor4f(0,0,0,1);

		MergeColor(reflect_sky_color,reflect_sky_color_day,reflect_sky_color_night);
	}

	{
		CKeyColor sun_color_day;
		CKeyColor sun_color_night;

		sun_color_day.GetOrCreateKey(0.0f)->Val().set(0.517647f, 0.462745f, 0.4f, 1.0f);
		sun_color_day.GetOrCreateKey(0.286519f)->Val().set(1.f, 1.f, 1.f, 1.f);
		sun_color_day.GetOrCreateKey(0.5f)->Val().set(1.f, 1.f, 1.f, 1.f);
		sun_color_day.GetOrCreateKey(0.870423f)->Val().set(1.f, 1.f, 1.f, 1.f);
		sun_color_day.GetOrCreateKey(1.0f)->Val().set(0.517647f, 0.462745f, 0.4f, 1.f);

		sun_color_night.GetOrCreateKey(0)->Val().set(0.517647f, 0.462745f, 0.4f, 1.f);
		sun_color_night.GetOrCreateKey(0.28169f)->Val().set(0.227451f, 0.219608f, 0.282353f, 1.f);
		sun_color_night.GetOrCreateKey(0.77666f)->Val().set(0.227451f, 0.219608f, 0.27451f, 1.f);
		sun_color_night.GetOrCreateKey(1)->Val().set(0.517647f, 0.462745f, 0.4f, 1.f);

		MergeColor(sun_color,sun_color_day,sun_color_night);
	}

	sColor4f base_fog_color(sColor4c(159, 157, 142, 255));
	fog_color.GetOrCreateKey(0.0f)->Val() = base_fog_color;
	fog_color.GetOrCreateKey(1.0f)->Val() = base_fog_color;

	shadow_color.GetOrCreateKey(0.0f)->Val().set(0.5f, 0.5f, 0.5f, 1.0f);
	shadow_color.GetOrCreateKey(1.0f)->Val().set(0.5f, 0.5f, 0.5f, 1.0f);

	circle_shadow_color.GetOrCreateKey(0.0f)->Val().set(0.5f, 0.5f, 0.5f, 1.0f);
	circle_shadow_color.GetOrCreateKey(1.0f)->Val().set(0.5f, 0.5f, 0.5f, 1.0f);

	global_fone_color=false;
	global_reflect_sky_color=false;
	global_sun_color=false;
	global_fog_color=false;
	global_shadow_color=true;
	global_circle_shadow_color=true;
}

void EnvironmentTimeColors::serialize(Archive& ar)
{
	serializeColors(ar,false);
}

void EnvironmentTimeColors::serializeColors(Archive& ar,bool is_local)
{
	if(is_local) ar.serialize(global_fone_color,"global_fone_color","Глобальный цвет неба");
	ar.serialize(static_cast<SkyGradient&>(fone_color),"fone_color","Цвет неба");
	if(is_local) ar.serialize(global_reflect_sky_color,"global_reflect_sky_color","Глобальный цвет отраженного неба в воде");
	ar.serialize(static_cast<SkyGradient&>(reflect_sky_color),"reflect_sky_color","Цвет отраженного неба в воде");
	if(is_local) ar.serialize(global_sun_color,"global_sun_color","Глобальный цвет солнца");
	ar.serialize(static_cast<SkyGradient&>(sun_color),"sun_color","Цвет солнца");
	if(is_local) ar.serialize(global_fog_color,"global_fog_color","Глобальный цвет тумана");
	ar.serialize(static_cast<SkyGradient&>(fog_color),"fog_color","Цвет тумана");
	if(is_local) ar.serialize(global_shadow_color,"global_shadow_color","Глобальный цвет теней");
	ar.serialize(static_cast<SkyGradient&>(shadow_color),"shadow_color","Цвет теней (!!! нормальный серый около 0.5 )");
	if(is_local) ar.serialize(global_circle_shadow_color,"global_circle_shadow_color","Глобальный цвет теней кружками");
	ar.serialize(static_cast<SkyAlphaGradient&>(circle_shadow_color),"circle_shadow_color","Цвет теней кружками");
}

void EnvironmentTimeColors::ReplaceGlobal(EnvironmentTimeColors& global)
{
	if(global_fone_color)fone_color=global.fone_color;
	if(global_reflect_sky_color)reflect_sky_color=global.reflect_sky_color;
	if(global_sun_color)sun_color=global.sun_color;
	if(global_fog_color)fog_color=global.fog_color;
	if(global_shadow_color)shadow_color=global.shadow_color;
	if(global_circle_shadow_color)circle_shadow_color=global.circle_shadow_color;
}


cEnvironmentTime::cEnvironmentTime(cScene* pScene_)
:pScene(pScene_)
{
	pSkyObj = new cSkyObj(pScene_, this);
//	pScene->AttachObj(pSkyObj);
	pCubeRender=new cRenderSky(pSkyObj);
	//pCubeRender=new cRenderCubemap;
	cur_sun_color.set(255,255,255);
	cur_fone_color.set(255,255,255);
	current_fog_color.set(255,255,255);
	day_time=14;
	time_angle=0;
	slant_angle=0;


	shadow_intensity=0.5f;
	time_shadow_off=1-(4.0f/12);
	speed_shadow_off=10;
	phase_cloud=0;
	iag_clouds=0;
	sun_radius=8000;

	latitude_angle=180.0f/5;
	cur_reflect_sky_color.set(255,255,255,0);

	enableChangeReflectSkyColor = true;
}

void cEnvironmentTime::ExportSunParameters(const char* file_name)
{
	FILE* f=fopen(file_name,"wt");
	if(f==NULL)
	{
		xassert(f && "ExportSunParameters. Cannot open file.");
		return;
	}

	int num=24;
	vector<Vect3f> direction(num);
	vector<sColor4f> ambient(num);
	vector<sColor4f> diffuse(num);
	vector<sColor4f> specular(num);
	for(int time=0;time<num;time++)
	{
		SetTime(time,false);
		direction[time]=pScene->GetSunDirection();
		ambient[time]=pScene->GetSunAmbient();
		diffuse[time]=pScene->GetSunDiffuse();
		specular[time]=pScene->GetSunSpecular();
	}

	fprintf(f,"int size=%i\n",num);
	fprintf(f,"Vect3f direction[]={\n");
	for(int time=0;time<num;time++)
	{
		fprintf(f,"Vect3f(%ff,%ff,%ff),\n",direction[time].x,direction[time].y,direction[time].z);
	}
	fprintf(f,"};\n");

	fprintf(f,"sColor4f ambient[]={\n");
	for(int time=0;time<num;time++)
	{
		fprintf(f,"sColor4f(%ff,%ff,%ff),\n",ambient[time].r,ambient[time].g,ambient[time].b);
	}
	fprintf(f,"};\n");

	fprintf(f,"sColor4f diffuse[]={\n");
	for(int time=0;time<num;time++)
	{
		fprintf(f,"sColor4f(%ff,%ff,%ff),\n",diffuse[time].r,diffuse[time].g,diffuse[time].b);
	}
	fprintf(f,"};\n");

	fprintf(f,"sColor4f specular[]={\n");
	for(int time=0;time<num;time++)
	{
		fprintf(f,"sColor4f(%ff,%ff,%ff),\n",specular[time].r,specular[time].g,specular[time].b);
	}
	fprintf(f,"};\n");

	fclose(f);
}

cEnvironmentTime::~cEnvironmentTime()
{
	if(pScene)
		pScene->SetSkyCubemap(NULL);
	delete pSkyObj;
	delete pCubeRender;
}

void cEnvironmentTime::Init()
{
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

float cEnvironmentTime::CalcNormalScale()
{
	static float d = 6850;
	float k_scale = (max(vMap.H_SIZE,vMap.V_SIZE)*2.5f)/d;
	return k_scale;
}

void cEnvironmentTime::Draw()
{
	pCubeRender->Animate(0);
	pCubeRender->Draw();
}

void cEnvironmentTime::Save()
{
	pCubeRender->Save("cube.dds");
}

const Vect3f& cEnvironmentTime::sunPosition() const
{
	return pSkyObj->GetSunPosition().trans();
}

float cEnvironmentTime::sunSize() const
{
	return pSkyObj->SunSize();
}

cTexture* cEnvironmentTime::GetCubeMap()
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


void cEnvironmentTime::SetTime(float time, bool init)
{
	xassert(time>=0 && time<=24.0f);
	day_time=time;
	time_angle=(time/12.0f-1)*M_PI;
	MatXf mat;
	MatXf latitude;
	MatXf trans;
	latitude.rot().set(Vect3f(1,0,0),latitude_angle*(M_PI/180));
	latitude.trans()=Vect3f::ZERO;
	trans.rot()=Mat3f::ID;
	trans.trans().set(vMap.H_SIZE/2,vMap.V_SIZE/2,0);

	prev_is_day=is_day;
	is_day=CheckIsDay();

	float light_angle=GetLightAngle();

	MatXf longitude;
	longitude.rot().set(Vect3f(0,1,0),light_angle);
	longitude.trans().set(0,0,0);


	MatXf slant;
	slant.rot().set(Vect3f(0,0,1),slant_angle*(M_PI/180));
	slant.trans().set(0,0,0);

	MatXf distance(Mat3f::ID,Vect3f(0,0,sun_radius));
	Vect3f light_vector=slant.rot()*latitude.rot()*longitude.rot()*Vect3f(0,0,-1);

	Vect3f light_vector_shadow;
	float light_angle_shadow = light_angle;
	if (light_angle > 0.78539816f || light_angle < -0.78539816f)
	{
		if (light_angle > 0.78539816f)
			light_angle_shadow = 0.44444444f*(light_angle-0.78539816f)+0.78539816f;
		else
			light_angle_shadow = -(0.44444444f*((-light_angle)-0.78539816f)+0.78539816f);
		MatXf longitude_s;
		longitude_s.rot().set(Vect3f(0,1,0),light_angle_shadow);
		longitude_s.trans().set(0,0,0);
		light_vector_shadow=slant.rot()*latitude.rot()*longitude_s.rot()*Vect3f(0,0,-1);
	}else
	{
		light_vector_shadow = light_vector;
	}
	
	mat=trans*slant*latitude*longitude*distance;
	//pCubeSun->SetPosition(mat);
	pSkyObj->SetSunPosition(mat);

	float factor=time/24;

	sColor4c sc=sun_color.Get(factor);
	sc.a=255;
	cur_sun_color=sc;
	sColor4f ambient(sc),diffuse(0,0,0,1),specular(0,0,0,1);

	sColor4c reflect_sky_c;
	cur_fone_color=fone_color.Get(factor);
	if(enableChangeReflectSkyColor)
		cur_reflect_sky_color=reflect_sky_c= reflect_sky_color.Get(factor);
	else
		reflect_sky_c = cur_reflect_sky_color;
	current_fog_color = fog_color.Get(factor);

	cur_fone_color.a=0;
	pCubeRender->SetFoneColor(reflect_sky_c);
	if(pSkyObj)
		pSkyObj->SetReflectFoneColor(reflect_sky_c);

	if(prev_is_day!=is_day || init)
	{
		//pScene->HideSelfIllumination(is_day);
		streamLogicCommand.set(fCommandHideSelfIllumination)<<pScene<<is_day;
		pSkyObj->SetDay(is_day);
	}

	float p2=M_PI/2;
	
	float shadow_factor;
//	if (is_day)
		shadow_factor = (fabsf(light_angle_shadow)-p2*time_shadow_off)*speed_shadow_off;
//	else
//		shadow_factor=(fabsf(time_angle)-p2*time_shadow_off)*speed_shadow_off;
	shadow_factor=clamp(shadow_factor,0.0f,1.0f);
	sColor4f sun_color_c(GetCurSunColor());

	sColor4f tilemap_color=sun_color_c;

	float sum_color=(tilemap_color.r+tilemap_color.g+tilemap_color.b)/3.0f;
 	tilemap_color.a=min(sum_color*shadowing.user_ambient_factor,shadowing.user_ambient_maximal);

	float tilema_diffuse=shadowing.user_diffuse_factor;
	tilemap_color.r*=tilema_diffuse;
	tilemap_color.g*=tilema_diffuse;
	tilemap_color.b*=tilema_diffuse;
	pScene->GetTileMap()->SetDiffuse(tilemap_color);

	sColor4f ambient_color=sun_color_c;
	ambient_color.r=
	ambient_color.g=
	ambient_color.b=tilemap_color.a*2;

	sColor4f specular_color=sun_color_c;
	sun_color_c.r=tilemap_color.r*2;
	sun_color_c.g=tilemap_color.g*2;
	sun_color_c.b=tilemap_color.b*2;

	pScene->SetSun(light_vector,ambient_color,sun_color_c,specular_color);
	pScene->SetSunShadowDir(light_vector_shadow);
	shadow_factor=shadow_factor*shadow_intensity+(1-shadow_intensity);


	float time_star_on=1;//-(4.0f/12);
	float star_factor=(fabsf(time_angle)-p2*time_star_on)*speed_shadow_off;
	star_factor=clamp(star_factor,0.0f,1.0f);
	sColor4f sky_star_color(1,1,1,star_factor);
	
	{
		sColor4f shadow_color_c=shadow_color.Get(factor);
		shadow_color_c.r=min(shadow_color_c.r*2*shadow_factor,1.0f);
		shadow_color_c.g=min(shadow_color_c.g*2*shadow_factor,1.0f);
		shadow_color_c.b=min(shadow_color_c.b*2*shadow_factor,1.0f);
		shadow_color_c.a=1;
		pScene->SetShadowIntensity(shadow_color_c);
	}

	{
		sColor4f circle=circle_shadow_color.Get(factor);
		//circle.r=255-circle.r;
		//circle.g=255-circle.g;
		//circle.b=255-circle.b;
		//circle.r=1-(1-circle.r)*circle.a;
		//circle.g=1-(1-circle.g)*circle.a;
		//circle.b=1-(1-circle.b)*circle.a;
		//circle.a=1;
		pScene->SetCircleShadowIntensity(circle);
	}
}

bool cEnvironmentTime::CheckIsDay()
{
	bool is=true;
	float angle=time_angle;
	if(angle>M_PI/2)
		is=false;
	if(angle<-M_PI/2)
		is=false;
	return is;
}

float cEnvironmentTime::GetLightAngle()
{
	float angle=time_angle;
	if(!IsDay())
		angle=-angle;

	if(angle>M_PI/2)
		angle-=M_PI;
	if(angle<-M_PI/2)
		angle+=M_PI;
	return angle;
}

sColor4c cEnvironmentTime::GetCurFogColor()
{
	return current_fog_color;
};

void cEnvironmentTime::serializeColors (Archive& ar)
{
	__super::serializeColors(ar,true);

	if(ar.isInput())
		SetTime(this->day_time);
}

void cEnvironmentTime::serializeParameters (Archive& ar)
{
	pSkyObj->serialize(ar);
	ar.serialize(RangedWrapperf(shadow_intensity, 0.0f, 1.0f), "shadow_intensity", "Интенсивность теней");
	ar.serialize(shadowing, "shadowing", "Самозатенение поверхности");
	ar.serialize(RangedWrapperf(latitude_angle, 0.0f, 70.0f), "latitude_angle", "Широта местности (0-экватор, 90-полюс)");
	ar.serialize(RangedWrapperf(slant_angle, -180.0f, 180.0f), "slant_angle", "Поворот солнца (-180..+180)");
	

	float dayTime=GetTime();
	ar.serialize(RangedWrapperf(dayTime, 0.0f, 24.0f), "dayTime", "Время суток");
	if(ar.isInput())
		SetTime(dayTime, true);
}
void cEnvironmentTime::SetFogCircle(bool need)
{
	pSkyObj->SetFogCircle(need,this);
}

void cSkyObj::SetFogCircle(bool need,cEnvironmentTime* pEnviromentTime)
{
	if (need)
	{
		if (!pFogCircle)
			pFogCircle=new cFogCircleEX(pEnviromentTime);
	}else if (pFogCircle)
	{
		delete pFogCircle;
		pFogCircle = NULL;
	}
}


void cEnvironmentTime::setFogHeight(int height)
{
	pSkyObj->setFogHeight(height);
}

void cSkyObj::setFogHeight(int height)
{
	if(pFogCircle)
		pFogCircle->SetHeight(height);
}

void cEnvironmentTime::serialize(Archive& ar)
{
	serializeParameters (ar);
	serializeColors (ar);
}

void cEnvironmentTime::DrawEnviroment(cCamera* pGlobalCamera)
{
	pGlobalCamera->SetFoneColor(GetCurFoneColor());
	pSkyObj->DrawSkyAndAnimate(pGlobalCamera);
}

