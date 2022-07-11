#include "stdafx.h"
#include "AI\PFTrap.h"
#include "normalMap.h"
#include "PositionGeneratorCircle.h"
#include "WhellController.h"
#include "RigidBodyCar.h"
#include "FormationController.h"
#include "GlobalAttributes.h"

///////////////////////////////////////////////////////////////

REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_UNIT, RigidBodyUnit)

///////////////////////////////////////////////////////////////
//
//    class RigidBodyUnitBase
//
///////////////////////////////////////////////////////////////

RigidBodyUnitBase::RigidBodyUnitBase() :
	groundZ_(0.0f),
	flyDownMode_(false),
	flyDownTime_(1000),
	flyDownSpeed_(0.0f),
	flyingHeightCurrent_(0.0f),
	waterLevelPoint_(0.0f),
	onDeepWater_(false),
	deepWaterChanged_(false),
	isUnderWaterSilouette_(false),
	unmovable_(false),
	isBoxMode_(false),
	animatedRise_(false),
	fallFromHill_(true),
	whellController_(0),
	car(0),
	prevVelocity_(Vect3f::ZERO),
	ptVelocity_(0.0f),
	forwardVelocity_(0.0f),
	moveSinkVelocity_(0.0f),
	standSinkVelocity_(0.0f),
	ptAdditionalRot_(QuatF::ID),
	groundRot_(QuatF::ID),
	flyingHeightDeltaFactorMax_(0.0f),
	flyingHeightDelta_(1.0f),
	flyingHeightDeltaPhase_(0.0f),
	flyingHeightDeltaOmega_(0.0f),
	impassability_(0),
	fieldFlag_(0),
	environmentDestruction_(0),
	canSink_(false),
	isSinking_(false),
	isSurfacing_(false)
{
	setRigidBodyType(RIGID_BODY_UNIT);
}

RigidBodyUnitBase::~RigidBodyUnitBase()
{
	if(whellController_){
		delete whellController_;
		whellController_ = 0;
	}
	if(car){
		car->detach();
		delete car;
		car = 0;
	}
}

void RigidBodyUnitBase::build(const RigidBodyPrm& prmIn, const Vect3f& center, const Vect3f& extent, float mass)
{
	__super::build(prmIn, center, extent, mass);

	colliding_ = 0;

	setColor(Color4c::CYAN);

	flyingModeEnabled_ = false;
	flyingMode_ = !prm().hoverMode;
	setFlyingHeight(prm().flying_height);
}

void RigidBodyUnitBase::initPose(const Se3f& poseNew)
{
	setPose(poseNew);
	if(flyDownMode() || isBoxMode()){
		posePrev_ = pose();
		return;
	}
	checkGround();
	QuatF groundQuat = placeToGround(groundZ_);
	if(flyingMode()){
		setPoseZ(groundZ_ + flyingHeightCurrentWithDelta());
		awake();
	}else{
		colliding_ = onWater() ? WATER_COLLIDING : GROUND_COLLIDING;
		setPoseZ(groundZ_ + flyingHeightCurrentWithDelta());
	}
	if(car)
		car->setPose(pose());
	posePrev_ = pose();
}

void RigidBodyUnitBase::setPose(const Se3f& pose)
{
	__super::setPose(pose);

	if(flyingMode())
		groundZ_ = position().z - flyingHeightCurrentWithDelta();
	else
		groundZ_ = position().z;

	setAngle(computeAngleZ());
}

bool RigidBodyUnitBase::evolve(float dt)
{
	start_timer_auto();
	if(isBoxMode()){
		if(!asleep() || unmovable()){
			if(!isFrozen())
				isUnderWaterSilouette_ = onWater() && (environment->water()->GetZFast(position().xi(), position().yi()) - position().z) > 2.0f * extent().z;
			RigidBodyBox::evolve(dt);
			return true;
		}
		disableBoxMode();
		if(!animatedRise_){
			enableWaterAnalysis();
			setFlyingModeEnabled(prm().flyingMode);
		}
		return false;
	}
	RigidBodyBase::evolve(dt);
	posePrev_ = pose();
	if(!prm().flyingMode){
		checkGround();
		bool isSinkingPrev = isSinking();
		if(onWater()){
			if(canSink_)
				isSinking_ = true;
			if(waterWeight() && !isUnderEditor()) {
				Vect2f waterVelocity(environment->water()->GetVelocity(pose().trans().xi(), pose().trans().yi()));
				waterVelocity *= waterWeight();
				if(waterVelocity.norm2() > 1e-5f){
					addPathTrackingVelocity(Vect3f(waterVelocity, 0.0f));
					awake();
				}
			}
			if(asleep() && !isFrozen()){
				if(fabsf(environment->water()->GetZFast(position().xi(), position().yi()) - waterLevelPoint_) > 0.1f)
					awake();
			}
		}else
			isSinking_ = false;
		if(isSinking() != isSinkingPrev)
			isSurfacing_ = true;
	}else{
		onLowWater_ = false;
		onWater_ = false;
		onIce_ = false;
		unFreeze();
	}
	if((!unmovable() && !isFrozen() && !asleep()) || car){
		colliding_ = 0;
		QuatF poseQuat(QuatF::ID);
		if(flyingMode() && !prm().flying_height_relative && !flyDownMode()){
			groundZ_ = 0;
			setPoseZ(flyingHeight());
		}else{
			float groundZNew = 0;
			poseQuat = placeToGround(groundZNew);
			if(isSinking()){
				flyingHeightCurrent_ -= isEq(ptVelocity_, 0.0f) ? standSinkVelocity_ : moveSinkVelocity_;
				flyingHeightCurrent_ = clamp(flyingHeightCurrent_, -(centreOfGravityLocal().z + extent().z), 0.0f);
			}
			if(isSurfacing_ && !isEq(ptVelocity_, 0.0f)){
				flyingHeightCurrent_ += standSinkVelocity_;
				if(flyingHeightCurrent_ > 0.0f){
					isSurfacing_ = false;
					flyingHeightCurrent_ = 0.0f;
				}
			}
			if(car){
				car->setLavaLevel(-flyingHeightCurrent_);
				setPose(car->pose());
				car->updateGraphicNodeOfset();
				return true;
			}
			if(flyDownMode()){
				if(F2DW(flyDownSpeed_) == 0)
					flyDownMode_ = false;
				else{
					flyingHeightCurrent_ -= flyDownSpeed_;
					float flyingHeightMax(flyingMode() ? (prm().flying_height_relative ? flyingHeight() : flyingHeight() - groundZ_) : 0.0f);
					if((flyingHeightCurrent() > flyingHeightMax) == (flyDownSpeed_ < 0.0f)){
						setFlyingHeightCurrent(flyingHeightMax);
						flyDownSpeed_ = 0.0f;
					}
				}
			}
			if(!flyDownMode() && waterAnalysis() && deepWaterChanged()){
				groundZ_ = groundZNew;
				setFlyingHeightCurrent(position().z - groundZ_);
			}else{
				if(whellController_)
					groundZ_ = groundZNew;
				else
					groundZ_ = groundZ_ * (1 - prm().position_tau) + groundZNew * prm().position_tau;

				computeFlyingHeightDelta(dt);

				setPoseZ(groundZ_ + flyingHeightCurrentWithDelta());
			}
			if(flyingHeightCurrent() > FLT_EPS)
				colliding_ = 0;
			else
				colliding_ = onWater() ? WATER_COLLIDING : GROUND_COLLIDING;
		}
		QuatF poseQuatNew;
		poseQuatNew.slerp(groundRot_, poseQuat, prm().orientation_tau);
		poseQuat = groundRot_ = poseQuatNew;
		poseQuat.postmult(QuatF(angle(), Vect3f::K, false));
		if(flyingMode())
			poseQuat.postmult(ptAdditionalRot_);
		poseQuat.normalize();
		setOrientation(poseQuat);
	}
	return true;
}

void RigidBodyUnitBase::attach()
{ 
	if(car)
		car->attach();
	if(isBoxMode() && !isFrozen())
		__super::attach(); 
}

void RigidBodyUnitBase::detach()
{ 
	if(car)
		car->detach();
	if(isBoxMode() && !isFrozen())
		__super::detach(); 
}

bool RigidBodyUnitBase::groundCheck( int xc, int yc, int r ) const
{
	short xL=(xc-r)>>environment->water()->GetCoordShift();
	short xR=(xc+r)>>environment->water()->GetCoordShift();
	short yT=(yc-r)>>environment->water()->GetCoordShift();
	short yD=(yc+r)>>environment->water()->GetCoordShift();

	short x,y;
	int a=0;
	for(y=yT; y<=yD; y++){
		for(x=xL; x<=xR; x++){
			const cWater::OnePoint& op = environment->water()->Get(x,y);
			if(!op.z) 
				return false;
		}
	}
	return true;
}

int RigidBodyUnitBase::calcPassabilityFlag() const
{
	int flags = 0;

	if(prm().groundPass)
		flags |= PathFinder::GROUND_FLAG;

	if(prm().waterPass)
		flags |= PathFinder::WATER_FLAG;

	if(prm().fieldPass)
		flags |= PathFinder::FIELD_FLAG;
	else
		flags |= fieldFlag();

	flags |= environmentDestruction();

	return flags;
}

bool RigidBodyUnitBase::checkImpassability(const Vect2f& pos) const
{
	float r = maxRadius() + 2.0f;
	if(pos.x - r < 0.0f || pos.x + r > vMap.H_SIZE - 1.0f || 
		pos.y - r < 0.0f || pos.y + r > vMap.V_SIZE - 1.0f) 
		return false;

	return (prm().waterPass || !checkWater(pos.xi(), pos.yi(), 0))
		&& !pathFinder->checkImpassability(pos.xi(), pos.yi(), impassability(), calcPassabilityFlag())
		&& (prm().fieldPass || !pathFinder->checkField(pos.xi(), pos.yi(), round(radius()),
		PathFinder::FIELD_FLAG & ~fieldFlag()))
		&& (prm().groundPass || groundCheck(pos.xi(), pos.yi(), round(radius())));
}

void RigidBodyUnitBase::enableBoxMode() 
{
	if(isSinking())
		return;

	setFlyingModeEnabled(false);
	setFlyingHeightCurrent(0.0f);
	isBoxMode_ = true;
	computeUnitVelocity();
	awake();
}

void RigidBodyUnitBase::disableBoxMode()
{ 
	detach();
	isBoxMode_ = false;
	if(angularEvolve())
		setAngle(computeAngleZ());
	setAngularEvolve(!prm().moveVertical);
	groundZ_ = position().z;
	awake();
}

void RigidBodyUnitBase::setFlyingModeEnabled(bool mode, int time) 
{	
	if(flyingModeEnabled_ != mode){
		flyingModeEnabled_ = mode;
		if(flyingModeEnabled_){
			deepWaterChanged_ = false;
			colliding_ = 0;
		}
		if(flyingMode_)
			enableFlyDownModeByTime(time ? time : flyDownTime_);
	}
}

void RigidBodyUnitBase::setFlyingMode(bool mode)
{
	if(flyingMode_ != mode){
		flyingMode_ = mode;
		if(flyingModeEnabled_)
			enableFlyDownMode();
	}
}

void RigidBodyUnitBase::setFlyingHeightAndDelta(float flyingHeight, float delta, float period) 
{ 
	setFlyingHeight(flyingHeight); 
	flyingHeightDeltaFactorMax_ = delta;
	flyingHeightDeltaOmega_ = 2.0f * M_PI / period;
	computeFlyingHeightDelta(logicRNDfrnd(period));
}

void RigidBodyUnitBase::computeFlyingHeightDelta(float dt)
{
	flyingHeightDeltaPhase_ = cycleAngle(flyingHeightDeltaPhase_ + flyingHeightDeltaOmega_ * dt);
	flyingHeightDelta_ = 1.0f - flyingHeightDeltaFactorMax_ * sinf(flyingHeightDeltaPhase_);
}

void RigidBodyUnitBase::enableFlyDownModeByTime(int time) 
{ 
	flyDownMode_ = true;
	flyDownSpeed_ = flyingHeightCurrent();
	if(flyingMode())
		flyDownSpeed_ -= flyingHeight();
	flyDownSpeed_ *= logicTimePeriod / (float)time;
	if(fabsf(flyDownSpeed_) <= FLT_EPS){
		flyDownMode_ = false;
		setFlyingHeightCurrent(flyingMode() ? (prm().flying_height_relative ? flyingHeight() : flyingHeight() - groundZ_) : 0.0f);
	}
}

void RigidBodyUnitBase::setSinkParameters(bool canSink, float moveSinkVelocity, float standSinkVelocity)
{
	canSink_ = canSink;
	moveSinkVelocity_ = moveSinkVelocity;
	standSinkVelocity_ = standSinkVelocity;
}

void RigidBodyUnitBase::startSinking(float level)
{
	isSinking_ = true;
	flyingHeightCurrent_ = level;
}

void RigidBodyUnitBase::updateFlyDownSpeed()
{
	flyDownMode_ = true;
	float flyDownSpeedNew = flyingHeightCurrent();
	if(flyingMode())
		flyDownSpeedNew -= flyingHeight();
	flyDownSpeedNew *= logicTimePeriod / (float)flyDownTime_;
	if(flyDownSpeed_ * flyDownSpeedNew > -FLT_EPS)
		flyDownSpeed_ = flyDownSpeed_ > 0.0f ? max(flyDownSpeed_, flyDownSpeedNew) : min(flyDownSpeed_, flyDownSpeedNew);
	else
		flyDownSpeed_ = flyDownSpeedNew;
	if(fabsf(flyDownSpeed_) <= FLT_EPS){
		flyDownMode_ = false;
		setFlyingHeightCurrent(flyingMode() ? (prm().flying_height_relative ? flyingHeight() : flyingHeight() - groundZ_) : 0.0f);
	}
}

void RigidBodyUnitBase::enableWaterAnalysis() 
{ 
	setWaterAnalysis(prm().waterAnalysis);
	onDeepWater_ = false;
	deepWaterChanged_ = false;
	if(!flyingModeEnabled_ && !flyDownMode()){
		groundZ_ += flyingHeightCurrentWithDelta();
		setFlyingHeightCurrent(0.0f);
	}
}

void RigidBodyUnitBase::disableWaterAnalysis() 
{ 
	setWaterAnalysis(false);
	onDeepWater_ = false;
	deepWaterChanged_ = false;
	groundZ_ += flyingHeightCurrentWithDelta();
	setFlyingHeightCurrent(0.0f);
}

void RigidBodyUnitBase::setWhellController(const WheelDescriptorList& wheelList, cObject3dx* model, cObject3dx* modelLogic) 
{ 
	whellController_ = new WhellController(wheelList, model, modelLogic);
	if(!whellController_->valid()){
		delete whellController_;
		whellController_ = 0;
	}
}

void RigidBodyUnitBase::setRealSuspension(cObject3dx* model, const RigidBodyCarPrm& carPrm) 
{
	car = new RigidBodyCar(carPrm.size(), carPrm.turnInPosition);
	car->build(*carPrm.rigidBodyPrm, centreOfGravityLocal(), extent(), core()->mass());
	car->setModel(model);
	int n = carPrm.size();
	for(int i = 0; i < n; ++i){
		float wheelRadius = carPrm[i].radius;
		car->initWheel(i, new RigidBodyWheel(i, car, carPrm[i].displacement, 
			wheelRadius, carPrm[i].nodeGraphicsSuspension, carPrm[i].nodeGraphicsWheel, 
			car->prm().suspensionSpringConstant * core()->mass() * car->prm().gravity / (carPrm[i].suspentionMovement * n), 
			car->prm().suspensionDamping * core()->mass(), carPrm[i].wheelType,
			carPrm[i].suspentionMovement * wheelRadius, -carPrm[i].suspentionMovement * wheelRadius,
			car->prm().drivingForceLimit * wheelRadius * core()->mass() * car->prm().gravity / n, 
			prm().waterAnalysis, waterLevel()), 
			Vect2f(car->prm().wheelMass, 0.5f * car->prm().wheelMass * wheelRadius * wheelRadius));
	}
	car->setPose(pose());
	awake();
}

void RigidBodyUnitBase::serialize(Archive& ar)
{
	ar.serialize(isBoxMode_, "isBoxMode", 0);
	if(isBoxMode()){
		ar.serialize(awakeCounter_, "awakeCounter", 0);
		ar.serialize(core()->velocity(), "velocity", 0);
	}else{
		ar.serialize(flyDownMode_, "flyDownMode", 0);
		if(flyDownMode())
			ar.serialize(flyDownSpeed_, "flyDownSpeed", 0);
		ar.serialize(flyingHeightCurrent_, "flyingHeightCurrent", 0);
	}
	__super::serialize(ar);
	if(prm().flyingMode)
		ar.serialize(flyingModeEnabled_, "flying_mode", 0);
	ar.serialize(onDeepWater_, "onDeepWater", 0);
	if(!flyingModeEnabled_)
		ar.serialize(deepWaterChanged_, "deepWaterChanged", 0);
	if(!asleep())
		attach();
}

void RigidBodyUnitBase::show()
{
	__super::show();

	if(showDebugRigidBody.realSuspension && car)
		car->show();

	if(showDebugRigidBody.ground_colliding){
		XBuffer buf;
		buf < "colliding: " <= colliding();
		show_text(position(), buf, Color4c::YELLOW);
	}

	if(showDebugRigidBody.localFrame){
		Mat3f rotation(orientation());
		show_vector(position(), rotation.xcol(), Color4c::RED);
		show_vector(position(), rotation.ycol(), Color4c::BLUE);
		show_vector(position(), rotation.zcol(), Color4c::GREEN);
	}

	if(showDebugRigidBody.onLowWater){
		XBuffer buf;
		buf < "On: " < (onWater() ? "water " : "") < (onLowWater() ? "lowWater " : "") < (onIce() ? "ice " : "") < (isFrozen() ? "frozen " : "");
		show_text(position(), buf, Color4c::YELLOW);
	}
}

void RigidBodyUnitBase::computeUnitVelocity()
{
	core()->velocity().linear().set(0, ptVelocity(), 0);
	pose().xformVect(core()->velocity().linear());
}

const Vect3f& RigidBodyUnitBase::velocity()
{ 
	if(!isBoxMode())
		computeUnitVelocity();
	return core()->velocity().linear();
}

void RigidBodyUnitBase::setAngleAndVelocity(bool moveback, float angleNew, float dirAngle, float velocityNew, float dt)
{
	ptVelocity_ = velocityNew;
	
	if(car){
		car->update(ptVelocity_, angleNew, moveback, dt);
		return;
	}
	if(unmovable() || isFrozen() || isBoxMode())
		return;
    addPathTrackingAngle(angleNew * 0.5f);
	
	if(ptVelocity_ > FLT_EPS){
		if(moveback)
			ptVelocity_ = -ptVelocity_;
	
		Vect3f velocity(Mat2f(float(angle() + dirAngle)) * Vect2f(0.0f, ptVelocity_), 0.0f);
		if(onIce()){
			Vect3f transDir;
			orientation().xform(Vect3f::J, transDir);
			prevVelocity_ = (transDir.dot(prevVelocity_) * 5.0f) * transDir;
			velocity += prevVelocity_;
		}
		prevVelocity_ = velocity;
		addPathTrackingVelocity(prevVelocity_ * dt);
		RigidBodyBox::setPose(pose());
		setColor(Color4c::CYAN);
	}else
		prevVelocity_ = Vect3f::ZERO;
	
	addPathTrackingAngle(angleNew * 0.5f);
	
	if(whellController_)
		whellController_->quant(3.0f * angleNew, ptVelocity_);
}

void RigidBodyUnitBase::setDeepWater(bool deepWater)
{
	if(!canSink_ && !flyingModeEnabled_ && onDeepWater() != deepWater)
		deepWaterChanged_ = true;
	onDeepWater_ = deepWater;
}

float RigidBodyUnitBase::checkDeepWater(float soilZ, float waterZ)
{
	float minRelativeZ(waterZ - position().z);
	float maxRelativeZ(waterZ - soilZ);
	if(!flyingMode()) {
		if(canSink_ || prm().placeOnWaterSurface) {
			setDeepWater(maxRelativeZ > 0.0f);
			isUnderWaterSilouette_ = min(minRelativeZ, maxRelativeZ) > 2.0f * extent().z;
		}else{
			if(onDeepWater() || onIcePrev())
				setDeepWater(maxRelativeZ > extent().z);
			else
				setDeepWater(min(minRelativeZ, maxRelativeZ) > 1.8f * extent().z);
			if(waterAnalysis())
				isUnderWaterSilouette_ = min(minRelativeZ, maxRelativeZ) > 2.0f * extent().z;
			else
				isUnderWaterSilouette_ = onDeepWater();				
		}
	}else{
		setDeepWater(maxRelativeZ > 0.0f);
		isUnderWaterSilouette_ = false;
	}
	if(onDeepWater() && waterAnalysis())
		return max(soilZ, waterZ - waterLevel());
	return soilZ;
}

float RigidBodyUnitBase::analyzeAreaFast(const Vect2i& center, int r, Vect3f& normalNonNormalized)
{
	if(onIce()){
		setDeepWater(false);
		return analyzeWaterFast(center, r, normalNonNormalized);
	}
	if(onLowWater() || onWater() || prm().flyingMode && checkLowWater(position(), radius())){
		int Sz = 0;
		int Sxz = 0;
		int Syz = 0;
		int SWaterz = 0;
		int SWaterxz = 0;
		int SWateryz = 0;
		int delta = max(vMap.w2m(r), 1);
		int xc = clamp(vMap.w2m(center.x), delta, vMap.GH_SIZE - delta - 4);
		int yc = clamp(vMap.w2m(center.y), delta, vMap.GV_SIZE - delta - 4);
		WaterScan waterScan(xc - delta, yc - delta, 2);
		for(int y = -delta; y <= delta; ++y){
			waterScan.goNextY();
			for(int x = -delta; x <= delta; ++x){
				int z = vMap.getZGrid(x + xc, y + yc);
				Sz += z;
				Sxz += x*z;
				Syz += y*z;
				z = max(z, waterScan.goNextX());
				SWaterz += z;
				SWaterxz += x*z;
				SWateryz += y*z;
			}
		}
		float N = sqr(2.0f * delta + 1.0f);
		float t = 3.0f / (N * delta * (delta + 1.0f));
		float groundZNew = checkDeepWater((float)Sz / N, (float)SWaterz / N);
		if(onDeepWater() && waterAnalysis())
			normalNonNormalized.set(-SWaterxz * t, -SWateryz * t, 4.0f);
		else
			normalNonNormalized.set(-Sxz * t, -Syz * t, 4.0f);
		return groundZNew;
	}
	setDeepWater(false);
	return vMap.analyzeArea(center, r, normalNonNormalized);
}

QuatF RigidBodyUnitBase::placeToGround(float& groundZNew)
{
	isUnderWaterSilouette_ = false;
	Vect2i center(position().xi(), position().yi());
	waterLevelPoint_ = environment->water()->GetZFast(center.x, center.y);
	Vect3f avrNormal(Vect3f::K);
	bool pointAreaAnalize(prm().canEnablePointAnalyze && radius() < GlobalAttributes::instance().analyzeAreaRadius);
	if(whellController_){
		Se3f poseNew;
		poseNew.trans() = position();
		poseNew.rot().set(angle(), Vect3f::K, false);
		avrNormal = whellController_->placeToGround(poseNew, groundZNew, onIce());
		if(!onIce() && (onLowWater() || onWater())){
			if(pointAreaAnalize)
				groundZNew = checkDeepWater(groundZNew, waterLevelPoint_);
			else{
				Vect3f waterNormal(Vect3f::K);
				groundZNew = checkDeepWater(groundZNew, analyzeWaterFast(center, round(radius()), waterNormal));
				if(onDeepWater() && waterAnalysis())
					avrNormal = waterNormal;
			}
		}else
			setDeepWater(false);
	}else if(pointAreaAnalize){
		if(onIce()){
			setDeepWater(false);
			groundZNew = max(groundZNew, environment->water()->GetZ(center.x, center.y));
		}else{
			groundZNew = float(vMap.getApproxAlt(center.x, center.y));
			if(onLowWater() || onWater() || prm().flyingMode && checkLowWater(position(), radius()))
				groundZNew = checkDeepWater(groundZNew, waterLevelPoint_);
			else
				setDeepWater(false);
		}
	}else
		groundZNew = analyzeAreaFast(center, round(radius()), avrNormal);
	if(!prm().moveVertical && !flyingMode() && !flyDownMode()){
		if(pointAreaAnalize){
			if((onDeepWater() && waterAnalysis()))
				avrNormal = water->GetNormalFast(center.x, center.y);
			else if(!whellController_)
				avrNormal = normalMap->normalLinear(position().x, position().y);
		}
		avrNormal.normalize();
		if(!prm().hoverMode && fallFromHill_ && (avrNormal.z < 0.5f))
			enableBoxMode();
		if(fabsf(avrNormal.z) < 0.9999995f)
			return QuatF(acosf(avrNormal.z), Vect3f(-avrNormal.y, avrNormal.x, 0.0f));
	}
	return QuatF::ID;
}

///////////////////////////////////////////////////////////////
//
//    class RigidBodyUnit
//
///////////////////////////////////////////////////////////////

void RigidBodyUnit::setDownUnit(UnitActing* unit) 
{ 
	start_timer_auto();
	if(!unit->rigidBody()->isInList(this)) 
		downUnit_ = unit; 
}

void RigidBodyUnit::clearDownUnit() 
{ 
	if(downUnit_ != 0 && position2D().distance2(downUnit_->position2D()) > sqr(radius() + downUnit_->radius())) 
		downUnit_ = 0; 
}

const RigidBodyUnit* RigidBodyUnit::downUnit() const
{
	return downUnit_ ? downUnit_->rigidBody() : 0;
}


