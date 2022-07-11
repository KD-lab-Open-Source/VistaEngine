#include "StdAfx.h"
#include "UI_MarkObjectAttribute.h"

#include "..\Util\ResourceSelector.h"
#include "..\Game\RenderObjects.h"
#include "..\Game\Universe.h"
#include "..\Units\WeaponTarget.h"

#include "UI_Render.h"
#include "UI_MarkObject.h"

Vect3f G2S(const Vect3f &vg);

// ------------------------------- UI_MarkObjectAttribute

UI_MarkObjectAttribute::UI_MarkObjectAttribute()
{
	modelScale_ = 1.f;
	animationPeriod_ = 0;
	lifeTime_ = 0;
	synchronizeWithModel_ = false;
}

void UI_MarkObjectAttribute::serialize(Archive& ar)
{
	ar.openBlock("model", "модель");
	ar.serialize(ModelSelector(modelName_), "modelName", "имя модели");
	ar.serialize(modelScale_, "modelScale", "масштаб модели");
	ar.serialize(animationName_, "animationName", "анимация");
	ar.serialize(animationPeriod_, "animationPeriod", "период анимации (секунды)");
	ar.closeBlock();

	ar.serialize(effect_, "effect", "спецэффект");
	ar.serialize(synchronizeWithModel_, "synchronizeWithModel", "синхронизировать эффект с анимацией модели");
	ar.serialize(lifeTime_, "lifeTime", "время жизни в секундах");
}

void UI_MarkObjectAttribute::setComboList(const char* model_name)
{
	if(!strlen(model_name))
		return;
	
	cScene* scene = gb_VisGeneric->CreateScene();
	cObject3dx* model = scene->CreateObject3dx(model_name);
	if(model){
		string comboList;
		int number = model->GetChainNumber();
		for(int i = 0; i < number; i++){
			comboList += "|";
			comboList += model->GetChain(i)->name;
		}
		animationName_.setComboList(comboList.c_str());
		model->Release();
	}
	scene->Release();
}

// ------------------------------- UI_MarkObjectInfo

UI_MarkObjectInfo::UI_MarkObjectInfo(UI_MarkType type, const Se3f pose, const UI_MarkObjectAttribute* attr, const UnitInterface* owner, const UnitInterface* target/*, EffectController::PlacementMode placement_mode*/) :
	type_(type),
	pose_(pose),
	attribute_(attr),
	owner_(owner),
	target_(target)
{
}

// ------------------------------- UI_MarkObject

UI_MarkObject::UI_MarkObject() : model_(0),
	attribute_(0),
	owner_(0),
	target_(0),
	effect_(0)
{
	effect_.setOwner(this);
	type_ = UI_MARK_TYPE_DEFAULT;
	animationPhase_ = 0;

	updated_ = false;
	lifeTime_ = 0;
}

UI_MarkObject::UI_MarkObject(const UI_MarkObject& obj) : BaseUniverseObject(obj),
	type_(obj.type_),
	updated_(obj.updated_),
	model_(obj.model_),
	animationPhase_(obj.animationPhase_),
	effect_(obj.effect_),
	lifeTime_(obj.lifeTime_),
	attribute_(obj.attribute_),
	owner_(obj.owner_),
	target_(obj.target_)
{
	effect_.setOwner(this);
}

UI_MarkObject& UI_MarkObject::operator = (const UI_MarkObject& obj)
{
	if(this == &obj)
		return *this;

	*static_cast<BaseUniverseObject*>(this) = obj;

	type_ = obj.type_;
	updated_ = obj.updated_;
	model_ = obj.model_;
	animationPhase_ = obj.animationPhase_;
	effect_ = obj.effect_;
	lifeTime_ = obj.lifeTime_;
	attribute_ = obj.attribute_;
	owner_ = obj.owner_;
	target_ = obj.target_;

	effect_.setOwner(this);

	return *this;
}

UI_MarkObject::~UI_MarkObject()
{
}

bool UI_MarkObject::operator == (const UI_MarkObjectInfo& inf) const
{
	return (type_ == inf.type() && attribute_ == inf.attribute() && pose().trans().eq(inf.pose().trans(), 1.f));
}

bool UI_MarkObject::operator == (const UI_MarkObject& obj) const
{
	return (type_ == obj.type_ && owner_ == obj.owner_ && attribute_ == obj.attribute_);
}

void UI_MarkObject::setPose(const Se3f& _pose, bool init)
{
	__super::setPose(_pose, init);

	if(init)
		return;

	if(model_)
		model_->SetPosition(pose());
	
	effect_.updatePosition();
}

void UI_MarkObject::clear()
{
	release();

	type_ = UI_MARK_TYPE_DEFAULT;
	attribute_ = 0;
	owner_ = 0;
}

bool UI_MarkObject::update(const UI_MarkObjectInfo& inf)
{
	MTG();

	updated_ = true;

	type_ = inf.type();
	owner_ = inf.owner();

	target_ = inf.target();

	if(target_)
		setPose(Se3f(QuatF::ID, target_->interpolatedPose().trans()), true);
	else
		setPose(inf.pose(), true);

	if(attribute_ != inf.attribute()){
		release();

		attribute_ = inf.attribute();

		if(!attribute_)
			return true;

		animationPhase_ = 0;

		if(attribute_->hasModel()){
			model_ = terScene->CreateObject3dx(attribute_->modelName());
			if(model_){
				model_->SetScale(attribute_->scale());

				if(attribute_->hasAnimation())
					model_->SetChain(attribute_->animationName());

				if(Player* pl = universe()->activePlayer())
					model_->SetSkinColor(pl->unitColor());

				model_->SetPosition(pose());
			}
		}

		if(attribute_->hasEffect()){
			effect_.setPlacementMode(EffectController::PLACEMENT_BY_ATTRIBUTE);
			effect_.effectStart(&attribute_->effect());
		}

	}

	if(model_)
		model_->SetPosition(pose());

	effect_.updatePosition();

	restartTimer();

	return true;
}

void UI_MarkObject::restartTimer(float time)
{
	lifeTime_ = -time;
}

bool UI_MarkObject::quant(float dt)
{
	MTG();

	xassert(attribute_);

	if(target_){
		Se3f pos = Se3f(QuatF::ID, target_->interpolatedPose().trans());
		setPose(pos, false);
	}

	if(model_ && attribute_->animationPeriod() > FLT_EPS){
		animationPhase_ += dt / attribute_->animationPeriod();
		if (animationPhase_>1.f&&attribute_->syncEffectWithModel()&&attribute_->hasEffect())
		{
			effect_.release();
			effect_.effectStart(&attribute_->effect());
		}
		animationPhase_ = cycle(animationPhase_, 1.f);
		model_->SetPhase(animationPhase_);
	}

	bool isAlive = (updated_ || lifeTime_ < attribute_->lifeTime());

	lifeTime_ = (owner_ ? lifeTime_ : attribute_->lifeTime()) + dt;
	updated_ = false;
	
	return isAlive;
}

void UI_MarkObject::showDebugInfo() const
{
	show_vector(position(), 10, RED);

	//Vect2f v = Vect2f(G2S(position()));
	//Recti scr_pos = Recti(Vect2i(v) - Vect2i(5,5), Vect2i(10,10));
	//Rectf out_pos = UI_Render::instance().relativeCoords(scr_pos);
	//UI_Render::instance().drawRectangle(out_pos, sColor4f(0.5f,1.f,0.5f,0.5f), true);

	effect_.showDebugInfo();
}

void UI_MarkObject::release()
{
	RELEASE(model_);
	effect_.release();
}
