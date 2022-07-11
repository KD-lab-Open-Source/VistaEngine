#include "StdAfx.h"
#include "CircleManager.h"
#include "Serialization\ResourceSelector.h"
#include "Render\D3D\D3DRender.h"
#include "Render\Src\cCamera.h"
#include "Water\Water.h"
#include "Terra\vMap.h"
#include "Serialization\EnumDescriptor.h"

BEGIN_ENUM_DESCRIPTOR(CircleManagerDrawOrder, "CircleManagerDrawOrder")
REGISTER_ENUM(CIRCLE_MANAGER_DRAW_AFTER_GRASS_NOZ, "Выше травы без Z")
REGISTER_ENUM(CIRCLE_MANAGER_DRAW_BEFORE_GRASS_NOZ, "Ниже травы без Z")
REGISTER_ENUM(CIRCLE_MANAGER_DRAW_NORMAL_ALPHA, "Обычная прозрачность (использовать Z)")
END_ENUM_DESCRIPTOR(CircleManagerDrawOrder)

CircleManagerDrawOrder CircleManager::currentDrawOrder_;
Color4c CircleManager::currentLegionColor_;
MTSection CircleManager::lock_;

CircleManager::CircleManager()
:BaseGraphObject(0)
{
	samplerCircle_=sampler_wrap_anisotropic;
	samplerCircle_.addressv=DX_TADDRESS_CLAMP;
	legionColor_.set(0,255,0);
	drawOrder_=CIRCLE_MANAGER_DRAW_BEFORE_GRASS_NOZ;
}

CircleManager::~CircleManager()
{
}

void CircleManager::PreDraw(Camera *camera)
{
	SceneNode order=SCENENODE_OBJECT_NOZ_BEFORE_GRASS;
	if(drawOrder_==CIRCLE_MANAGER_DRAW_AFTER_GRASS_NOZ)
		order=SCENENODE_OBJECT_NOZ_AFTER_GRASS;
	if(drawOrder_==CIRCLE_MANAGER_DRAW_NORMAL_ALPHA)
		order=SCENENODE_OBJECTSORT;
		
	camera->Attach(order,this);
}

void CircleManager::Draw(Camera* camera)
{
	MTAuto lock(lock_);

	currentDrawOrder_= drawOrder_;
	currentLegionColor_ = legionColor_;

	gb_RenderDevice3D->SetSamplerData(0,samplerCircle_);

	Layers::iterator i;
	FOR_EACH(layers_, i)
		i->second.draw();

	clearLayers();
}

void CircleManager::addCircle(const Vect2f& pos, float radius, const CircleManagerParam& param)
{
	if(param.color.a < FLT_EPS || radius < FLT_EPS)
		return;

	MTAuto lock(lock_);

	Layers::iterator i = layers_.find(param.color.RGBA());
	if(i == layers_.end())
		i = layers_.insert(Layers::value_type(param.color.RGBA(), Layer(param))).first;
	i->second.circles->add(pos, radius);
}

void CircleManager::SetLegionColor(Color4c color)
{
	legionColor_ = color;
}

void CircleManager::clearLayers()
{
	Layers::iterator layer;
	FOR_EACH(layers_, layer)
		layer->second.circles->clear();
}

void CircleManager::clear()
{
	layers_.clear();
}

CircleManager::Layer::Layer(const CircleManagerParam& _param)
{
	param = _param;
	texture = GetTexLibrary()->GetElement3D(param.texture.c_str());
	circles = new OrCircle;
}

void CircleManager::Layer::draw()
{
	gb_RenderDevice->SetWorldMaterial(ALPHA_BLEND,MatXf::ID,0, texture);

	circles->calc();
	OrCircleSplines splines;
	circles->get_circle_segment(splines, param.segmentLength, param.length);
	OrCircleSplines::iterator spline;
	FOR_EACH(splines, spline)
		drawSpline(*spline);
//	circles->testDraw();
}

void CircleManager::Layer::drawSpline(OrCircleSpline& spline)
{
	if(spline.empty())
		return;
	DrawStrip strip;

	strip.Begin();
	float size=param.width;
	float additionalz=0.0f;
	if(currentDrawOrder_==CIRCLE_MANAGER_DRAW_NORMAL_ALPHA)
		additionalz=1.0f;

	sVertexXYZDT1 v1,v2;
	if(param.useLegionColor)
		v1.diffuse=v2.diffuse=currentLegionColor_;
	else
		v1.diffuse=v2.diffuse=param.color;
	OrCircleSplineElement sp=spline.back();
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
			p.z=vMap.getAltWhole(p.x,p.y);
			if(water)
			{
				float z=water->GetZ(p.x,p.y);
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

CircleManagerParam::CircleManagerParam(const Color4c& colorIn)
{
	color = colorIn;
	length=30.0f;
	width=1.0f;
	segmentLength=5.0f;
	texture="Scripts\\Resource\\Textures\\Region.tga";
	useLegionColor = false;
}

void CircleManagerParam::serialize(Archive& ar)
{
	static ResourceSelector::Options textureOptions("*.tga", "Resource\\TerrainData\\Textures");
	// CONVERSION 23.04.07
	if(ar.isInput() && !ar.isEdit() && ar.serialize(useLegionColor, "CircleManager_useLegionColor", "Использовать цвет легиона")){
		if(!useLegionColor)
			ar.serialize(color, "CircleManager_diffuse", "Цвет");
		ar.serialize(ResourceSelector(texture, textureOptions), "CircleManager_texture_name", "Имя текстуры");

		ar.serialize(length, "CircleManager_length", "Длина линии");
		ar.serialize(width, "CircleManager_width", "Ширина линии");
		ar.serialize(segmentLength, "CircleManager_segment_len", "Длина сегмента разбиения");
	}

	ar.serialize(useLegionColor, "useLegionColor", "Использовать цвет легиона");
	if(!useLegionColor)
		ar.serialize(color, "color", "Цвет");
	ar.serialize(ResourceSelector(texture, textureOptions), "texture", "Имя текстуры");

	ar.serialize(length, "length", "Длина линии");
	ar.serialize(width, "width", "Ширина линии");
	ar.serialize(segmentLength, "segmentLength", "Длина сегмента разбиения");
}
