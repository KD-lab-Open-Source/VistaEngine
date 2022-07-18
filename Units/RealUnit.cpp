#include "StdAfx.h"
#include "RealUnit.h"
#include "Universe.h"

#include "PFTrap.h"
#include "ClusterFind.h"
#include "RenderObjects.h"
#include "serialization.h"
#include "GlobalAttributes.h"
#include "GameOptions.h"
#include "IronBullet.h"
#include "UnitAttribute.h"
#include "..\Environment\Environment.h"
#include "IronBuilding.h"
#include "..\Physics\RigidBodyUnitRagDoll.h"
#include "..\physics\crash\CrashSystem.h"

#include "EditorVisual.h"

#pragma warning(disable: 4355)

UNIT_LINK_GET(UnitReal)

REGISTER_CLASS(UnitBase, UnitReal, "Юнит");
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_REAL, UnitReal)

BEGIN_ENUM_DESCRIPTOR(AutoAttackMode, "Режими атаки (без цели)")
REGISTER_ENUM(GOTO_AND_KILL, "Следовать до упора")
REGISTER_ENUM(RETURN_TO_POSITION, "Следовать до упора и возвращаться на позицию")
REGISTER_ENUM(STOP_AND_ATTACK, "Стоять и атаковать")
REGISTER_ENUM(STOP_NOT_ATTACK, "Стоять и не атаковать")
REGISTER_ENUM(PATROL, "Патрулировать")
REGISTER_ENUM(PATROL_WITH_STOPS, "Патрулировать с остановками")

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

BEGIN_ENUM_DESCRIPTOR(PatrolMovementMode, "Режими движения при патрулировании")
REGISTER_ENUM(PATROL_MOVE_RANDOM, "случайным образом в радиусе партулирования")
REGISTER_ENUM(PATROL_MOVE_CW, "по кругу по часовой стрелке")
REGISTER_ENUM(PATROL_MOVE_CCW, "по кругу против часовой стрелки")
END_ENUM_DESCRIPTOR(PatrolMovementMode)

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
	frozenCounter_ = 0;
	freezeAnimationCounter_ = 0;

	model_ = 0;
	modelLogic_ = 0;
	modelLogicUpdated_ = false;

	whellController = 0;
	
	gripNodeRelativePose_ = Se3f::ID;
	useGripNode_ = false;
	flyingReason_ = 0;

	setModel(attr().modelName.c_str());
	if(!model()){
		xassertStr(0 && "Не установлена или не найдена модель у юнита ", attr().libraryKey());
		Kill(); 
		return;
	}

	pathFindSucceeded_ = false;
	pathFindTarget_ = Vect2i::ZERO;

	staticReason_ = 0;
	staticReasonXY_ = 0;
	animationStopReason_ = 0;
	hideReason_ = 0;

	unitState = AUTO_MODE;

	isMoving_ = false;

	manualRunMode = false;
	manualUnitPosition = Vect3f::ZERO;

	forwardVelocityFactor_ = 1.0f;

	mainChain_ = 0;
	prevState_ = 0;
	desiredState_ = 0;
	posibleStates_ = 0;
	currentStateID_ = CHAIN_NONE;
	currentState_ = StateBase::instance();
	rotationDeath = 0;
}

UnitReal::~UnitReal()
{
	RELEASE(model_);
	RELEASE(modelLogic_);

	ShowCircleControllers::iterator sc_it;
	FOR_EACH(showCircleControllers_, sc_it)
		sc_it->second.release();
}

void UnitReal::serialize (Archive& ar) 
{
    __super::serialize(ar);

	float radiusIn = radius();
    ar.serialize(radiusIn, "radius", 0);
	if(ar.isInput() && fabs(radiusIn - radius()) > FLT_EPS)
		setRadius(radiusIn);

    ar.serialize(label_, "label", "Метка");

	if(universe()->userSave()){
		ar.serialize(wayPoints_, "wayPoints", 0);
		ar.serialize(dockingController_, "dockingController", 0);
		ar.serialize(useGripNode_, "useGripNode", 0);
		ar.serialize(unitState, "unitState", 0);
		ar.serialize(chainDelayTimer, "chainDelayTimer", 0);
		vector<int> chainIndexes;
		int mainChainIndex;
		if(ar.isOutput()){
			vector<ChainController>::iterator chainController;
			FOR_EACH(chainControllers_.chainControllers(), chainController)
				chainIndexes.push_back(attr().animationChainIndex(chainController->chain()));
			mainChainIndex = attr().animationChainIndex(mainChain_);
		} 
		int staticReason(staticReason_), staticReasonXY(staticReasonXY_), hideReason(hideReason_), flyingReason(flyingReason_);
		ChainID prevStateID = prevState_ ? prevState_->id() : CHAIN_NONE, desiredStateID = desiredState_ ? desiredState_->id() : CHAIN_NONE;
		ar.serialize(currentStateID_, "currentState", 0);
		ar.serialize(prevStateID, "prevState", 0);
		ar.serialize(desiredStateID, "desiredState", 0);
		if(rigidBody_->prm().flyingMode)
			ar.serialize(flyingReason, "flyingReason", 0);
		ar.serialize(staticReason, "staticReason", 0);
		ar.serialize(staticReasonXY, "staticReasonXY", 0);
		ar.serialize(hideReason, "hideReason", 0);
		ar.serialize(chainIndexes, "chainIndexes", 0);
		ar.serialize(mainChainIndex, "mainChainIndex", 0);
		ar.serialize(manualRunMode, "manualRunMode", 0);
		if(ar.isInput()){
			if(flyingReason)
				disableFlying(flyingReason);
			if(staticReason)
				makeStatic(staticReason);
			if(staticReasonXY)
				makeStaticXY(staticReasonXY);
			if(hideReason & HIDE_BY_PLACED_BUILDING){
				setCollisionGroup(attr().collisionGroup);
				hideReason &= ~HIDE_BY_PLACED_BUILDING;
			}
			if(hideReason)
				hide(hideReason, true);
			currentState_ = UnitAllPosibleStates::instance()->find(currentStateID_);
			if(prevStateID != CHAIN_NONE)
				prevState_ = UnitAllPosibleStates::instance()->find(prevStateID);
			if(desiredStateID != CHAIN_NONE)
				desiredState_ = UnitAllPosibleStates::instance()->find(desiredStateID);
			vector<int>::iterator chainIndex;
			FOR_EACH(chainIndexes, chainIndex)
				setChain(attr().animationChainByIndex(*chainIndex));
			mainChain_ = attr().animationChainByIndex(mainChainIndex);
			if(manualRunMode)
				goToRun();
		}
		ar.serialize(chainControllers_, "chainControllers", 0);
		if(currentState() == CHAIN_DEATH){
			alive_ = false;
			ar.serialize(rotationDeath, "rotationDeath", 0);
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

	setRadius(0);

	if(attr().showSilhouette && GlobalAttributes::instance().enableSilhouettes && GameOptions::instance().getBool(OPTION_SILHOUETTE) && !player()->isWorld()) {
		model()->SetAttr(ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE);
		model()->SetSilhouetteIndex(player()->playerID());
	}

	if(attr().hideByDistance)
		model()->SetAttr(ATTRUNKOBJ_HIDE_BY_DISTANCE);

	attr().modelShadow.setShadowType(model(), radius());

    updateSkinColor();

	CalcSilouetteHeuristic();

	if(!attr().wheelList.empty()){
		whellController = new WhellController(*this);
		if(whellController->init(attr().wheelList)){
			rigidBody()->setWhellController(whellController);
			whellController->addVelocity(0);
		}
	}
	
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

void UnitReal::setRadius(float radiusNew)
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

	if(model() && modelLogic() && !attr().rigidBodyModelPrm.empty()){
		rigidBody_ = RigidBodyBase::buildRigidBody(RIGID_BODY_UNIT_RAG_DOLL, attr().rigidBodyPrm, attr().boundBox.min, attr().boundBox.max, attr().mass);
		modelLogic_->Update();
		safe_cast<RigidBodyUnitRagDoll*>(rigidBody_)->setModel(model(), modelLogic(), attr().rigidBodyModelPrm);
	}else
		rigidBody_ = RigidBodyBase::buildRigidBody(attr().rigidBodyPrm->rigidBodyType, attr().rigidBodyPrm, attr().boundBox.min, attr().boundBox.max, attr().mass);

	rigidBody_->setRadius(radius());
	if(rigidBody_->isUnit()){
		rigidBody()->setOwnerUnit(this);
		rigidBody()->setEnablePathTracking(attr().enablePathTracking);
		rigidBody()->setPathTrackingAngle(attr().pathTrackingAngle /(180.0/M_PI));
		rigidBody()->setWaterWeight(attr().waterWeight_ / 100.0f);
		rigidBody()->setWaterLevel(attr().waterLevel);
		if(attr().enableRotationMode)
			rigidBody()->enableRotationMode();
		if(attr().flyingHeight)
			rigidBody()->setFlyingHeight(attr().flyingHeight);
		if(attr().chainFlyDownTime)
			rigidBody()->setFlyDownTime(attr().chainFlyDownTime);
		enableFlying(0);
	}
}

void UnitReal::MoveQuant()
{
	start_timer_auto();

	WayPointController();

	if(!dockingController_.attached()){
		int n = 0;
		if(rigidBody_->isMissile())
			n = round(logicPeriodSeconds*safe_cast<RigidBodyMissile*>(rigidBody_)->forwardVelocity()/rigidBody_->boundRadius()*2);
		if(n <= 1)
			rigidBody_->evolve(logicPeriodSeconds);
		else{
			bool hideSent = false;
			float dt = logicPeriodSeconds/n;
			for(int i = 0; i <= n; i++){
				rigidBody_->evolve(dt);
				testRealCollision();
				if(!alive() && !hideSent){
					hideSent = true;
					//streamLogicInterpolator.set(fHideByFactor, model()) << float(i + 2)/n;
				}
			}
		}
	}

	//log_var(attr().libraryKey());
	//log_var(rigidBody_->pose());

	setPose(rigidBody_->pose(), false);

	if(rigidBody_->onWater){
		if(environment->waterIsLava()){
			if(!rigidBody_->onIce && lavaEffect().isEnabled())
				setAbnormalState(lavaEffect(), 0);
		}
		else {
			if(!rigidBody_->onIce && waterEffect().isEnabled())
				setAbnormalState(waterEffect(), 0);
		}
	}
	else {
		if(!rigidBody_->onIce && earthEffect().isEnabled())
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

	isMoving_ = rigidBody_->isUnit() && rigidBody()->unitMove();
	
	if(!isFrozen() || !alive()){
		start_timer_auto();
		stateQuant();
		if(dead())
			return;
	}

	if(environment->dayChanged())
		dayQuant(attr().isActing() && safe_cast<const UnitActing*>(this)->isInvisible());

	if(rigidBody_->isUnit()){

		if(whellController && isMoving()) {
			whellController->setAngle(rigidBody()->getWhellAngle());
			whellController->addVelocity(rigidBody()->getWhellVelocity());
		}
		
		if(!rigidBody()->isAutoPTMode() || rigidBody()->isManualMode())
			stopToRun();

		if(rigidBody()->isUnderWaterSilouette){
			if(!rigidBody()->isUnderWaterSilouettePrev){
				streamLogicCommand.set(fCommandSetUnderwaterSiluet) << model() << true;
				rigidBody()->isUnderWaterSilouettePrev = true;
			}
			if(!isFrozen())
				setColor(player()->underwaterColor());
		}
		else if(rigidBody()->isUnderWaterSilouettePrev){
			streamLogicCommand.set(fCommandSetUnderwaterSiluet) << model() << false;
			rigidBody()->isUnderWaterSilouettePrev = false;
		}

		frozenCounter_--;
		freezeAnimationCounter_--;
	
		if(rigidBody()->isFrozen() && rigidBody()->onIce)
			makeFrozen();

		if(isFrozen()){
			makeStatic(STATIC_DUE_TO_FROZEN);
			disableAnimation(ANIMATION_FROZEN);
			if(frozenCounter_ > 0 && iceEffect().isEnabled())
				setAbnormalState(iceEffect(), 0);
		} else {
			if(getStaticReason() & STATIC_DUE_TO_FROZEN){
				makeDynamic(STATIC_DUE_TO_FROZEN);
				enableAnimation(ANIMATION_FROZEN);
				if(currentState() & ~(CHAIN_DEATH | CHAIN_FALL)){
					rigidBody()->disableBoxMode();
					rigidBody()->setWaterAnalysis(rigidBody()->prm().waterAnalysis);
					enableFlying(0);
				}
			}
		}
	}

	interpolationQuant();
}

void UnitReal::dayQuant(bool invisible)
{
	if(environment->isDay() || hiddenLogic() || invisible){
		stopAdditionalChain(CHAIN_NIGHT);
		model()->SetAttr(ATTR3DX_HIDE_LIGHTS);
	}else{
		setAdditionalChain(CHAIN_NIGHT);
		model()->ClearAttr(ATTR3DX_HIDE_LIGHTS);
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
		rigidBody_->initPose(poseClamped);
		UnitBase::setPose(rigidBody_->pose(), initPose);
		posePrev = rigidBody_->pose();
	}
	else{
		UnitBase::setPose(poseClamped, initPose);
	}
	stop_timer(1)

	modelLogicUpdated_ = false;
	if(modelLogic_)
		modelLogic_->SetPosition(pose_);

	if(initPose){
		initPoseTimer_.start(200);
		if(!currentStateID_)
			stateQuant();
		//if(model())
		//	model()->SetPosition(pose_);

		if(isConstructed() && (!attr().isProjectile() || !isDocked())){
			stopPermanentEffects();
			startPermanentEffects();
		}
		manualUnitPosition = poseIn.trans();
	}
	
	if(model())
		streamLogicInterpolator.set(fSe3fInterpolation, model()) << posePrev << pose_;
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

	rigidBody_->setPose(poseClamped);
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

	if(rigidBody_)
		rigidBody_->show();

	if(showDebugUnitInterface.debugString)
		show_text(position(), debugString_.c_str(), RED);

	if(showDebugUnitReal.parameters && !isDocked())
		parameters().showDebug(position(), GREEN);

	if(showDebugUnitReal.parametersMax)
		parametersMax().showDebug(position(), RED);

	if(showDebugUnitReal.waterDamage)
		waterEffect().damage().showDebug(position(), RED);

	if(showDebugUnitReal.lavaDamage)
		lavaEffect().damage().showDebug(position(), RED);

	if(showDebugUnitReal.iceDamage)
		iceEffect().damage().showDebug(position(), RED);

	if(showDebugUnitReal.earthDamage)
		earthEffect().damage().showDebug(position(), RED);

	if(showDebugUnitReal.positionValue){
		XBuffer msg(256, 1);
		msg.SetDigits(6);
		msg <= position();
		show_text(position(), msg, GREEN);
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
		msg < getEnumNameAlt(currentState());
		msg < ": " <= chainDelayTimer() < "\n";
		show_text(position() + Vect3f(0, 0, height()), msg, RED);
	}

	if(showDebugUnitReal.homePosition && manualUnitPosition.norm2() > FLT_EPS){
		Vect3f rr = To3D(Vect2f(manualUnitPosition));
		show_line(position(), rr, CYAN);
		show_vector(rr, 5.f, CYAN);
	}
}

void UnitReal::refreshAttribute()
{
	setRadius(radius());
	rigidBody_->initPose(pose());
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
}

void UnitReal::Kill()
{
	if(alive_){
		__super::Kill();
		if(model())
			streamLogicPostCommand.set(fCommandSetIgnored, model()) << true;
		if(currentState()){
			// @dilesoft
			try {
			currentState_->finish(this, false);
			} catch (...) {
			}
			currentStateID_ = CHAIN_NONE;
			currentState_ = 0;
		}
	}else if(!dead()){
		unitID_.unregisterUnit();
		unitID_.registerUnit(this);
	}

	if(whellController) {
		delete whellController;
		whellController = 0;
		if(rigidBody_->isUnit())
			rigidBody()->setWhellController(0);
	}
}

//-------------------------------------------------
void UnitReal::executeCommand(const UnitCommand& command)
{
	UnitInterface::executeCommand(command);

	switch(command.commandID()){

	case COMMAND_ID_POINT:
		wayPoints_.clear();
		addWayPoint(command.position());
		break;

	case COMMAND_ID_OBJECT:
		if(command.unit() && (isEnemy(command.unit()) || command.unit()->player()->isWorld())){
			if(command.commandID() == COMMAND_ID_OBJECT)
				wayPoints_.clear();
		}
		break;

	case COMMAND_ID_STOP: 
		stop();
		break;
	}
}

void UnitReal::WayPointController()
{
	start_timer_auto();

	if(rigidBody_->isUnit()){
		rigidBody()->way_points.clear();
		if(!wayPoints().empty()){
			if(rigidBody()->is_point_reached(Vect2i(wayPoints().front())))
				wayPoints_.erase(wayPoints_.begin());
			else 
				moveToPoint(wayPoints().front());
		}
	}
}

float UnitReal::getPathFinderDistance(const UnitBase* unit_) const
{
	// Если везде проходит - растояние по прямой.	
	if(rigidBody()->prm().groundPass == RigidBodyPrm::PASSABILITY &&
		rigidBody()->prm().waterPass == RigidBodyPrm::PASSABILITY &&
		rigidBody()->prm().impassabilityPass == RigidBodyPrm::PASSABILITY)
		return position2D().distance(unit_->position2D());

	
	PathFinder::PFTile flags = 0;
		
	if(rigidBody()->prm().groundPass == RigidBodyPrm::PASSABILITY) {
		flags |= PathFinder::GROUND_FLAG;
		flags |= PathFinder::ELEVATOR_FLAG;
		flags |= PathFinder::SECOND_MAP_FLAG;
	}

	if(rigidBody()->prm().waterPass == RigidBodyPrm::PASSABILITY)
		flags |= PathFinder::WATER_FLAG;

	if(rigidBody()->prm().impassabilityPass == RigidBodyPrm::PASSABILITY)
		flags |= PathFinder::IMPASSABILITY_FLAG;

	flags |= int(attr().environmentDestruction);

	vector<Vect2i> points_;
	bool pathFind_ = pathFinder->findPath(Vect2i(position2D()), Vect2i(unit_->position2D()), flags, points_, rigidBody()->onSecondMap, true);

	if(points_.size() < 2)
		return position2D().distance(unit_->position2D());

	float retDist = sqrtf(sqr(position2D().x - points_[0].x) + sqr(position2D().y - points_[0].y));
	for(int i =1; i< points_.size(); i++)
		retDist += sqrtf(sqr(points_[i].x - points_[i-1].x) + sqr(points_[i].y - points_[i-1].y));

	return retDist;
}

void UnitReal::moveToPoint(const Vect3f& v)
{
	start_timer_auto();

	if(rigidBody()->is_point_reached(Vect2i(v)))
		return;

	if(!attr().enablePathFind){
		rigidBody()->way_points.push_back(v);
		return;
	}

	if(rigidBody()->prm().groundPass == RigidBodyPrm::PASSABILITY &&
		rigidBody()->prm().waterPass == RigidBodyPrm::PASSABILITY &&
		rigidBody()->prm().impassabilityPass == RigidBodyPrm::PASSABILITY) {

		rigidBody()->way_points.push_back(v);
		return;
	}

	Vect2i target = v;
	if(pathFindTarget_ != target){
		pathFindSucceeded_ = false;
		pathFindTarget_ = target;
		recalcPathTimer_.stop();
	}
	if(!pathFindSucceeded_ || !recalcPathTimer_){

		PathFinder::PFTile flags = 0;
		
		if(rigidBody()->prm().groundPass == RigidBodyPrm::PASSABILITY) {
			flags |= PathFinder::GROUND_FLAG;
			flags |= PathFinder::ELEVATOR_FLAG;
			flags |= PathFinder::SECOND_MAP_FLAG;
		}

		if(rigidBody()->prm().waterPass == RigidBodyPrm::PASSABILITY)
			flags |= PathFinder::WATER_FLAG;

		if(rigidBody()->prm().impassabilityPass == RigidBodyPrm::PASSABILITY)
			flags |= PathFinder::IMPASSABILITY_FLAG;

		flags |= int(attr().environmentDestruction);

		pathFindSucceeded_ = true;
		bool pathFind_ = pathFinder->findPath(Vect2i(position2D()), pathFindTarget_, flags, pathFindList_, rigidBody()->onSecondMap, /*targetOnSecondMap*/ true);
		
		// Заплатка на случай когда WP в недоступной точке. Ее перекидываем на доступную.
		if(!pathFind_ && !wayPoints().empty() && v.eq(wayPoints().front())) {
			wayPoints()[0] = To3D(Vect2f(pathFindList_.back()));
			pathFindTarget_ = Vect2i(wayPoints()[0]);
		}

		recalcPathTimer_.start(2000);
	}

	if(pathFindSucceeded_ && !pathFindList_.empty()){
		vector<Vect2i>::iterator pi;
        for(pi=pathFindList_.begin(); pi!=pathFindList_.end();)
			if(pathFindList_.size() > 1 ? rigidBody()->is_point_reached_mid(*pi) : rigidBody()->is_point_reached(*pi))
				pi = pathFindList_.erase(pi);
			else {
				rigidBody()->way_points.push_back(To3D(*pi));
				pi++;
			}
	}
}

void UnitReal::graphQuant(float dt)
{
	__super::graphQuant(dt);

	ShowCircleControllers::iterator sc_it = showCircleControllers_.begin();
	while(sc_it != showCircleControllers_.end()){
		if(sc_it->second.isOld()){
			sc_it->second.release();
			sc_it = showCircleControllers_.erase(sc_it);
			continue;
		}
		sc_it->second.makeOld();
		++sc_it;
	}
}

void UnitReal::interpolationQuant()
{
	if(!animationStopReason_)
		chainControllers_.quant(rigidBody_->isUnit() && rigidBody()->unitMove() ? forwardVelocityFactor_ : 1.0f);

	if(whellController)
		whellController->interpolationQuant();

	modelLogicUpdated_ = false; // у большинства объектов есть анимация, которая может изменить координаты
}

void UnitReal::explode()
{
	if(!attr().leavingItems.empty()){
		if(attr().leavingItems.size() == 1){
			UnitBase* item = universe()->worldPlayer()->buildUnit(attr().leavingItems.front());
			item->setPose(pose(), true);
		}
		else{
			float radiusMax = 0;
			AttributeItemReferences::const_iterator i;
			FOR_EACH(attr().leavingItems, i)
				if(radiusMax < (*i)->radius())
					radiusMax = (*i)->radius();
			float angle = 0;
			float deltaAngle = 2*M_PI/attr().leavingItems.size();
			FOR_EACH(attr().leavingItems, i){
				UnitBase* item = universe()->worldPlayer()->buildUnit(*i);
				Se3f poseShifted = pose();
				poseShifted.trans() += Vect3f(cosf(angle), sinf(angle), 0)*radiusMax;
				item->setPose(poseShifted, true);
				angle += deltaAngle;
			}
		}
	}

	if(model() && (deathAttr().explodeReference->enableExplode || deathAttr().explodeReference->animatedDeath) 
		&& (!attr().isBuilding() || (safe_cast<UnitBuilding*>(this)->constructionProgress() > 0.9f)))
		startState(StateDeath::instance());
	else
		if(isExplosionSourcesEnabled())
			createExplosionSources(position2D());
}

void UnitReal::changeUnitOwner(Player* playerIn)
{
	stop();
	
	UnitBase::changeUnitOwner(playerIn);

	updateSkinColor();
}

bool UnitReal::isMoving() const 
{ 
	//xassert(rigidBody()->isBoxMode() || isDocked() || isMoving_ || (Vect2f(rigidBody()->pose().trans()).distance(rigidBody()->posePrev().trans()) < FLT_EPS));
	return isMoving_;	
}

bool UnitReal::addWayPoint(const Vect3f& p)
{ 
	if(unitState == TRIGGER_MODE) return false;

	rigidBody()->clearAutoPT(RigidBodyUnit::ACTION_PRIORITY_HIGH);
	rigidBody()->disableRotationMode();

	unitState = MOVE_MODE;
	makeDynamicXY(STATIC_DUE_TO_ATTACK);

	Vect3f p2 = clampWorldPosition(p, radius());
	wayPoints_.push_back(p2);
	manualUnitPosition = p2;
	
	return true;
}

void UnitReal::setWayPoint(const Vect3f& p)
{ 
	wayPoints_.clear();
	addWayPoint(p);
}

void UnitReal::stop()
{
	wayPoints_.clear();

	if(unitState == TRIGGER_MODE)
		return;

//	if(rigidBody_->isUnit())
//		rigidBody()->way_points.clear();
	unitState = AUTO_MODE;
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

bool UnitReal::nearUnit(const UnitBase* unit, float radiusMin)
{
	rigidBody()->setIgnoreUnit(unit);
	if(position2D().distance2(unit->position2D()) < sqr(radius() + unit->radius() + radiusMin)){
		rigidBody()->setIgnoreUnit(0);
		stop();
		return rot2Point(unit->position());
	}
	else 
		setWayPoint(unit->position());

	return false;
}

bool UnitReal::rot2Point(const Vect3f& point)
{
	if(!attr().rotToTarget)
		return true;

	return rigidBody()->rot2point(point);
}

void UnitReal::disableFlying(int flyingReason, int time)
{
	flyingReason_ |= flyingReason;
	rigidBody()->setFlyingMode(false, time);
}

void UnitReal::enableFlying(int flyingReason, int time)
{
	flyingReason_ &= ~flyingReason;
	if(!flyingReason_)
		rigidBody()->setFlyingMode(rigidBody()->prm().flyingMode, time);
}

void UnitReal::makeStatic(int staticReason) 
{ 
	staticReason_ |= staticReason; 
	// @dilesoft
	try {
		rigidBody_->makeStatic();
	} catch (...) {
	}
}

void UnitReal::makeStaticXY(int staticReason) 
{ 
	staticReasonXY_ |= staticReason; 
	rigidBody()->makeStaticXY();
}

void UnitReal::makeDynamic(int staticReason) 
{ 
	staticReason_ &= ~staticReason; 
	if(!staticReason_){
		rigidBody_->makeDynamic();
	}
}

void UnitReal::makeDynamicXY(int staticReason) 
{ 
	staticReasonXY_ &= ~staticReason; 
	if(!staticReasonXY_){
		rigidBody()->makeDynamicXY();
	}
}

void UnitReal::goToRun()
{
	if(!rigidBody()->isRunMode()) {
		rigidBody()->setRunMode();
//		setChain(CHAIN_GO_RUN, 1 - health());
//		makeStatic(STATIC_DUE_TO_ANIMATION);
//		chainDelayTimer.start(attr().chainGoToRunTime);
	}
}

void UnitReal::stopToRun()
{
	if(rigidBody()->isRunMode() && !manualRunMode) {
		rigidBody()->setRunMode(false);
//		setChain(CHAIN_STOP_RUN, 1 - health());
//		makeStatic(STATIC_DUE_TO_ANIMATION);
//		chainDelayTimer.start(attr().chainStopToRunTime);
	}
}

bool UnitReal::checkInPathTracking( const UnitBase* tracker ) const
{
	if(tracker->attr().isSquad())
		return attr().enablePathTracking;

	if(!tracker->rigidBody())
		return false;

	if(rigidBody_->isUnit() && tracker->rigidBody()->isUnit()) {

		if(rigidBody()->flyingMode() && safe_cast<RigidBodyUnit*>(tracker->rigidBody())->flyingMode()) {
		
			if(rigidBody_->prm().flying_down_without_way_points && !tracker->rigidBody()->prm().flying_down_without_way_points)
				return false;

			if(!rigidBody_->prm().flying_down_without_way_points && tracker->rigidBody()->prm().flying_down_without_way_points)
				return false;

			return attr().enablePathTracking;
		}

		if(!rigidBody()->flyingMode() && !safe_cast<RigidBodyUnit*>(tracker->rigidBody())->flyingMode())
			return attr().enablePathTracking;

		return false;
	}
	else if(rigidBody_->isUnit()) {

		if(!rigidBody()->flyingMode())
			return attr().enablePathTracking;

		return false;
	}
	else if(tracker->rigidBody()->isUnit()) {

		if(!safe_cast<RigidBodyUnit*>(tracker->rigidBody())->flyingMode())
			return attr().enablePathTracking;

		return false;
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
	if(modelLogic_){
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

const AnimationChain* UnitReal::setAdditionalChain(ChainID chainID, int counter)
{
	const AnimationChain* chain = attr().animationChain(chainID, counter, abnormalStateType(), getMovementState());
	chainControllers_.setChain(chain, true);
	modelLogicUpdated_ = false;
	return chain;
}

const AnimationChain* UnitReal::setAdditionalChainWithTimer(ChainID chainID, int chainTime, int counter)
{
	chainDelayTimer.start(chainTime);
	return setAdditionalChain(chainID, counter);
}

void UnitReal::setAdditionalChainByHealth(ChainID chainID)
{
	const AnimationChain* chain = attr().animationChainByFactor(chainID, 1.0f - health(), abnormalStateType(), getMovementState());
	chainControllers_.setChain(chain);
	modelLogicUpdated_ = false;
}

const AnimationChain* UnitReal::setChain(ChainID chainID, int counter)
{
	mainChain_ = attr().animationChain(chainID, counter, abnormalStateType(), getMovementState());
	chainControllers_.setChain(mainChain_);
	modelLogicUpdated_ = false;
	return mainChain_;
}

bool UnitReal::setChainExcludeGroups(ChainID chainID, int counter, vector<int>& animationGroups)
{
	mainChain_ = attr().animationChain(chainID, counter, abnormalStateType(), getMovementState());
	modelLogicUpdated_ = false;
	return chainControllers_.setChainExcludeGroups(mainChain_, animationGroups);
}

const AnimationChain* UnitReal::setChainWithTimer(ChainID chainID, int chainTime, int counter)
{
	mainChain_ = attr().animationChain(chainID, counter, abnormalStateType(), getMovementState());
	chainControllers_.setChain(mainChain_);
	chainDelayTimer.start(chainTime);
	modelLogicUpdated_ = false;
	return mainChain_;
}

void UnitReal::setChainByFactor(ChainID chainID, float factor)
{
	mainChain_ = attr().animationChainByFactor(chainID, factor, abnormalStateType(), getMovementState());
	chainControllers_.setChain(mainChain_);
	modelLogicUpdated_ = false;
}

void UnitReal::setChainByHealth(ChainID chainID)
{
	mainChain_ = attr().animationChainByFactor(chainID, 1.0f - health(), abnormalStateType(), getMovementState());
	chainControllers_.setChain(mainChain_);
	modelLogicUpdated_ = false;
}

bool UnitReal::setChainByFactorExcludeGroups(ChainID chainID, float factor, vector<int>& animationGroups)
{
	mainChain_ = attr().animationChainByFactor(chainID, factor, abnormalStateType(), getMovementState());
	modelLogicUpdated_ = false;
	return chainControllers_.setChainExcludeGroups(mainChain_, animationGroups);
}

void UnitReal::setChainByHealthWithTimer(ChainID chainID, int chainTime)
{
	mainChain_ = attr().animationChainByFactor(chainID, 1.0f - health(), abnormalStateType(), getMovementState());
	chainControllers_.setChain(mainChain_);
	chainDelayTimer.start(chainTime);
	modelLogicUpdated_ = false;
}

void UnitReal::setChainTransition(MovementState stateFrom, MovementState stateTo, int chainTime)
{
	mainChain_ = attr().animationChainTransition(1.0f - health(), abnormalStateType(), stateFrom, stateTo);
	chainControllers_.setChain(mainChain_);
	chainDelayTimer.start(chainTime);
	modelLogicUpdated_ = false;
}

void UnitReal::setChain(const AnimationChain* chain)
{
	mainChain_ = chain;
	chainControllers_.setChain(chain);
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

	if(model())
		hide(HIDE_BY_EDITOR, !editorVisual().isVisible(objectClass()));
	
	if(!auxiliary()){
		if(editorVisual().isVisible(objectClass())){
			if(selected())
				editorVisual().drawRadius(position(), radius(), EditorVisual::RADIUS_OBJECT, selected());
			else
				editorVisual().drawImpassabilityRadius(*this);
		}
		if(label()[0] != '\0'){
			editorVisual().drawText(position(), label(), EditorVisual::TEXT_LABEL);
		}
	}
}

MovementState UnitReal::getMovementState()
{
	MovementState state;

	if(!rigidBody_->isUnit())
		return state;

	// Выставляем признаки поверхностей.
	if(rigidBody()->onDeepWater)
		state |= MOVEMENT_STATE_ON_WATER;
	else if(rigidBody()->onLowWater)
		state |= MOVEMENT_STATE_ON_LOW_WATER;
	else
		state |= MOVEMENT_STATE_ON_GROUND;

	// Выставляем направления движения
	if(!rigidBody()->isManualMode() && rigidBody()->getRotationMode() == RigidBodyUnit::ROT_LEFT)
		state |= MOVEMENT_STATE_LEFT;
	else if(!rigidBody()->isManualMode() && rigidBody()->getRotationMode() == RigidBodyUnit::ROT_RIGHT)
		state |= MOVEMENT_STATE_RIGHT;
	else if(rigidBody()->ptDirection() == RigidBodyUnit::PT_DIR_FORWARD)
		state |= MOVEMENT_STATE_FORWARD;
	else if(rigidBody()->ptDirection() == RigidBodyUnit::PT_DIR_BACK)
		state |= MOVEMENT_STATE_BACKWARD;
	else if(rigidBody()->ptDirection() == RigidBodyUnit::PT_DIR_LEFT)
		state |= MOVEMENT_STATE_LEFT;
	else if(rigidBody()->ptDirection() == RigidBodyUnit::PT_DIR_RIGHT)
		state |= MOVEMENT_STATE_RIGHT;
	else
		state |= MOVEMENT_STATE_ALL_SIDES;

	return state;
}

void UnitReal::hide(int reason, bool hide)
{
	if(hide){
		if(!hideReason_)
			streamLogicCommand.set(fCommandSetIgnored, model()) << true;
		if((reason &~ HIDE_BY_FOW) && !(hideReason_ &~ HIDE_BY_FOW)){
			stopPermanentEffects();
			dayQuant(attr().isActing() && safe_cast<const UnitActing*>(this)->isInvisible());
		}
		hideReason_ |= reason;
	}
	else{
		if(hideReason_){
			if(isConstructed() && (!attr().isProjectile() || !isDocked()) 
				&& (reason &~ HIDE_BY_FOW) && (hideReason_ &~ HIDE_BY_FOW) 
				&& !((hideReason_ &~ reason) &~ HIDE_BY_FOW)){
				startPermanentEffects();
				dayQuant(attr().isActing() && safe_cast<const UnitActing*>(this)->isInvisible());
			}
			hideReason_ &= ~reason;
			if(!hideReason_)
				streamLogicCommand.set(fCommandSetIgnored, model()) << false;
		}
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

class CircIdEqu{
	int id_;
public:
	CircIdEqu(int id) : id_(id) {}
	bool operator()(const ShowCircleControllers::value_type& pr) const { return pr.first == id_; }
};

void UnitReal::drawCircle(int id, const Vect3f pos, float radius, const CircleEffect& circle_attr)
{
	MTG();
	if(!circle_attr.color.a)
		return;
	ShowCircleControllers::iterator it = find_if(showCircleControllers_.begin(), showCircleControllers_.end(), CircIdEqu(id));
	if(it ==  showCircleControllers_.end()){
		showCircleControllers_.push_back(make_pair(id, ShowCircleController(circle_attr)));
		it = showCircleControllers_.end()-1;
	}
	it->second.redraw(pos, radius);
}

void UnitReal::setCorpse()
{
	start_timer_auto();
	alive_ = false;

	stopAllEffects();

	setColor(defaultColor());
	stopChains();

	if(rigidBody_->isUnit())
		makeStaticXY(STATIC_DUE_TO_DEATH);
	else
		makeStatic(STATIC_DUE_TO_DEATH);

	if(attr().isLegionary() && ((deathAttr().explodeReference->animatedDeath && rigidBody()->onDeepWater) 
		|| !rigidBody()->colliding())){
		if(rigidBody()->prm().flying_down_without_way_points || rigidBody()->flyingMode()){
			rigidBody()->setGravityZ(-10.0f);
			rotationDeath = 1;
			rigidBody()->setWaterAnalysis(true);
		}else if(rigidBody()->onDeepWater){
			rigidBody()->setGravityZ(-10.0f);
			rigidBody()->setWaterAnalysis(false);
		}
		if(rigidBody()->prm().moveVertical || deathAttr().explodeReference->animatedDeath)
			rigidBody()->enableAngularEvolve(false);
		rigidBody()->enableBoxMode();
		setChain(CHAIN_DEATH);
		if(isDocked()){
			Vect3f velocity(0, rigidBody()->prm().forward_velocity_max, 0);
			dock()->pose().xformVect(velocity);
			rigidBody()->setVelocity(velocity);
		}
	}else{
		if(attr().isLegionary())
			rigidBody()->setFlyingMode(false);
		if(deathAttr().explodeReference->animatedDeath)
			setChain(CHAIN_DEATH);
	}

	model()->ClearAttr(ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE);
	
	startChainTimer(deathAttr().explodeReference->liveTime);
		
	if(deathAttr().explodeReference->animatedDeath){
		if(isExplosionSourcesEnabled())
			createExplosionSources(position2D());
		if(deathAttr().explodeReference->enableExplode)
			universe()->crashSystem->addCrashModel(deathAttr(), model(), lastContactPoint_, lastContactWeight_, 
			GlobalAttributes::instance().debrisLyingTime, UnitBase::rigidBody()->velocity());
	}
	startEffect(&deathAttr().effectAttrFly);
}

bool UnitReal::corpseQuant()
{
	start_timer_auto();

	int time(chainDelayTimer.get_start_time() - global_time());
	
	if(time > deathAttr().explodeReference->liveTime - 100)
		return false;

	if(!attr().isProjectile() && deathAttr().explodeReference->animatedDeath){
		if(!rigidBody_->colliding())
			rigidBody_->awake();
		if(!chainDelayTimer){
			if(deathAttr().explodeReference->alphaDisappear){
				float opacityNew(0.0002f * time);
				if(opacityNew < 1.0f)
					setOpacity(opacityNew);
			}
			else if(time < 3000) {
				makeStatic(STATIC_DUE_TO_DEATH);
				Se3f nextPose(pose());
				nextPose.trans().z -= height() / 20.0f;
				setPose(nextPose, false);
				rigidBody()->setPose(nextPose);
			}
		} else {
			stopEffect(&deathAttr().effectAttrFly);
			return true;
		}
	} else if(attr().isProjectile() || rigidBody_->colliding()) {
		stopEffect(&deathAttr().effectAttrFly);
		if(deathAttr().explodeReference->enableExplode)
			universe()->crashSystem->addCrashModel(deathAttr(), model(), lastContactPoint_, lastContactWeight_, 
			attr().isProjectile() ? GlobalAttributes::instance().debrisProjectileLyingTime : GlobalAttributes::instance().debrisLyingTime, UnitBase::rigidBody()->velocity());
		return true;
	} else if(rotationDeath > 0){
		xassert(rigidBody()->isBoxMode());
		rigidBody()->awake();
		rigidBody()->setGravityZ(-10.0f - rotationDeath);
		rigidBody()->setAngularVelocity(Vect3f(0.0f, 0.0f, 3.0f + rotationDeath / 50.0f));
		++rotationDeath;
	}
	return false;
}

void UnitReal::killCorpse()
{
	__super::Kill();

	if(model())
		streamLogicCommand.set(fCommandSetIgnored, model()) << true;
}
