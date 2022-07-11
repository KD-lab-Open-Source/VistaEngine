#include "StdAfx.h"
#include "CircleManager.h"
#include "terra.h"
#include "EditArchive.h"
#include "ResourceSelector.h"
#include "..\Render\inc\IVisD3D.h"
#include "..\Water\Water.h"
#include "..\Environment\Environment.h"

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(CircleColor, CircleType, "Тип окружности")
REGISTER_ENUM_ENCLOSED(CircleColor, CIRCLE_SOLID, "Непрерывная линия")
REGISTER_ENUM_ENCLOSED(CircleColor, CIRCLE_CHAIN_LINE, "Штрих пунктир")
REGISTER_ENUM_ENCLOSED(CircleColor, CIRCLE_TRIANGLE, "Треугольники")
REGISTER_ENUM_ENCLOSED(CircleColor, CIRCLE_CROSS, "Перекрестье")
END_ENUM_DESCRIPTOR_ENCLOSED(CircleColor, CircleType)

BEGIN_ENUM_DESCRIPTOR(CIRCLE_MANAGER_DRAW_ORDER, "CIRCLE_MANAGER_DRAW_ORDER")
REGISTER_ENUM(CIRCLE_MANAGER_DRAW_AFTER_GRASS_NOZ, "Выше травы без Z")
REGISTER_ENUM(CIRCLE_MANAGER_DRAW_BEFORE_GRASS_NOZ, "Ниже травы без Z")
REGISTER_ENUM(CIRCLE_MANAGER_DRAW_NORMAL_ALPHA, "Обычная прозрачность (использовать Z)")
END_ENUM_DESCRIPTOR(CIRCLE_MANAGER_DRAW_ORDER)

void CircleColor::serialize(Archive& ar)
{
	ar.serialize(dotted, "dotted", "тип линии");
	ar.serialize(color, "color", "цвет");
	ar.serialize(width, "width", "толщина линии");
	ar.serialize(length, "length", 0);
}

CircleManager::CircleManager()
:cBaseGraphObject(0)
{
	pTexture=NULL;
	//pTexture=GetTexLibrary()->GetElement(param.texture_name.c_str());
	pTexture=GetTexLibrary()->GetElement3D(param.texture_name.c_str());
	sampler_circle=sampler_wrap_anisotropic;
	sampler_circle.addressv=DX_TADDRESS_CLAMP;
	legionColor.set(0,255,0);
	draw_order=CIRCLE_MANAGER_DRAW_BEFORE_GRASS_NOZ;
}

CircleManager::~CircleManager()
{
	RELEASE(pTexture);
}

void CircleManager::PreDraw(cCamera *pCamera)
{
	DWORD order=SCENENODE_OBJECT_NOZ_BEFORE_GRASS;
	if(draw_order==CIRCLE_MANAGER_DRAW_AFTER_GRASS_NOZ)
		order=SCENENODE_OBJECT_NOZ_AFTER_GRASS;
	if(draw_order==CIRCLE_MANAGER_DRAW_NORMAL_ALPHA)
		order=SCENENODE_OBJECTSORT;
		
	pCamera->Attach(order,this);
}

void CircleManager::Draw(cCamera *pCamera)
{
	circles.calc();
	list<vector<OrCircleSpline> > splines;

	gb_RenderDevice->SetWorldMaterial(ALPHA_BLEND,MatXf::ID,0,pTexture);
	gb_RenderDevice3D->SetSamplerData(0,sampler_circle);

	circles.get_circle_segment(splines,param.segment_len,param.length);
	list<vector<OrCircleSpline> >::iterator itv;
	FOR_EACH(splines,itv)
	{
		vector<OrCircleSpline>& spline=*itv;
		DrawSpline(spline);
	}
	circles.clear();
}

void CircleManager::Animate(float dt)
{
}

void CircleManager::addCircle(const Vect2f& pos,float radius)
{
	circles.add(pos,radius);
}

void CircleManager::DrawSpline(vector<OrCircleSpline>& spline)
{
	if(spline.empty())
		return;
	DrawStrip strip;

	strip.Begin();
	float size=param.width;
	float additionalz=0.0f;
	if(draw_order==CIRCLE_MANAGER_DRAW_NORMAL_ALPHA)
		additionalz=1.0f;

	cWater* pWater=environment->water();

	sVertexXYZDT1 v1,v2;
	if(param.useLegionColor)
		v1.diffuse=v2.diffuse=legionColor;
	else
		v1.diffuse=v2.diffuse=param.diffuse;
	OrCircleSpline sp=spline.back();
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
		{
			p.z=vMap.GetAltWhole(p.x,p.y);
			if(pWater)
			{
				float z=pWater->GetZ(p.x,p.y);
				if(p.z<z)
					p.z=z;
			}
		}else
			p.z=0;

		p.z+=additionalz;
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

void CircleManager::SetParam(const CircleManagerParam& param_)
{
	param=param_;
	RELEASE(pTexture);
	//pTexture=GetTexLibrary()->GetElement(param.texture_name.c_str());
	pTexture=GetTexLibrary()->GetElement3D(param.texture_name.c_str());
}

void CircleManager::SetLegionColor(sColor4c color)
{
	legionColor = color;
}

void CircleManager::clear()
{
	circles.clear();
}

CircleManagerParam::CircleManagerParam()
{
	length=30.0f;
	width=1.0f;
	segment_len=5.0f;
	texture_name="Scripts\\Resource\\Textures\\Region.tga";
	useLegionColor = false;
	diffuse.set(0,255,0);
}

void CircleManagerParam::serialize(Archive& ar)
{
	ar.serialize(ResourceSelector(texture_name, ResourceSelector::TEXTURE_OPTIONS), "CircleManager_texture_name", "Селект: Имя текстуры");
	ar.serialize(length, "CircleManager_length", "Селект: Длина линии");
	ar.serialize(width, "CircleManager_width", "Селект: Ширина линии");
	ar.serialize(segment_len, "CircleManager_segment_len", "Селект: Длина сегмента разбиения");
	ar.serialize(useLegionColor, "CircleManager_useLegionColor", "Селект: Использовать цвет легиона");
	

	if(!useLegionColor)
		ar.serialize(diffuse, "CircleManager_diffuse", "Селект: Цвет");
}
