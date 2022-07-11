#include "StdAfx.h"
#include "UI_MarkObjectAttribute.h"

#include "Serialization\ResourceSelector.h"
#include "Game\RenderObjects.h"
#include "Game\CameraManager.h"
#include "Render\src\cCamera.h"
#include "Game\Universe.h"
#include "Units\WeaponTarget.h"
#include "Render\Src\Scene.h"
#include "Render\src\VisGeneric.h"

#include "UI_Render.h"
#include "UI_Logic.h"
#include "UI_MarkObject.h"

Vect3f G2S(const Vect3f &vg);

// ------------------------------- UI_MarkObjectAttribute

UI_MarkObjectAttribute::UI_MarkObjectAttribute()
{
	modelScale_ = 1.f;
	animationPeriod_ = 0;
	lifeTime_ = 0;
	synchronizeWithModel_ = false;
	rotateWithCamera_ = false;
	animateByAttack_ = false;
	finishAnimation_ = false;
}

void UI_MarkObjectAttribute::serialize(Archive& ar)
{
	if(ar.openBlock("model", "модель")){
		ar.serialize(ModelSelector(modelName_), "modelName", "имя модели");
		linkNode_.setName(modelName_.c_str());
		if(ar.isInput())
			setComboList(modelName_.c_str());
		ar.serialize(modelScale_, "modelScale", "масштаб модели");
		ar.serialize(animationName_, "animationName", "анимация");
		ar.serialize(linkNode_, "linkNode", "место линковки связи");
		ar.serialize(animationPeriod_, "animationPeriod", "период анимации (секунды)");
		ar.serialize(animateByAttack_, "animateByAttack", "анимировать при нажатой атаке");
		if(!animateByAttack_ && animationPeriod_ > 0.1f)
			ar.serialize(finishAnimation_, "finishAnimation", "доигрывать анимацию");
		ar.closeBlock();
	}

	ar.serialize(effect_, "effect", "спецэффект");
	ar.serialize(synchronizeWithModel_, "synchronizeWithModel", "синхронизировать эффект с анимацией модели");

	static ResourceSelector::Options options("*.cur", "Resource\\Cursors", "Cursors");
	ar.serialize(ResourceSelector(cursorfileName_, options), "cursorfileName", "Имя файла с курсором");
	if(ar.isInput())
		cursorProxy_.createCursor(cursorfileName_.c_str());

	ar.serialize(rotateWithCamera_, "rotateWithCamera", "поворачивать отметку вместе с камерой");

	ar.serialize(lifeTime_, "lifeTime", "время жизни в секундах");
}

void UI_MarkObjectAttribute::setComboList(const char* model_name)
{
	if(!strlen(model_name))
		return;
	
	if(!gb_VisGeneric)
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

UI_MarkObjectInfo::UI_MarkObjectInfo(UI_MarkType type, const Se3f pose, const UI_MarkObjectAttribute* attr, const BaseUniverseObject* owner, const UnitInterface* target) :
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

	updated_ = 0;
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

void UI_MarkObject::setPose(const Se3f& _pose, bool init)
{
	__super::setPose(_pose, init);

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
	target_ = 0;
}

bool UI_MarkObject::update(const UI_MarkObjectInfo& inf)
{
	MTG();

	updated_ = gb_VisGeneric->GetGraphLogicQuant() + 1;

	type_ = inf.type();
	owner_ = inf.owner();

	target_ = inf.target();

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
			}
		}

		if(attribute_->hasEffect()){
			effect_.setPlacementMode(EffectController::PLACEMENT_BY_ATTRIBUTE);
			effect_.effectStart(&attribute_->effect());
		}

	}

	Vect3f pos3d(inf.pose().trans());
	calcTargetMarkPosition(pos3d);
	
	setPose(Se3f(inf.pose().rot(), pos3d), true);

	lifeTime_ = 0.f;

	return true;
}

bool UI_MarkObject::calcTargetMarkPosition(Vect3f& pos3d)
{
	MTG();
	if(const UnitInterface* target = target_){
		pos3d = target->interpolatedPose().trans();
		if(Camera* camera = cameraManager->GetCamera()){
			Vect3f dir;
			pos3d -= dir.sub(pos3d, camera->GetPos()).normalize(1.5f * target->radius());
		}
		return true;
	}
	return false;
}

bool UI_MarkObject::quant(float dt)
{
	MTG();

	xassert(attribute_);

	Vect3f pos(position());
	if(calcTargetMarkPosition(pos))
		setPose(Se3f(orientation(), pos), false);

	bool animationLock = false;
	if(model_ && attribute_->animationPeriod() > FLT_EPS){
		if(!attribute_->animateByAttack() || UI_LogicDispatcher::instance().isAttackPressed())
			animationPhase_ += dt / attribute_->animationPeriod();
		if(animationPhase_ > 1.f && attribute_->syncEffectWithModel() && attribute_->hasEffect()){
			effect_.release();
			effect_.effectStart(&attribute_->effect());
		}
		if(attribute_->finishAnimation() && animationPhase_ < 1.f)
			animationLock = true;
		animationPhase_ = cycle(animationPhase_, 1.f);
		model_->SetPhase(animationPhase_);
	}

	bool isAlive = (animationLock || gb_VisGeneric->GetGraphLogicQuant() <= updated_ || lifeTime_ < attribute_->lifeTime());

	lifeTime_ = (owner_ ? lifeTime_ : attribute_->lifeTime()) + dt;
	
	return isAlive;
}

void UI_MarkObject::showDebugInfo() const
{
	show_vector(position(), 10, Color4c::RED);

	//Vect2f v = Vect2f(G2S(position()));
	//Recti scr_pos = Recti(Vect2i(v) - Vect2i(5,5), Vect2i(10,10));
	//Rectf out_pos = UI_Render::instance().relativeCoords(scr_pos);
	//UI_Render::instance().drawRectangle(out_pos, Color4f(0.5f,1.f,0.5f,0.5f), true);

	effect_.showDebugInfo();
}

void UI_MarkObject::release()
{
	RELEASE(model_);
	effect_.release();
}

//-------------------------------------------------------------------

void UI_LinkToMarkObject::updateLink(const cObject3dx* obj, int nodeIndex)
{
	MTG();
	if(isEnabled() && obj){
		xassert(owner() && owner()->attribute_);
		Vect3f pos = (owner()->model_
			? owner()->model_->GetNodePositionMats(owner()->attribute_->linkNodeIndex()).trans()
			: owner()->position());
		effect_->SetTarget(pos, obj->GetNodePositionMats(nodeIndex).trans());
	}
}