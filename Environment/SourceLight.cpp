#include "stdafx.h"

#include "SourceLight.h"
#include "Serialization.h"

#include "Environment.h"
#include "..\Terra\vmap.h"


SourceLight::SourceLight()
: SourceBase()
, pSpherLight_(NULL)
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
, pSpherLight_(NULL)
{
	frequency_ = l.frequency_;
	AnimKeys_[0] = l.AnimKeys_[0];
	AnimKeys_[1] = l.AnimKeys_[1];
	radius_ = AnimKeys_[0].radius;
	toObjects_ = l.toObjects_;
}

SourceLight::~SourceLight()
{
	xassert(!pSpherLight_);
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

	if(pSpherLight_)
		pSpherLight_->GetPos() = pos.trans();
}

void SourceLight::start()
{
	__super::start();

	if(!pSpherLight_)
		createLight(environment->scene());
	if(pSpherLight_)
		pSpherLight_->ClearAttr(ATTRLIGHT_IGNORE);
}

void SourceLight::stop()
{
	__super::stop();

	if(pSpherLight_)
		RELEASE(pSpherLight_);
}

void SourceLight::quant()
{
	__super::quant();
	if(pSpherLight_) {
		poseInterpolator_ = pose();
		poseInterpolator_(pSpherLight_);
	}
}

void SourceLight::createLight(cScene* scene)
{
	xassert(scene);
	xassert(!pSpherLight_);
	
	cTexture* sperical=GetTexLibrary()->GetSpericalTexture();
	if(toObjects_)
		pSpherLight_ = scene->CreateLight(ATTRLIGHT_SPHERICAL_OBJECT|ATTRLIGHT_SPHERICAL_TERRAIN,sperical);
	else
		pSpherLight_ = scene->CreateLight(ATTRLIGHT_SPHERICAL_TERRAIN,sperical);
	
	xassert(pSpherLight_);
	
	pose_.trans().z = vMap.getVoxelW(pose_.trans().xi(), pose_.trans().yi());
	
	pSpherLight_->SetPosition(MatXf(Mat3f::ID,pose_.trans()));
	
	AnimKeys_[1].radius = AnimKeys_[0].radius;
	pSpherLight_->SetAnimKeys(AnimKeys_,2);
	
	if(frequency_ > 0.001f)
		pSpherLight_->SetAnimation(1.f/frequency_);
	else
		pSpherLight_->SetAnimation(1e20f);
	
	if(!active())
		pSpherLight_->SetAttr(ATTRLIGHT_IGNORE);
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
