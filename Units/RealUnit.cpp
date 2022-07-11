#include "StdAfx.h"
#include "RealUnit.h"
#include "Universe.h"

#include "RenderObjects.h"
#include "Serialization\Serialization.h"
#include "Serialization\SerializationFactory.h"
#include "GlobalAttributes.h"
#include "GameOptions.h"
#include "IronBullet.h"
#include "UnitAttribute.h"
#include "Environment\Environment.h"
#include "Water\Water.h"
#include "IronBuilding.h"
#include "IronLegion.h"
#include "Physics\RigidBodyUnitRagDoll.h"
#include "Physics\crash\CrashSystem.h"
#include "Terra\vMap.h"
#include "Render\src\Scene.h"
#include "EditorVisual.h"

#pragma warning(disable: 4355)

UNIT_LINK_GET(UnitReal)

DECLARE_SEGMENT(UnitReal)
REGISTER_CLASS(UnitBase, UnitReal, "Юнит");
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_REAL, UnitReal)

BEGIN_ENUM_DESCRIPTOR(AutoAttackMode, "Режими атаки (без цели)")
REGISTER_ENUM(ATTACK_MODE_DISABLE, "Не атаковать")
REGISTER_ENUM(ATTACK_MODE_DEFENCE, "Защита")
REGISTER_ENUM(ATTACK_MODE_OFFENCE, "Нападение")
END_ENUM_DESCRIPTOR(AutoAttackMode)

BEGIN_ENUM_DESCRIPTOR(WalkAttackMode, "Режими атаки при движении")
REGISTER_ENUM(WALK_NOT_ATTACK, "Не останавливатся и не атаковать")
REGISTER_ENUM(WALK_AND_ATTACK, "Атаковать на ходу")
REGISTER_ENUM(WALK_STOP_AND_ATTACK, "Останавливатся и атаковать")
END_ENUM_DESCRIPTOR(WalkAttackMode)

BEGIN_ENUM_DESCRIPTOR(AutoTargetFilter, "Режим автоматического выбора целей")
REGISTER_ENUM(AUTO_ATTACK_ALL, "Атаковать всех")
REGISTER_ENUM(AUTO_ATTACK_BUILDINGS, "Атаковать только здания")
REGISTER_ENUM(AUTO_ATTACK_UNITS, "Атаковать только юнитов")
END_ENUM_DESCRIPTOR(AutoTargetFilter)

BEGIN_ENUM_DESCRIPTOR(WeaponMode, "Режими оружия")
REGISTER_ENUM(LONG_RANGE, "Атаковать только оружием дальнего боя")
REGISTER_ENUM(SHORT_RANGE, "Атаковать только оружием ближнего боя")
REGISTER_ENUM(ANY_RANGE, "Атаковать любым оружием")
END_ENUM_DESCRIPTOR(WeaponMode)

BEGIN_ENUM_DESCRIPTOR(ProduceType, "Тип производства")
REGISTER_ENUM(PRODUCE_INVALID, "Нет производства")
REGISTER_ENUM(PRODUCE_RESOURCE, "Производство ресурса")
REGISTER_ENUM(PRODUCE_UNIT, "Производство юнита")
END_ENUM_DESCRIPTOR(ProduceType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UnitReal, UnitState, "UnitReal::UnitState")
REGISTER_ENUM_ENCLOSED(UnitReal, AUTO_MODE, "AUTO_MODE")
REGISTER_ENUM_ENCLOSED(UnitReal, ATTACK_MODE, "ATTACK_MODE")
REGISTER_ENUM_ENCLOSED(UnitReal, MOVE_MODE, "MOVE_MODE")
REGISTER_ENUM_ENCLOSED(UnitReal, TRIGGER_MODE, "TRIGGER_MODE")
END_ENUM_DESCRIPTOR_ENCLOSED(UnitReal, UnitState)

UnitReal::UnitReal(const UnitTemplate& data) 
: UnitInterface(data), chainControllers_(this)
{
	corpseColisionTimer_ = 0;
	frozenCounter_ = 0;
	frozenAttack_ = false;
	freezeAnimationCounter_ = 0;

	model_ = 0;
	modelLogic_ = 0;
	modelLogicUpdated_ = false;

	gripNodeRelativePose_ = Se3f::ID;
	useGripNode_ = false;

	mainChain_ = 0;

	setModel(attr().modelName.c_str());
	if(!model()){
		xassertStr(0 && "Не установлена или не найдена модель у юнита ", attr().libraryKey());
		Kill(); 
		return;
	}

	animationStopReason_ = 0;
	
	setUnitState(AUTO_MODE);

	riseFur_ = true;
}

UnitReal::~UnitReal()
{
	RELEASE(model_);
	RELEASE(modelLogic_);
}

void UnitReal::serialize (Archive& ar) 
{
    __super::serialize(ar);

	ar.serialize(label_, "label", "Метка");

	if(universe()->userSave()){
		ar.serialize(dockingController_, "dockingController", 0);
		ar.serialize(useGripNode_, "useGripNode", 0);
		ar.serialize(unitState_, "unitState", 0);
		ar.serialize(chainDelayTimer, "chainDelayTimer", 0);
		vector<int> chainIndexes;
		int mainChainIndex;
		if(ar.isOutput()){
			vector<ChainController>::iterator chainController;
			FOR_EACH(chainControllers_.chainControllers(), chainController)
				chainIndexes.push_back(attr().animationChainIndex(chainController->chain()));
			mainChainIndex = attr().animationChainIndex(mainChain_);
		} 
		ar.serialize(stateController_, "stateController", 0);
		int hideReason(hideReason_);
		ar.serialize(hideReason, "hideReason", 0);
		ar.serialize(chainIndexes, "chainIndexes", 0);
		ar.serialize(mainChainIndex, "mainChainIndex", 0);
		if(ar.isInput()){
			if(hideReason & HIDE_BY_PLACED_BUILDING){
				setCollisionGroup(attr().collisionGroup);
				hideReason &= ~HIDE_BY_PLACED_BUILDING;
			}
			if(hideReason)
				hide(hideReason, true);
			MovementState mstate = getMovementState();
			vector<int>::iterator chainIndex;
			FOR_EACH(chainIndexes, chainIndex)
				setChain(attr().animationChainByIndex(*chainIndex), mstate);
			mainChain_ = attr().animationChainByIndex(mainChainIndex);
		}
		ar.serialize(chainControllers_, "chainControllers", 0);
		if(currentState() == CHAIN_DEATH){
			alive_ = false;
			if(ar.isInput())
				Kill();
		}
	}
}

void UnitReal::setModel(const char* name)
{
	if(!name || !name[0])
		return;
	
	cObject3dx* modelIn = terScene->CreateObject3dxDetached(name, NULL, GlobalAttributes::instance().enableAnimationInterpolation);

	if(!modelIn)
		return;

	setModel(modelIn);
	
	setModelLogic(name);

	setRadiusInternal();

	if(attr().showSilhouette && GlobalAttributes::instance().enableSilhouettes && GameOptions::instance().getBool(OPTION_SILHOUETTE) && !player()->isWorld()) {
		model()->setAttribute(ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE);
		model()->SetSilhouetteIndex(player()->playerID());
	}

	if(attr().hideByDistance)
		model()->setAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE);

    updateSkinColor();

	CalcSilouetteHeuristic();

	dayQuant(false);

	if(!attr().isActing())
		attachSmart(model());
	SetLodDistance(model(),attr().lodDistance);
}
	
void UnitReal::setModel(cObject3dx* modelIn)
{
	model_ = modelIn;

	chainControllers_.setModel(model());
}

void UnitReal::setModelLogic(const char* name)
{
	if(modelLogic_)
		modelLogic_->Release();
	modelLogic_ = terScene->CreateLogic3dx(name, GlobalAttributes::instance().enableAnimationInterpolation);
	if(modelLogic_)
		modelLogic_->SetPosition(pose());
	chainControllers_.setModelLogic(modelLogic_);
}

void UnitReal::CalcSilouetteHeuristic()
{
//Проблема такова - нужно по видимости одной точки определить, рисуется ли силуэт оюъекта.
//Здесь часть кода для определения, где расположенна точка.
	cObject3dx* pobj=modelLogic();
	if(pobj==NULL)
		pobj=model();
	if(pobj==NULL)
		return;

	sBox6f box;
	pobj->GetBoundBoxUnscaled(box);
	Vect3f center=(box.min+box.max)*0.5f;
	model()->SetSilouetteCenter(center);
}

void UnitReal::setRadiusInternal()
{
	__super::setRadius(attr().radius());

	if (model())
		model()->SetScale(attr().boundScale);
	if(modelLogic())
		modelLogic()->SetScale(attr().boundScale);
	
	if(modelLogic()){
		int gripNodeIndex = attr().gripNode;
		if(gripNodeIndex > 0){
			gripNodeRelativePose_ = modelLogicNodePosition(gripNodeIndex);
			gripNodeRelativePose_.invert();
		}
	}

	height_ = attr().boundBox.max.z - attr().boundBox.min.z;

	delete rigidBody_;
	if(model() && modelLogic() && !attr().rigidBodyModelPrm.empty()){
		rigidBody_ = RigidBodyBase::buildRigidBody(RIGID_BODY_UNIT_RAG_DOLL, attr().rigidBodyPrm, attr().boundBox.center(), attr().boundBox.extent(), attr().mass);
		modelLogic_->Update();
		safe_cast<RigidBodyUnitRagDoll*>(rigidBody())->setModel(model(), modelLogic(), attr().rigidBodyModelPrm);
	}else
		rigidBody_ = RigidBodyBase::buildRigidBody(attr().rigidBodyPrm->rigidBodyType, attr().rigidBodyPrm, attr().boundBox.center(), attr().boundBox.extent(), attr().mass);

	rigidBody()->setRadius(radius());
}

void UnitReal::MoveQuant()
{
	start_timer_auto();

	if(!dockingController_.attached())
		rigidBody()->evolve(logicPeriodSeconds);
	else
		setColor(dockingController_.dock()->color());

	if(!isUnderEditor() && collisionGroup() & COLLISION_GROUP_ACTIVE_COLLIDER && alive())
		testCollision();

	//log_var(attr().libraryKey());
	//log_var(rigidBody_->pose());

	setPose(rigidBody()->pose(), false);

	if(!attr().isActing() || !safe_cast<UnitActing*>(this)->rigidBody()->onIce()){
		if(rigidBody()->onWater()){
			if(water->isLava()){
				if(lavaEffect().isEnabled())
					setAbnormalState(lavaEffect(), 0);
			}else{
				if(waterEffect().isEnabled())
					setAbnormalState(waterEffect(), 0);
			}
		}else if(earthEffect().isEnabled())
			setAbnormalState(earthEffect(), 0);
	}
}

void UnitReal::Quant()
{
	start_timer_auto();

	__super::Quant();
	
	if(dead())
		return;

	MoveQuant();

	if(!isFrozen() || !alive()){
		start_timer_auto();
		if(furTimer_.started()){
			setFurPhase(riseFur_ ? furTimer_.factor() : 1.f - furTimer_.factor());
			if(1.f - furTimer_.factor() < FLT_EPS)
				furTimer_.stop();
		}
		stateController_.stateQuant();
		if(dead())
			return;
	}

	if(environment->dayChanged())
		dayQuant(attr().isActing() && safe_cast<const UnitActing*>(this)->isInvisible());

	interpolationQuant();
}

void UnitReal::dayQuant(bool invisible)
{
	if(hiddenLogic() || invisible){
		model()->setAttribute(ATTR3DX_HIDE_LIGHTS);
	}else if(environment->isDay()){
		stopAdditionalChain(CHAIN_NIGHT);
		if(int(attr().nightVisibilitySet) > 0){
			model()->clearAttribute(ATTR3DX_HIDE_LIGHTS);
			streamLogicCommand.set(fCommandSetVisibilityGroupOfSet, model()) 
				<< int(attr().dayVisibilityGroup) << int(attr().nightVisibilitySet);
		}else
			model()->setAttribute(ATTR3DX_HIDE_LIGHTS);
	}else{
		setAdditionalChain(CHAIN_NIGHT);
		model()->clearAttribute(ATTR3DX_HIDE_LIGHTS);
		if(int(attr().nightVisibilitySet) > 0)
			streamLogicCommand.set(fCommandSetVisibilityGroupOfSet, model()) 
				<< int(attr().nightVisibilityGroup) << int(attr().nightVisibilitySet);
	}
}

void UnitReal::setPose(const Se3f& poseIn, bool initPose)
{
	start_timer_auto();

	if(isDocked() && !initPose)
		return;
	
	Se3f poseClamped = poseIn;

	float delta = radius() + 2;
	poseClamped.trans().x = clamp(poseClamped.trans().x, delta, (vMap.H_SIZE - 1) - delta);
	poseClamped.trans().y = clamp(poseClamped.trans().y, delta, (vMap.V_SIZE - 1) - delta);
	
	Se3f posePrev = pose();

	start_timer(1)
	if(initPose){
		rigidBody()->initPose(poseClamped);
		UnitInterface::setPose(rigidBody()->pose(), initPose);
		posePrev = rigidBody()->pose();
	}
	else{
		UnitInterface::setPose(poseClamped, initPose);
	}
	stop_timer(1)

	modelLogicUpdated_ = false;
	if(modelLogic_)
		modelLogic_->SetPosition(pose_);

	if(model()){
		if(initPose)
			streamLogicCommand.set(fCommandSetPose, model()) << pose_;
		else
			streamLogicInterpolator.set(fSe3fInterpolation, model()) << posePrev << pose_;
	}

	if(initPose){
		initPoseTimer_.start(200);
		if(!currentState())
			stateController_.stateQuant();
	}
}

void UnitReal::updateDockPose()
{
	Se3f poseClamped;

	if(useGripNode_)
		poseClamped.mult(dockingController_.pose(), gripNodeRelativePose_);
	else
		poseClamped = dockingController_.pose();

	float delta = radius() + 2;
	poseClamped.trans().x = clamp(poseClamped.trans().x, delta, (vMap.H_SIZE - 1) - delta);
	poseClamped.trans().y = clamp(poseClamped.trans().y, delta, (vMap.V_SIZE - 1) - delta);

	Se3f posePrev = pose();

	rigidBody()->setPose(poseClamped);
	UnitBase::setPose(poseClamped, false);

	modelLogicUpdated_ = false;
	if(modelLogic_)
		modelLogic_->SetPosition(pose_);

	if(model())
		streamLogicInterpolator.set(fSe3fInterpolation, model()) << posePrev << pose_;
}

void UnitReal::showDebugInfo()
{
	__super::showDebugInfo();

	if(rigidBody())
		rigidBody()->show();

	if(!showDebugUnitReal.modelNode.empty() && model()){
		int node_index = model()->FindNode(showDebugUnitReal.modelNode.c_str());
		if(node_index != -1){
			MatXf X(model()->GetNodePosition(node_index));

			Vect3f delta = X.rot().xcol();
			delta.normalize(5);
			show_vector(X.trans(), delta, Color4c::RED);

			delta = X.rot().ycol();
			delta.normalize(5);
			show_vector(X.trans(), delta, Color4c::BLUE);

			delta = X.rot().zcol();
			delta.normalize(5);
			show_vector(X.trans(), delta, Color4c::GREEN);

			if(model()->HasUserTransform(node_index))
				show_vector(X.trans(), 2.f, Color4c::GREEN);
		}
	}

	if(!showDebugUnitReal.modelLogicNode.empty() && modelLogic()){
		int node_index = modelLogic()->FindNode(showDebugUnitReal.modelLogicNode.c_str());
		if(node_index != -1){
			MatXf X(modelLogic()->GetNodePosition(node_index));

			Vect3f delta = X.rot().xcol();
			delta.normalize(5);
			show_vector(X.trans(), delta, Color4c::RED);

			delta = X.rot().ycol();
			delta.normalize(5);
			show_vector(X.trans(), delta, Color4c::BLUE);

			delta = X.rot().zcol();
			delta.normalize(5);
			show_vector(X.trans(), delta, Color4c::GREEN);

			if(modelLogic()->HasUserTransform(node_index))
				show_vector(X.trans(), 3.f, Color4c::BLUE);
		}
	}

	if(showDebugUnitInterface.debugString)
		show_text(position(), debugString_.c_str(), Color4c::RED);

	if(showDebugUnitReal.parameters && !isDocked())
		parameters().showDebug(position(), Color4c::GREEN);

	if(showDebugUnitReal.parametersMax)
		parametersMax().showDebug(position(), Color4c::RED);

	if(showDebugUnitReal.waterDamage)
		waterEffect().damage().showDebug(position(), Color4c::RED);

	if(showDebugUnitReal.lavaDamage)
		lavaEffect().damage().showDebug(position(), Color4c::RED);

	if(showDebugUnitReal.iceDamage)
		iceEffect().damage().showDebug(position(), Color4c::RED);

	if(showDebugUnitReal.earthDamage)
		earthEffect().damage().showDebug(position(), Color4c::RED);

	if(showDebugUnitReal.positionValue){
		XBuffer msg(256, 1);
		msg.SetDigits(6);
		msg <= position();
		show_text(position(), msg, Color4c::GREEN);
	}

	if(showDebugUnitReal.chain)
		chainControllers_.showDebugInfo();

//	if(showDebugUnitReal.unitID){
//		XBuffer msg;
//		msg <= unitID().unitID();
//		show_text(position(), msg, MAGENTA);
//	}

	if(showDebugUnitReal.docking && isDocked())
		dockingController_.showDebug();

	if(showDebugUnitReal.currentChain){
		XBuffer msg(256, 1);
		msg.SetDigits(2);
		msg < getEnumName(currentState());
		msg < ": " <= chainDelayTimer.time() < "\n";
		msg < getEnumName(unitState()) < "\n";
		show_text(position() + Vect3f(0, 0, height()), msg, Color4c::RED);
	}
}

//----------------------------------------------
void UnitReal::attachToDock(UnitActing* dock, int nodeIndex, bool initPose, bool useGripNode)
{
	useGripNode_ = useGripNode;
	if(dock)
		dock->attachUnit(this);
	dockingController_.set(dock, nodeIndex);
	setCollisionGroup(attr().collisionGroup & ~COLLISION_GROUP_REAL);
	setPose(dockingController_.pose(), initPose);
}

void UnitReal::detachFromDock()
{
	if(dock())
		dock()->detachUnit(this);
	dockingController_.clear();
	setCollisionGroup(attr().collisionGroup);
	useGripNode_ = false;
	rigidBody()->awake();
}

void UnitReal::Kill()
{
	if(alive_){
		__super::Kill();
		if(model())
			streamLogicPostCommand.set(fCommandSetIgnored, model()) << true;
		startState(StateBase::instance());
	}else if(!dead()){
		unitID_.unregisterUnit();
		unitID_.registerUnit(this);
	}
}

void UnitReal::interpolationQuant()
{
	if(!animationStopReason_){
		float forwardVelocityFactor = 1.0f;
		if(attr().isLegionary() && safe_cast<UnitLegionary*>(this)->formationUnit_.unitMove()){
			forwardVelocityFactor = safe_cast<UnitLegionary*>(this)->formationUnit_.ptVelocity() / (parameters().findByType(ParameterType::VELOCITY, attr().forwardVelocity) + 1.e-5f);
			if(forwardVelocityFactor < FLT_EPS)
				forwardVelocityFactor = 0.0f;
		}
		chainControllers_.quant(forwardVelocityFactor);
	}

	modelLogicUpdated_ = false; // у большинства объектов есть анимация, которая может изменить координаты
}

void UnitReal::explode()
{
	if(!attr().leavingItems.empty()){
		if(attr().leavingItems.size() == 1){
			UnitBase* item = universe()->worldPlayer()->buildUnit(attr().leavingItems.front());
			item->setPose(pose(), true);
		}
		else if(attr().leavingItemsRandom){
			UnitBase* item = universe()->worldPlayer()->buildUnit(attr().leavingItems[logicRND(attr().leavingItems.size())]);
			item->setPose(pose(), true);
		}
		else{
			float radiusMax = 0;
			AttributeItemReferences::const_iterator i;
			FOR_EACH(attr().leavingItems, i)
				if(radiusMax < (*i)->radius())
					radiusMax = (*i)->radius();
			float angle = 0;
			float deltaAngle = 2.0f * M_PI / attr().leavingItems.size();
			FOR_EACH(attr().leavingItems, i){
				UnitBase* item = universe()->worldPlayer()->buildUnit(*i);
				Se3f poseShifted = pose();
				poseShifted.trans() += Vect3f(cosf(angle), sinf(angle), 0)*radiusMax;
				item->setPose(poseShifted, true);
				angle += deltaAngle;
			}
		}
	}

	if(model() && (deathAttr().explodeReference->enableExplode || deathAttr().explodeReference->animatedDeath || deathAttr().explodeReference->enableRagDoll) 
		&& (!attr().isBuilding() || (safe_cast<UnitBuilding*>(this)->constructionProgress() > 0.9f)))
		startState(StateDeath::instance());
	else
		if(isExplosionSourcesEnabled())
			createExplosionSources(position2D());
}

void UnitReal::changeUnitOwner(Player* playerIn)
{
	UnitBase::changeUnitOwner(playerIn);

	updateSkinColor();
}

//---------------------------------

DockingController::DockingController()
{
	dock_ = 0;
	tileIndex_ = 0;
}

void DockingController::set(UnitActing* dock, int nodeIndex)
{
	dock_ = dock;
	tileIndex_ = max(nodeIndex, 0);
}

void DockingController::clear()
{
	dock_ = 0;
}

const Se3f& DockingController::pose() const
{
	return dock_->modelLogicNodePosition(tileIndex_);
}

void DockingController::serialize(Archive& ar)
{
	ar.serialize(dock_, "dock", 0);
	ar.serialize(tileIndex_, "tileIndex", 0);
}

bool UnitReal::checkInPathTracking(const UnitBase* tracker) const
{
	if(tracker->attr().isSquad())
		return attr().enablePathTracking;

	if(!tracker->rigidBody())
		return false;

	if(tracker->attr().isActing()){
		if(!safe_cast<const UnitActing*>(tracker)->rigidBody()->flyingMode())
			return attr().enablePathTracking;
	}
	return false;
}

int UnitReal::modelLogicNodeIndex(const char* node_name) const
{
	if(modelLogic_)
		return modelLogic_->FindNode(node_name);

	return -1;
}

const Se3f& UnitReal::modelLogicNodePosition(int node_index) const
{
	if(modelLogic_ && node_index >= 0){
		if(!modelLogicUpdated_ && MT_IS_LOGIC()){
			modelLogicUpdated_ = true;
			modelLogic_->Update();
		}
		return modelLogic_->GetNodePosition(node_index);
	}

	return pose();
}

bool UnitReal::modelLogicNodeOffset(int node_index, Mats& offset, int& parent_node) const
{
	if(modelLogic_){
		if(!modelLogicUpdated_ && MT_IS_LOGIC()){
			modelLogicUpdated_ = true;
			modelLogic_->Update();
		}
		modelLogic_->GetNodeOffset(node_index, offset, parent_node);
		return true;
	}

	return false;
}

void UnitReal::setModelLogicNodeTransform(int node_index, const Se3f& pos)
{
	MTL();
	xassert(node_index != -1);

	if(modelLogic_){
		modelLogicUpdated_ = false;
		modelLogic_->SetUserTransform(node_index, pos);
	}
}

bool UnitReal::isModelLogicNodeVisible(int node_index) const
{
	xassert(node_index != -1);

	if(modelLogic_)
		return modelLogic_->GetVisibilityTrack(node_index);

	return false;
}

void UnitReal::updateSkinColor() 
{ 
	if(model()) 
		player()->setModelSkinColor(model());
}

const struct UI_ShowModeSprite* UnitReal::getSelectSprite() const
{
	return attr().getSelectSprite(AttributeBase::UI_STATE_TYPE_NORMAL);
}

int UnitReal::modelNodeIndex(const char* node_name) const
{
	if(model_)
		return model_->FindNode(node_name);

	return -1;
}

void UnitReal::setModelNodeTransform(int node_index, const Se3f& pos)
{
	xassert(node_index != -1);

	if(model_)
		streamLogicCommand.set(fCommandSetUserTransform, model_) << node_index << pos;
}

const AnimationChain* UnitReal::findChain(ChainID chainID, MovementState mstate, int counter, WeaponAnimationType weapon)
{
	const AnimationChain* chain = attr().animationChain(chainID, counter, abnormalStateType(), mstate, weapon);
	if(!chain && weapon)
		chain = attr().animationChain(chainID, counter, abnormalStateType(), mstate);
	return chain;
}

void UnitReal::setAdditionalChain(ChainID chainID, int counter, WeaponAnimationType weapon)
{
	MovementState mstate = getMovementState();
	setChain(findChain(chainID, mstate, counter, weapon), mstate, true);
}

void UnitReal::setAdditionalChainWithTimer(ChainID chainID, int chainTime, int counter)
{
	chainDelayTimer.start(chainTime);
	setAdditionalChain(chainID, counter);
}

void UnitReal::setAdditionalChainByFactor(ChainID chainID, float factor)
{
	MovementState mstate = getMovementState();
	setChain(attr().animationChainByFactor(chainID, factor, abnormalStateType(), mstate), mstate, true);
}

void UnitReal::setAdditionalChainByHealth(ChainID chainID)
{
	setAdditionalChainByFactor(chainID, 1.0f - health());
}

void UnitReal::setChain(ChainID chainID, int counter, WeaponAnimationType weapon)
{
	MovementState mstate = getMovementState();
	setChain(findChain(chainID, mstate, counter, weapon), mstate);
}

void UnitReal::setChainWithTimer(ChainID chainID, int chainTime, int counter)
{
	MovementState mstate = getMovementState();
	chainControllers_.setChain(attr().animationChain(chainID, counter, abnormalStateType(), mstate), mstate);
	chainDelayTimer.start(chainTime);
}

void UnitReal::setChainByFactor(ChainID chainID, float factor)
{
	MovementState mstate = getMovementState();
	setChain(attr().animationChainByFactor(chainID, factor, abnormalStateType(), mstate), mstate);
}

void UnitReal::setChainByHealth(ChainID chainID)
{
	setChainByFactor(chainID, 1.0f - health());
}

bool UnitReal::setChainByHealthExcludeGroups(ChainID chainID, vector<bool>& animationGroups)
{
	MovementState mstate = getMovementState();
	mainChain_ = attr().animationChainByFactor(chainID, 1.0f - health(), abnormalStateType(), mstate);
	modelLogicUpdated_ = false;
	return chainControllers_.setChainExcludeGroups(mainChain_, animationGroups, mstate);
}

void UnitReal::setChainByHealthWithTimer(ChainID chainID, int chainTime)
{
	chainDelayTimer.start(chainTime);
	setChainByHealth(chainID);
}

void UnitReal::setChainByHealthTransition(MovementState stateFrom, MovementState stateTo, int chainTime)
{
	setChain(attr().animationChainTransition(1.0f - health(), abnormalStateType(), stateFrom, stateTo), stateFrom | stateTo);
	chainDelayTimer.start(chainTime);
}

void UnitReal::setChain(const AnimationChain* chain, MovementState mstate, bool additionalChain)
{
	if(!additionalChain)
		mainChain_ = chain;
	chainControllers_.setChain(chain, mstate, additionalChain);
	modelLogicUpdated_ = false;
}

const AnimationChain* UnitReal::getChain(int animationGroup) const
{
	return chainControllers_.getChain(animationGroup);
}

bool UnitReal::isChainFinished(int animationGroup)	const 
{
	return chainControllers_.isChainFinished(animationGroup);
}

void UnitReal::stopChains()	
{
	chainControllers_.stop();
}

void UnitReal::stopCurrentChain()
{
	if(mainChain_)
		chainControllers_.stopChain(mainChain_);
}

bool UnitReal::animationChainEffectMode()
{
	if(mainChain_)
		return mainChain_->stopPermanentEffects;
	return false;
}

void UnitReal::stopAdditionalChain(ChainID chainID, int counter)
{
	const AnimationChain* chain = attr().animationChain(chainID, counter, abnormalStateType(), getMovementState());
	if(chain)
		chainControllers_.stopChain(chain);
}

void UnitReal::showEditor()
{
	__super::showEditor();

	if(!auxiliary() && editorVisual().isVisible(UNIVERSE_OBJECT_SOURCE)){
		if(selected())
			editorVisual().drawRadius(position(), radius(), EditorVisual::RADIUS_OBJECT, selected());
		else
			editorVisual().drawImpassabilityRadius(*this);
		if(strlen(label()))
			editorVisual().drawText(position(), label(), EditorVisual::TEXT_LABEL);
	}
}

bool UnitReal::isSuspendCommand(CommandID commandID)
{
	switch(commandID) {
	case COMMAND_ID_ATTACK:
	case COMMAND_ID_POINT:
	case COMMAND_ID_OBJECT:
		return true;
	}

	return false;
}

void UnitReal::setCorpse()
{
	alive_ = false;

	stopAllEffects();

	setColor(defaultColor());
	stopChains();

	model()->clearAttribute(ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE);
	
	startChainTimer(deathAttr().explodeReference->liveTime);
		
	if(deathAttr().explodeReference->animatedDeath){
		setChainByHealth(CHAIN_DEATH);
		if(isExplosionSourcesEnabled())
			createExplosionSources(position2D());
		if(deathAttr().explodeReference->enableExplode)
			universe()->crashSystem->addCrashModel(deathAttr(), model(), position(), lastContactPoint_, lastContactWeight_, 
			GlobalAttributes::instance().debrisLyingTime, UnitBase::rigidBody()->velocity());
	}
	startEffect(&deathAttr().effectAttrFly);
}

bool UnitReal::corpseQuant()
{
	return chainDelayTimer.time() < deathAttr().explodeReference->liveTime - 100;
}

void UnitReal::killCorpse()
{
	__super::Kill();

	if(model())
		streamLogicCommand.set(fCommandSetIgnored, model()) << true;
}

bool UnitReal::canPlaiceInOnePoint() { return rigidBody()->prm().hoverMode || rigidBody()->prm().alwaysMoving ;}

void fCommandSetFurPhase(XBuffer& stream)
{
	cObject3dx* cur;
	stream.read(cur);
	float phase;
	stream.read(phase);
	cur->SetFurScalePhase(phase);
	cur->SetFurAlphaPhase(phase);
}

void UnitReal::setFurPhase(float phase)
{
	streamLogicCommand.set(fCommandSetFurPhase) << model() << phase;
}

void UnitReal::setRiseFur(int time)
{
	furTimer_.start(time);
	riseFur_ = !riseFur_;
}

float UnitReal::noiseRadius() const
{
	if(inTransport())
		return 0.f;

	float r = __super::noiseRadius();
	if(mainChain_)
		r *= mainChain_->noiseRadiusFactor;

	return r;
}
