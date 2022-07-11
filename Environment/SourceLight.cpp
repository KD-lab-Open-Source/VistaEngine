#include "stdafx.h"

#include "SourceLight.h"
#include "Serialization\Serialization.h"
#include "Render\Src\TexLibrary.h"
#include "Render\Src\Scene.h"

#include "Environment.h"
#include "Terra\vmap.h"


SourceLight::SourceLight()
: SourceBase()
, light_(NULL)
{
	frequency_ = 0.f;
	AnimKeys_[0].diffuse.set(255, 255, 255, 128);
	AnimKeys_[0].radius=50.0f;
	AnimKeys_[1].diffuse.set(255, 255, 255, 128);
	AnimKeys_[1].radius=50.0f;
	radius_ = AnimKeys_[0].radius;
	toObjects_ = true;
}

SourceLight::SourceLight(const SourceLight& l)
: SourceBase(l)
, light_(NULL)
{
	frequency_ = l.frequency_;
	AnimKeys_[0] = l.AnimKeys_[0];
	AnimKeys_[1] = l.AnimKeys_[1];
	radius_ = AnimKeys_[0].radius;
	toObjects_ = l.toObjects_;
}

SourceLight::~SourceLight()
{
	xassert(!light_);
}

void SourceLight::setRadius(float _radius)
{
	AnimKeys_[0].radius = _radius;

	__super::setRadius(_radius);
}

void SourceLight::setPose(const Se3f& pos, bool init)
{
	__super::setPose(pos, init);

	if(init)
		poseInterpolator_.initialize();

	if(light_)
		light_->GetPos() = pos.trans();
}

void SourceLight::start()
{
	__super::start();

	if(!light_)
		createLight(environment->scene());
	if(light_)
		light_->clearAttribute(ATTRLIGHT_IGNORE);
}

void SourceLight::stop()
{
	__super::stop();

	if(light_)
		RELEASE(light_);
}

void SourceLight::quant()
{
	__super::quant();
	if(light_) {
		poseInterpolator_ = pose();
		poseInterpolator_(light_);
	}
}

void SourceLight::createLight(cScene* scene)
{
	xassert(scene);
	xassert(!light_);
	
	cTexture* sperical=GetTexLibrary()->GetSpericalTexture();
	if(toObjects_)
		light_ = scene->CreateLight(ATTRLIGHT_SPHERICAL_OBJECT|ATTRLIGHT_SPHERICAL_TERRAIN,sperical);
	else
		light_ = scene->CreateLight(ATTRLIGHT_SPHERICAL_TERRAIN,sperical);
	
	xassert(light_);
	
	pose_.trans().z = vMap.getApproxAlt(pose_.trans().xi(), pose_.trans().yi());
	
	light_->SetPosition(MatXf(Mat3f::ID,pose_.trans()));
	
	AnimKeys_[1].radius = AnimKeys_[0].radius;
	light_->SetAnimKeys(AnimKeys_,2);
	
	if(frequency_ > 0.001f)
		light_->SetAnimationPeriod(1.f/frequency_);
	else
		light_->SetAnimationPeriod(1e20f);
	
	if(!active())
		light_->setAttribute(ATTRLIGHT_IGNORE);
}

void SourceLight::serialize(Archive& ar) 
{
	__super::serialize(ar);

	AnimKeys_[0].radius = radius();

	//ar.serialize(AnimKeys_[0].rotate.x, "Radius1", "Радиус 1");
	//radius_ = AnimKeys_[0].rotate.x;
	
	ar.serialize(AnimKeys_[1].radius, "Radius2", "Радиус 2");
	ar.serialize(AnimKeys_[0].diffuse, "Light_color1", "Цвет 1");
	ar.serialize(AnimKeys_[1].diffuse, "Light_color2", "Цвет 2");
	ar.serialize(frequency_, "frequency_", "Частота мерцания");
	ar.serialize(toObjects_, "toObjects_", "На объеты");
	
	serializationApply(ar);
}
