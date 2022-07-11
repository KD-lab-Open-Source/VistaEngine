#include "stdafx.h"
#include "Serialization\Serialization.h"
#include "ShowCircleController.h"
#include "..\Environment\Environment.h"
#include "ExternalShow.h"

void CircleEffect::serialize(Archive& ar)
{
	ar.serialize(type_, "type", "“ип обозначени€");
	if(type_ == EFFECT)
		ar.serialize(color, "color", "цвет");
	else
		__super::serialize(ar);
}

ShowCircleController::ShowCircleController(const CircleEffect& circle)
: circle_(&circle)
, light_(0)
{
	if(circle_->type() != CircleEffect::CIRCLE && circle_->color.a > 0){
		light_ = environment->scene()->CreateLight(ATTRLIGHT_SPHERICAL_TERRAIN, GetTexLibrary()->GetSpericalTexture());
		xassert(light_);
		light_->SetRadius(1.f);
	}
	updated_ = true;
}

void ShowCircleController::release()
{
	RELEASE(light_);
}

void ShowCircleController::redraw(const Vect3f& pos, float radius)
{
	updated_ = true;
	if(circle_->type() != CircleEffect::EFFECT)
			gbCircleShow->Circle(pos, radius, *circle_);
	if(circle_->type() != CircleEffect::CIRCLE && light_){
		sLightKey key = { circle_->color, radius };
		light_->SetAnimKeys(&key, 1);
		light_->SetPosition(MatXf(MatXf::ID.rot(), pos));
	}
}


BEGIN_ENUM_DESCRIPTOR_ENCLOSED(CircleEffect, GraphType, "GraphType")
REGISTER_ENUM_ENCLOSED(CircleEffect, EFFECT, "Ёффект")
REGISTER_ENUM_ENCLOSED(CircleEffect, CIRCLE, "ќкружность")
REGISTER_ENUM_ENCLOSED(CircleEffect, BOTH, "эффект и окружность")
END_ENUM_DESCRIPTOR_ENCLOSED(CircleEffect, GraphType)
