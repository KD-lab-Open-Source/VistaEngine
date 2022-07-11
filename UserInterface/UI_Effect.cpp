#include "StdAfx.h"
#include "UI_Effect.h"
#include "Serialization\Serialization.h"
#include "VistaRender\StreamInterpolation.h"
#include "Render\Src\cCamera.h"
#include "Render\D3d\D3DRender.h"
#include "UI_Types.h"
#include "UI_Render.h"
#include "UI_BackgroundScene.h"
#include "Render\src\NParticle.h"
#include "Render\src\Scene.h"
#include "Render\3dx\Node3dx.h"

class FunctorSimpleZ : public FunctorGetZ
{
	float retZ_;
public:
	FunctorSimpleZ(float z = 0) : retZ_(z) {}

	float getZf(int x, int y) const
	{
		return retZ_;
	}
};

// --------------------------------------------------------------------------

UI_EffectAttribute::UI_EffectAttribute()
{
	isCycled_ = true;
	stopImmediately_ = false;
	legionColor_ = false;

	scale_ = 1.0f;
}

void UI_EffectAttribute::serialize(Archive& ar)
{
	ar.serialize(effectReference_, "effectReference", "&<");

	ar.serialize(isCycled_, "isCycled", "зацикливать");
	ar.serialize(stopImmediately_, "stopImmediately", "Обрывать при окончании");
	ar.serialize(legionColor_, "legionColor", "окрашивать в цвет модели");

	ar.serialize(scale_, "scale", "масштаб");
}

// --------------------------------------------------------------------------

void UI_EffectAttributeAttachable::serialize(Archive& ar)
{
	__super::serialize(ar);
	
	if(ar.isEdit()){
		ComboListString nodes(UI_BackgroundScene::instance().nodeComboList(), nodeLink_.c_str());
		ar.serialize(nodes, "nodeLink", "Узел привязки");
		if(ar.isInput())
			nodeLink_ = nodes;
	}
	else
		ar.serialize(nodeLink_, "nodeLink", 0);
}

// --------------------------------------------------------------------------

UI_EffectController::UI_EffectController()
: effectAttribute_(0)
, effect_(0)
{
}

void UI_EffectController::initEffect(cEffect* eff)
{
	eff->SetUseFogOfWar(false);

	eff->setCycled(effectAttribute_->isCycled());
	eff->SetAutoDeleteAfterLife(false);

	eff->setInterfaceEffectFlag(true);
	eff->toggleDistanceCheck(false);
	eff->SetParticleRate(1.f);

	static FunctorSimpleZ FunctorZeroZ;
	eff->SetFunctorGetZ(&FunctorZeroZ);
}

void UI_EffectController::effectStop(bool immediately)
{
	if(effect_){
		if(MT_IS_GRAPH())
			if(immediately || effectAttribute_->stopImmediately())
				effect_->Release();
			else{
				effect_->setCycled(false);
				effect_->SetAutoDeleteAfterLife(true);
			}
		else
			if(immediately || effectAttribute_->stopImmediately())
				streamLogicPostCommand.set(fCommandRelease, effect_);
			else{
				streamLogicPostCommand.set(fCommandSetCycle, effect_) << false;
				streamLogicPostCommand.set(fCommandSetAutoDeleteAfterLife, effect_) << true;
			}
		effect_ = 0;
	}
	effectAttribute_ = 0;
}

bool UI_EffectController::effectStart(const UI_EffectAttribute* attribute, const Vect3f& pose)
{
	if(!attribute || attribute->isEmpty()){
		effectStop();
		return false;
	}

	float moveToTime = -1.f;
	if(effect_){
		if(effectAttribute_){
			if(!effectAttribute_->eq(*attribute)){
				if(effectAttribute_->effectReference() == attribute->effectReference())
					moveToTime = effect_->GetTime();
				effectStop();
			}
		}
		else
			effectStop();
	}

	effectAttribute_ = attribute;

	if(!effect_){
		const cObject3dx* bg = 0;
		if(effectAttribute_->legionColor())
			if(const UI_BackgroundModel* model = static_cast<const UI_BackgroundScene&>(UI_BackgroundScene::instance()).currentModel())
				bg = model->model();

		EffectKey* key = bg ?
			effectAttribute_->effect(effectAttribute_->scale(), bg->GetSkinColor()) :
		effectAttribute_->effect(effectAttribute_->scale());

		if(!key)
			return false;

		cEffect* eff = UI_BackgroundScene::instance().scene()->CreateEffectDetached(*key, 0);
		if(!eff)
			return false;

		initEffect(eff);

		eff->SetPosition(MatXf(Mat3f::ID, pose));
		if(moveToTime > 0.1f)
			eff->MoveToTime(moveToTime);

		attachSmart(eff);
		effect_ = eff;
	}
	
	return true;
}

void UI_EffectController::drawDebugInfo(Camera* camera) const
{
	MTG();

	if(!isEnabled())
		return;

	if(showDebugEffects.axis){
		MatXf X = effect_->GetPosition();

		Vect3f pv, pe;
		camera->ConvertorWorldToViewPort(&X.trans(), &pv, &pe);
		if(pv.z > 0.5f)
			UI_Render::instance().outDebugText(UI_Render::instance().relativeCoords(pe), effectAttribute_->effectReference().c_str(), &Color4c::WHITE);

		Vect3f delta = X.rot().xcol();
		delta.normalize(15);
		gb_RenderDevice3D->DrawLine(X.trans(), X.trans() + delta, Color4c::RED);

		delta = X.rot().ycol();
		delta.normalize(15);
		gb_RenderDevice3D->DrawLine(X.trans(), X.trans() + delta, Color4c::GREEN);

		delta = X.rot().zcol();
		delta.normalize(15);
		gb_RenderDevice3D->DrawLine(X.trans(), X.trans() + delta, Color4c::BLUE);
	}
}

// --------------------------------------------------------------------------

bool UI_EffectController3D::effectStart(const UI_EffectAttribute* attribute, const Vect3f& pose)
{
	position_ = pose;
	return UI_EffectController::effectStart(attribute, position_);
}

void UI_EffectController3D::setPosition(const Vect3f& pos)
{
	if(cEffect* eff = effect_){
		if(!isEnabled())
			return;
		
		if(MT_IS_GRAPH())
			eff->SetPosition(MatXf(Mat3f::ID, position_));
		else {
			Se3f pose(MatXf(Mat3f::ID, position_));
			streamLogicInterpolator.set(fSe3fInterpolation, eff) << pose << Se3f(MatXf(Mat3f::ID, pos));
			position_ = pos;
		}
	}
}

// --------------------------------------------------------------------------

const Vect3f& UI_EffectControllerAttachable::position() const
{
	if(const cEffect* effect = effect_)
		return effect->GetPosition().trans();
	return Vect3f::ZERO;
}

// --------------------------------------------------------------------------

bool UI_EffectControllerAttachable3D::effectStart(const UI_EffectAttributeAttachable* attribute, cObject3dx* owner)
{
	xassert(owner);
	
	effectStop();
	
	if(!attribute || attribute->isEmpty())
		return false;

	effectAttribute_ = attribute;

	EffectKey* key = attribute->legionColor() ?
		effectAttribute_->effect(effectAttribute_->scale(), owner->GetSkinColor()) :
		effectAttribute_->effect(effectAttribute_->scale());

	if(!key)
		return false;

	cEffect* eff = UI_BackgroundScene::instance().scene()->CreateEffectDetached(*key, owner);
	if(!eff)
		return false;

	initEffect(eff);

	eff->LinkToNode(owner, max(owner->FindNode(attribute->node()), 0));

	attachSmart(eff);
	
	effect_ = eff;
	return true;
}

// --------------------------------------------------------------------------

bool UI_EffectControllerAttachable2D::effectStart(const UI_EffectAttribute* attribute, const UI_ControlBase* owner)
{
	xassert(owner);

	if(UI_EffectController::effectStart(attribute, calcPos(owner, UI_BackgroundScene::instance().camera()))){
		owner_ = owner;
		return true;
	}
	
	return false;
}

Vect3f UI_EffectControllerAttachable2D::calcPos(const UI_ControlBase* ctrl, Camera* camera)
{
	xassert(ctrl);
	Vect3f dir, pos;
	camera->GetWorldRay(UI_Render::instance().relative2deviceCoords(ctrl->transfPosition().center()), pos, dir);
	return pos.scaleAdd(dir, clamp(1024.f / fabs(dir.z) + 5.f * ctrl->screenZ(), 100.f, 2000.f));
}

void UI_EffectControllerAttachable2D::updatePosition()
{
	MTG();
	if(cEffect* effect = effect_)
		effect->SetPosition(MatXf(Mat3f::ID, calcPos(owner_, UI_BackgroundScene::instance().camera())));
}

void UI_EffectControllerAttachable2D::getDebugInfo(Camera* camera, XBuffer& buf) const
{
	MTG();
	xassert(owner_);

	if(cEffect* effect = effect_){
		//Vect2f pos2d = owner_->transfPosition().center();
		//Vect3f pos3d = position();
		//char str[128];
		//_snprintf(str, 127, "2D(%.3f, %.3f), 3D(%.2f, %.2f, %.2f)", pos2d.x, pos2d.y, pos3d.x, pos3d.y, pos3d.z);
		//buf < attr()->effectReference().c_str() < ", Link:" < UI_ControlReference(owner_).referenceString() < ", " < str;
		buf < attr()->effectReference().c_str() < ", Link:" < UI_ControlReference(owner_).referenceString();
	}
}