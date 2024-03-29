#include "stdafx.h"
#include "FormationController.h"
#include "Universe.h"
#include "NormalMap.h"
#include "AI\PFTrap.h"
#include "Squad.h"
#include "GlobalAttributes.h"

#ifndef FOR_OTHER
#define FOR_OTHER(list, iterator) \
	for(iterator = (list).begin() + 1; iterator != (list).end(); ++iterator)
#endif

#ifndef FOR_EACH_ACTIVE
#define FOR_EACH_ACTIVE(list, iterator) \
	for(iterator = (list).begin(); iterator != (list).inactive(); ++iterator)
#endif

#ifndef FOR_EACH_INACTIVE
#define FOR_EACH_INACTIVE(list, iterator) \
	for(iterator = (list).inactive(); iterator != (list).end(); ++iterator)
#endif

#ifndef FOR_OTHER_ACTIVE
#define FOR_OTHER_ACTIVE(list, iterator) \
	for(iterator = (list).begin() + 1; iterator != (list).inactive(); ++iterator)
#endif

///////////////////////////////////////////////////////////////
//
//    class PossibleSector
//
///////////////////////////////////////////////////////////////

PossibleSector::PossibleSector(float left, float right)
	: left_(left)
	, right_(right)
	, shifted_(right < left)
{
	dassert(isLess(left_, M_PI) && isGreater(left_, -M_PI));
	dassert(isLess(right_, M_PI) && isGreater(right_, -M_PI));
}

///////////////////////////////////////////////////////////////

bool PossibleSector::checkInSector(const MovementDirection& direction) const
{
	if(shifted_)
		return isGreater(direction, left_) || isLess(direction, right_);
	return isGreater(direction, left_) && isLess(direction, right_);
}

///////////////////////////////////////////////////////////////

MovementDirection PossibleSector::getPossibleDirection(const MovementDirection& direction) const
{ 
	if(checkInSector(direction))
		return direction;

	float rightDir = cycleAngle(right_ - direction);
	float leftDir = cycleAngle(left_ - direction);
	if(min(rightDir, 2.0f * M_PI - rightDir) > min(leftDir, 2.0f * M_PI - leftDir))
		return left_;
	return right_;
}

///////////////////////////////////////////////////////////////
//
//    class Passability
//
///////////////////////////////////////////////////////////////

Passability::Passability(bool obstacleCheck) 
	: obstaclePassability(obstacleCheck ? -1 : scanningDistance)
	, terrainPassability(-1)
{
}

///////////////////////////////////////////////////////////////

Passability::operator bool()
{
	return (terrainPassability == -1 || terrainPassability == scanningDistance) 
		&& (obstaclePassability == -1 || obstaclePassability == scanningDistance);
}

///////////////////////////////////////////////////////////////

Passability::operator int() const
{
	return ((terrainPassability < 0 ? scanningDistance : terrainPassability) << 6) 
		| (obstaclePassability < 0 ? scanningDistance : obstaclePassability);
}

///////////////////////////////////////////////////////////////

const Passability& Passability::operator &= (const Passability&  passability)
{
	if(obstaclePassability == -1)
		obstaclePassability = passability.obstaclePassability;
	else
		obstaclePassability = min(obstaclePassability, passability.obstaclePassability);
	if(terrainPassability == -1)
		terrainPassability = passability.terrainPassability;
	else
		terrainPassability = min(terrainPassability, passability.terrainPassability);
	return *this;
}

///////////////////////////////////////////////////////////////

bool Passability::isFullyPassable() const
{
	return (terrainPassability < 0 || terrainPassability == scanningDistance) 
		&& (obstaclePassability < 0 || obstaclePassability == scanningDistance);
}

///////////////////////////////////////////////////////////////
//
//    class PossibleMovementDirection
//
///////////////////////////////////////////////////////////////

PossibleMovementDirection::PossibleMovementDirection(const MovementDirection& direction)
	: Passability()
	, direction_(direction)
{
}

///////////////////////////////////////////////////////////////

const PossibleMovementDirection& PossibleMovementDirection::operator = (const Passability&  passability)
{
	obstaclePassability = passability.obstaclePassability;
	terrainPassability = passability.terrainPassability;
	return *this;
}

///////////////////////////////////////////////////////////////
//
//    class PossibleMovementDirections
//
///////////////////////////////////////////////////////////////

PossibleMovementDirections::PossibleMovementDirections(const PossibleSector& sector, int linesNum)
{
	float step = sector.size() / float(linesNum);
	resize(linesNum, MovementDirection(sector.left()));
	for(int i = 0; i < linesNum; ++i)
		operator[](i).direction() += step * i;
}

///////////////////////////////////////////////////////////////

const PossibleMovementDirection& PossibleMovementDirections::getOptimalDirection(bool moveLeft) const
{
	return moveLeft ? *getOptimalLeftDirection() : *getOptimalRightDirection();
}

///////////////////////////////////////////////////////////////

bool PossibleMovementDirections::checkLeftBeterThenRight() const
{
	return (getOptimalLeftDirection() - begin() + size() / 2) < (getOptimalRightDirection() - rbegin() + size() / 2);
}

///////////////////////////////////////////////////////////////

PossibleMovementDirections::const_iterator PossibleMovementDirections::getOptimalLeftDirection() const
{
	const_iterator prev = begin();
	for(const_iterator it = begin() + 1; it != end(); ++it)
		if(it->isFullyPassable() && !prev->isFullyPassable())
			return it;
		else
			prev = it;
	return (front().isFullyPassable() && back().isFullyPassable()) ? begin() : end() - 1;
}

///////////////////////////////////////////////////////////////

PossibleMovementDirections::const_reverse_iterator PossibleMovementDirections::getOptimalRightDirection() const
{
	const_reverse_iterator prev = rbegin();
	for(const_reverse_iterator it = rbegin() + 1; it != rend(); ++it)
		if(it->isFullyPassable() && !prev->isFullyPassable())
			return it;
		else
			prev = it;
	return (front().isFullyPassable() && back().isFullyPassable()) ? rbegin() : rend() - 1;
}

///////////////////////////////////////////////////////////////
//
//    class FormationUnit
//
///////////////////////////////////////////////////////////////

FormationUnit::FormationUnit(RigidBodyUnit* rigidBody)
	: rigidBody_(rigidBody)
	, rotSpeed_(0.0f)
	, onImpassability_(false)
	, additionalHorizontalRot_(0.0f)
	, rotationSide_(ROT_LEFT)
	, movementMode_(MODE_WALK)
	, ignoreUnit_(0)
	, manualViewPoint(Vect2f::ZERO)
	, unmovable_(false)
	, manualMoving_(false)
	, manualPTLine(0.0f)
	, ptDirection_(PT_DIR_FORWARD)
	, curVerticalFactor_(1.0f)
	, ptPose_(MatX2f::ID)
	, ownerUnit_(0)
	, penetrationFound_(false)
	, rotationToPoint_(false)
	, pointToRotate_(Vect2f::ZERO)
	, ptVelocity_(0.0f)
	, formationIndex_(-1)
{
}

bool FormationUnit::personalWayPoint() const
{ 
	return !ownerUnit_->formationController_.wayPointEmpty();  
}

void FormationUnit::clearPersonalWayPoint() 
{ 
	ownerUnit_->formationController_.wayPointsClear();  
}

void FormationUnit::setPersonalWayPoint(const Vect2f& wayPoint)
{
	ownerUnit_->formationController_.setWayPoint(wayPoint);
}

float FormationUnit::rotate(const MovementDirection& moveDirection)
{
	float rotSpeed = (rigidBody_->onWater() ? ownerUnit().attr().angleSwimRotationMode : ownerUnit().attr().angleRotationMode) * (M_PI / 1800.0f);
	rotSpeed = clamp(moveDirection, -rotSpeed, rotSpeed);
	setRotationSpeed(fabsf(rotSpeed) > 0.01f ? rotSpeed : 0.0f);
	return rotSpeed;
}

bool FormationUnit::collidingObstacleList(const Vect2f& vehiclePosition, const Mat2f& vehicleOrientation, const vector<RigidBodyBase*>& obstacleList, bool firstStep) const
{
	vector<RigidBodyBase*>::const_iterator it;
	FOR_EACH(obstacleList, it){
		if(*it != rigidBody_ && (firstStep || !(*it)->isUnit() || isEq(safe_cast<RigidBodyUnit*>(*it)->ptVelocity(), 0.0f)) && (*it)->bodyIntersect(rigidBody_, vehiclePosition, vehicleOrientation))
			return true;
	}

	return false;
}

float FormationUnit::computeVerticalFactor(const Vect2f& position, const Vect2f& dir) const
{
	Vect3f terNormal = normalMap->normalLinear(position.x, position.y);
	return 1.0f + rigidBody_->prm().verticalFactor * dir.dot(terNormal);
}

float FormationUnit::computeTerrainVelocityFactor(bool moveback) const
{
	if(onImpassability())
		return 1.0f;

	float factor = ownerUnit().attr().velocityFactorsByTerrain[pathFinder->getTerrainType(rigidBody_->position().xi(), rigidBody_->position().yi())];

	if(!ownerUnit().attr().enableVerticalFactor)
		return factor;

	float ptAngleDelta = ptDirectionAngle();
	if(moveback)
		ptAngleDelta += M_PI;
	QuatF r(ptAngleDelta, Vect3f::K);
	r.premult(rigidBody_->orientation());
	Vect3f dir(Vect3f::J);
	r.xform(dir);
	dir.z = 0.0f;
	dir.normalize();
	return factor * computeVerticalFactor(rigidBody_->position2D(), dir);
}

MovementDirection FormationUnit::movingDirection(bool moveback)
{
	float ptAngleDelta = ptDirectionAngle();
	if(moveback)
		ptAngleDelta += M_PI;
	MovementDirection dir = rigidBody_->angle();
	dir += ptAngleDelta;
	return dir;
}

int FormationUnit::computePassabilityFactor(const Vect2f& position, const Vect2f& newPosition) const
{
	if(!rigidBody_->checkImpassability(position))
		return 100;

	int factor = 100;
	if(!manualMoving() || GlobalAttributes::instance().checkImpassabilityInDC){
		if(!rigidBody_->checkImpassability(newPosition))
			return 0;

		factor = 100 * ownerUnit().attr().velocityFactorsByTerrain[pathFinder->getTerrainType(newPosition.xi(), newPosition.yi())];
	}

	if(!ownerUnit().attr().enableVerticalFactor || (rigidBody_->onWater() && rigidBody_->waterAnalysis()))
		return factor;

	Vect2f dir(newPosition);
	dir -= position;
	dir.normalize(1.0f);
	return factor * computeVerticalFactor(position, dir);
}

void FormationUnit::ptRotationMode()
{
	Vect2f wpoint;
	if(manualMoving())
		wpoint = manualViewPoint;
	else
		wpoint = pointToRotate_;
	ptPose_.trans = rigidBody_->position2D();
	ptPose_.rot.set(rigidBody_->angle());
	Vect2f localWayPoint = ptPose().invXform(wpoint);
	float rotSpeed = (rigidBody_->onWater() ? ownerUnit().attr().angleSwimRotationMode : ownerUnit().attr().angleRotationMode) * (M_PI / 1800.0f);
	float angleNew = -atan2f(localWayPoint.x, localWayPoint.y);
	if(angleNew > 0.0f){
		if(angleNew < rotSpeed){
			disableRotationMode();
			rotSpeed = angleNew;
		}
	}else{
		rotSpeed *= -1.0f;
		if(angleNew > rotSpeed){
			disableRotationMode();
			rotSpeed = angleNew;
		}
	}
	setRotationSpeed(rotSpeed);
}

void FormationUnit::postQuant(bool moveback)
{
	if(!rigidBody_->unmovable() && !rigidBody_->isFrozen() && !rigidBody_->isBoxMode()){// && !rigidBody_->asleep()){
		rigidBody_->setColor(Color4c::BLUE);
		if(rotSpeed() > 0.0f)
			rotationSide_ = ROT_RIGHT;
		else
			rotationSide_ = ROT_LEFT;
		if(rigidBody_->flyingMode()){
			QuatF ptAdditionalRotNew(clamp(ptVelocity() / (rigidBody_->forwardVelocity() + 0.01f), -1.0f, 1.0f) * rigidBody_->prm().additionalForvardRot, moveback ? Vect3f::I : Vect3f::I_, false);
			ptAdditionalRotNew.postmult(QuatF(clamp(rotSpeed() / G2R(ownerUnit().attr().pathTrackingAngle), -1.0f, 1.0f) * additionalHorizontalRot_, Vect3f::J_, false));
			if(rigidBody_->prm().orientation_tau < 1.0f){
				QuatF poseQuatNew;
				poseQuatNew.slerp(rigidBody_->ptAdditionalRot_, ptAdditionalRotNew, rigidBody_->prm().orientation_tau);
				rigidBody_->ptAdditionalRot_ = poseQuatNew;
			}else
				rigidBody_->ptAdditionalRot_ = ptAdditionalRotNew;
		}
		rigidBody_->setAngleAndVelocity(moveback, rotSpeed(), ptDirectionAngle(), ptVelocity(), logicPeriodSeconds);
	}
}

bool FormationUnit::unitMove() const 
{
	return !isEq(ptVelocity(), 0.0f); 
}
bool FormationUnit::unitRotate() const 
{ 
	return !isEq(rotSpeed(), 0.0f); 
}

void FormationUnit::setOwnerUnit(UnitLegionary* unit) 
{ 
	rigidBody_->setForwardVelocity(rigidBody_->prm().forward_velocity_max);
	ownerUnit_ = unit; 
	rigidBody_->setFallFromHill(!ownerUnit().attr().isBuilding());
	rigidBody_->setWaterWeight(ownerUnit().attr().waterWeight_ * 0.01f);
	rigidBody_->setBoundCheck(ownerUnit().attr().ptBoundCheck);
	if(ownerUnit().attr().flyingHeight)
		rigidBody_->setFlyingHeightAndDelta(ownerUnit().attr().flyingHeight, ownerUnit().attr().flyingHeightDeltaFactor, ownerUnit().attr().flyingHeightDeltaPeriod);
	setAdditionalHorizontalRot(ownerUnit().attr().additionalHorizontalRot ? ownerUnit().attr().additionalHorizontalRot : rigidBody_->prm().additionalHorizontalRot);
	if(ownerUnit().attr().chainFlyDownTime)
		rigidBody_->setFlyDownTime(ownerUnit().attr().chainFlyDownTime);
}

void FormationUnit::initPose()
{
	ptPose_.trans = rigidBody_->position2D();
	ptPose_.rot.set(rigidBody_->angle() + ptDirectionAngle());
}

void FormationUnit::resolvePenetration(const Vect2f& position, float radius)
{
	if(unitMove())
		return;

	Vect2f point = rigidBody_->position2D() - position;
	if(point.norm2() < FLT_EPS)
		point = rigidBody_->angle() + ptDirectionAngle();
	point.normalize(radius + rigidBody_->radius());
	point += position;
	disableRotationMode();
	if(ownerUnit_->squad())
		ownerUnit_->squad()->formationController_.moveAction(point, FormationController::ACTION_PRIORITY_HIGH);
	rigidBody_->awake();
	setPenetrationFound(true);
}

///////////////////////////////////////////////////////////////
//
//    class ObstaclesFinder
//
///////////////////////////////////////////////////////////////

class ObstaclesFinder 
{
public:
	ObstaclesFinder(UnitLegionary* owner)
		: found(false)
		, owner_(owner) 
	{
	}
	
	bool checkPenetration(UnitActing* p)
	{
		if(owner_->squad() == p->getSquadPoint())
			return false;
		if(p->rigidBody()->flyingMode() && !p->rigidBody()->prm().hoverMode) 
			return false;
		if(!p->rigidBody()->bodyIntersect(owner_->rigidBody(), owner_->rigidBody()->position2D(), owner_->formationUnit_.ptPose().rot))
			return false;
		float height =  2.0f * p->rigidBody()->extent().z;
		float deltaZ = owner_->rigidBody()->flyingHeightCurrent() - p->rigidBody()->flyingHeightCurrent();
		if(fabsf(deltaZ) < height)
			return true;
		float deltaH = owner_->rigidBody()->flyingHeightCurrent() - owner_->rigidBody()->flyingHeight();
		if(deltaH > 0.0f && deltaZ > height && deltaZ - deltaH < height)
			return true;
		return false;
	}
	
	bool isDownUnit(UnitActing* p)
	{
		if(!isLess(owner_->rigidBody()->radius(), p->rigidBody()->radius(), 1.0e-3f))
			return false;
		if(isEq(p->rigidBody()->radius(), owner_->rigidBody()->radius(), 1.0e-3f)
		&& !isGreater(owner_->rigidBody()->flyingHeightCurrent(), p->rigidBody()->flyingHeightCurrent(), 1.0e-3f))
			return false;
		if(isEq(owner_->rigidBody()->flyingHeightCurrent(), p->rigidBody()->flyingHeightCurrent(), 1.0e-3f) 
		&& !owner_->isMoving() && p->isMoving())
			return false;
		if(!owner_->rigidBody()->downUnit())
			return true;
		if(isGreater(p->rigidBody()->flyingHeightCurrent(), owner_->rigidBody()->downUnit()->flyingHeightCurrent(), 1.0e-3f))
			return true;
		return false;
	}

	void operator () (UnitBase* p) 
	{
		if(owner_ == p || p->dead() || p->isDocked() || !p->attr().isActing())
			return;
		UnitActing* unit = safe_cast<UnitActing*>(p);
		if(checkPenetration(unit)){
			if(owner_->rigidBody()->prm().hoverMode){
				if(isDownUnit(unit))
					owner_->rigidBody()->setDownUnit(unit);
			}else if(owner_->position2D().distance2(p->position2D()) < sqr(p->radius())){
				owner_->formationUnit_.resolvePenetration(p->position2D(), p->radius());
				found = true;
			}
		}
	}

	bool found;

private:
	UnitLegionary* owner_;
};

///////////////////////////////////////////////////////////////

void FormationUnit::checkPenetration()
{
	if(!rigidBody_->unmovable() && !rigidBody_->isFrozen() && !rigidBody_->isBoxMode()){

		float flyingHeightPrev = 0.0f;
		if(rigidBody_->prm().hoverMode){
			flyingHeightPrev = rigidBody_->flyDownMode() ? rigidBody_->flyingHeight() : rigidBody_->flyingHeightCurrent();
			rigidBody_->clearDownUnit();
		}
		ObstaclesFinder of(&ownerUnit());
		universe()->unitGrid.Scan(Vect2i(rigidBody_->position().xi(), rigidBody_->position().yi()), int(rigidBody_->radius()), of);
		setPenetrationFound(of.found);
		if(rigidBody_->prm().hoverMode){
			if(rigidBody_->downUnit())
				rigidBody_->awake();
			rigidBody_->setFlyingMode(unitMove() || unitRotate() || rigidBody_->downUnit());
			if(rigidBody_->flyingMode() && !isEq(flyingHeightPrev, rigidBody_->flyingHeight()))
				rigidBody_->enableFlyDownMode();
			else if(rigidBody_->downUnit() && rigidBody_->flyDownMode())
				rigidBody_->updateFlyDownSpeed();
		}
	}
}

///////////////////////////////////////////////////////////////

void FormationUnit::quant()
{
	ptPose_.trans = rigidBody_->position2D();
	ptPose_.rot.set(rigidBody_->angle() + ptDirectionAngle());

	setRotationSpeed(0.0f);

	onImpassability_ = !rigidBody_->checkImpassability(rigidBody_->position2D());
	rigidBody_->setFallFromHill(!onImpassability());
	
	if(rigidBody_->isFrozen() || rigidBody_->isBoxMode() || unmovable()){
		if(!rigidBody_->unmovable() && !rigidBody_->isFrozen() && !rigidBody_->isBoxMode() 
		&& rigidBody_->asleep() && (rigidBody_->flyDownMode() || rigidBody_->isSinking() || rotationToPoint_))
			rigidBody_->awake();
		setPtVelocity(0.0f);
	}else{
		rigidBody_->awake();

		if(rotationToPoint_ || (manualMoving() && ownerUnit().attr().rotToTarget)){
			ptRotationMode();
			if(!manualMoving())
				setPtVelocity(0.0f);
		}

		if(manualMoving()){
			setPtVelocity(clamp(ptVelocity() - 0.75f * rigidBody_->prm().forward_acceleration * logicPeriodSeconds, 0.0f, rigidBody_->forwardVelocity()));
			if(manualPTLine < 0)
				manualPTLine += 0.4f;
			else if(manualPTLine > 0)
				manualPTLine -= 0.4f;
			manualPTLine = clamp(manualPTLine, -float(LINES_NUM), float(LINES_NUM));
		}
	}
}

void FormationUnit::show()
{
	if(showDebugRigidBody.showPathTracking_Lines){
		Vect3f forward(rigidBody_->angle(), 0);
		forward.scale(10.0f);
		show_line(rigidBody_->position(), rigidBody_->position() + Vect3f(forward, 0), Color4c::RED);
	}

	if(showDebugRigidBody.showPathTracking_Lines){
		Vect3f forward(MovementDirection(rigidBody_->angle() + ptDirectionAngle()), 0);
		forward.scale(10.0f);
		show_line(rigidBody_->position(), rigidBody_->position() + Vect3f(forward, 0), Color4c::GREEN);
	}

	if(showDebugRigidBody.downUnit && rigidBody_->downUnit())
		show_line(rigidBody_->position(), rigidBody_->downUnit()->position(), Color4c::RED);

	if(showDebugRigidBody.ptVelocityValue){
		XBuffer buf;
		buf.SetDigits(2);
		buf <= ptVelocity();
		show_text(rigidBody_->position(), buf, Color4c::MAGENTA);
	}
}

bool FormationUnit::isPointReached(const Vect2i& point)
{ 
	Vect2f p0 = rigidBody_->posePrev().trans();
	Vect2f p1 = rigidBody_->position2D();
	Vect2f axis = p1 - p0;
	float norm2 = axis.norm2();
	float dist2 = 0;
	if(norm2 < 0.1f){
		dist2 = p0.distance2(point);
	}
	else{
		Vect2f dir = point - p0;
		float dotDir = axis.dot(dir);
		if(dotDir < 0)
			dist2 = p0.distance2(point);
		else if(axis.dot(point - p1) > 0)
			dist2 = p1.distance2(point);
		else
			dist2 = (dir - axis*(dotDir/norm2)).norm2();
	}
	
	bool reached = dist2 < sqr(rigidBody_->prm().is_point_reached_radius_max);
	//if(reached)
	//	stopFunctorFirstStep = 0;
	return reached;
}

bool FormationUnit::isPointReachedMid(const Vect2i& point)
{ 
	bool reached = rigidBody_->position2D().distance2(point) < sqr(rigidBody_->prm().is_point_reached_radius_max);
	//if(reached)
	//	stopFunctorFirstStep = 0;
	return reached;
}

void FormationUnit::makeStatic() 
{ 
	unmovable_ = true;  
	setPtVelocity(0.0f);
}

void FormationUnit::setPenetrationFound(bool penetrationFound) 
{ 
	penetrationFound_ = penetrationFound; 
}

float FormationUnit::ptDirectionAngle() const 
{ 
	switch(ptDirection()){
	case PT_DIR_RIGHT:
		return M_PI_2;
		break;
	case PT_DIR_LEFT:
		return -M_PI_2;
		break;
	case PT_DIR_BACK:
		return M_PI;
		break;
	}
	return 0.0f;
}

void FormationUnit::manualTurn(float factor)
{
	manualPTLine += 0.5f * factor;
}

void FormationUnit::manualMove(float factor, bool backward)
{
	if(rigidBody_->isBoxMode() || unmovable())
		return;

	/*if(backward == moveback_)
		setPtVelocity(ptVelocity() + rigidBody_->prm().forward_acceleration * factor * logicPeriodSeconds);
	else{
		setPtVelocity(ptVelocity() - rigidBody_->prm().forward_acceleration * factor * logicPeriodSeconds);
		if(ptVelocity() < 0){
			moveback_ = backward;
			setPtVelocity(ptVelocity());
		}
	}*/
}

float FormationUnit::ptVelocity() const 
{ 
	return ptVelocity_; 
}

void FormationUnit::setPtVelocity(float ptVelocity) 
{ 
	ptVelocity_ = rigidBody_->prm().alwaysMoving ? max(ptVelocity, ownerUnit_->forwardVelocity(false)) * 0.5f : ptVelocity;
}

bool FormationUnit::rot2pointMoving(const Vect2f& point)
{
	MovementDirection angleNew = point - rigidBody_->position2D();
	float ptDir = ptDirectionAngle();
	angleNew -= rigidBody_->angle() + ptDir;
	if(fabs(angleNew) > 0.1f) {
		if(angleNew > 3 * M_PI_4 || angleNew < -3 * M_PI_4){
			setPtDirection(PT_DIR_BACK);
			rigidBody_->addPathTrackingAngle(-M_PI + ptDir);
		}else if(angleNew > M_PI_4){
			setPtDirection(PT_DIR_LEFT);
			rigidBody_->addPathTrackingAngle(M_PI_2 + ptDir);			
		}else if(angleNew < -M_PI_4){
			setPtDirection(PT_DIR_RIGHT);
			rigidBody_->addPathTrackingAngle(-M_PI_2 + ptDir);
		}else{
			rigidBody_->addPathTrackingAngle(ptDir);
			setPtDirection(PT_DIR_FORWARD);
		}
		return false;
	}

	return true;
}

bool FormationUnit::rot2direction(MovementDirection angleNew)
{
	if(fabs(angleNew - rigidBody_->angle()) > 0.1f) {
		pointToRotate_ = rigidBody_->position2D() + angleNew;
		rotationToPoint_ = true;
		return false;
	}
	return true;
}

bool FormationUnit::rot2point(const Vect2f& point)
{
	MovementDirection angleNew = point - rigidBody_->position2D();
	angleNew -= rigidBody_->angle();
	if(fabs(angleNew) > 0.1f) {
		pointToRotate_ = point;
		rotationToPoint_ = true;
		return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////
//
//    class PTAction
//
///////////////////////////////////////////////////////////////

void FormationController::PTAction::checkFinish(FormationController* formation)
{
	if(!timer_.busy())
		mode_ = ACTION_MODE_NONE;

	switch(mode_) {
	case ACTION_MODE_MOVE:
		if(formation->isMidPointReached(point_)){
			mode_ = ACTION_MODE_NONE;
			timer_.stop();
		}
		break;
	}
}

///////////////////////////////////////////////////////////////

void FormationController::PTAction::stop(PTActionPriority priority)
{
	if(active() && priority_ <= priority) {
		mode_ = ACTION_MODE_NONE;
		timer_.stop();
	}
}

///////////////////////////////////////////////////////////////

bool FormationController::PTAction::setMovePoint(const Vect2f& point, PTActionPriority priority)
{
	if(!active() || priority_ <= priority){
		priority_ = priority;
		point_ = point;
		mode_ = ACTION_MODE_MOVE;
		timer_.start(10000);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////
//
//    class FormationController
//
///////////////////////////////////////////////////////////////

FormationController::FormationController(const AttributeSquad::Formation* formation, bool forceUnitsAutoAttack)
	: position_(Vect2f::ZERO)
	, orientation_(0.0f)
	, radius_(0.0f)
	, velocity_(0.0f)
	, velocityMax_(0.0f)
	, velocityMaxPermanent_(0.0f)
	, forwardAcceleration_(0.0f)
	, pathTrackingAngle_(0.0f)
	, goingAroundObstacle_(false)
	, goingAroundBackObstacle_(false)
	, goingLeftAroundObstacle_(false)
	, obstacleCheck_(false)
	, disablePathTracking_(false)
	, followMainUnitInAutoMode_(forceUnitsAutoAttack)
	, canRotate_(false)
	, canMoveBack_(false)
	, moveback_(false)
	, wayPointSet_(false)
	, wayPoint_(Vect2f::ZERO)
	, autoPosition_(Vect2f::ZERO)
	, impassability_(0)
	, passability_(0)
	, stopQuants_(0)
	, stopFunctorFirstStep(0)
	, followSquad_(0)
	, positionInterpolationFactor_(1.0f)
{
	for(int i = 0; i < TERRAIN_TYPES_NUMBER; ++i)
		velocityFactorsByTerrain_[i] = 0.0f;
	setFormation(formation);
}

///////////////////////////////////////////////////////////////

void FormationController::initPose(const Vect2f& position, const MovementDirection& orientation)
{
	autoPosition_ = position_ = position;
	orientation_ = orientation;
}

///////////////////////////////////////////////////////////////

Vect2f FormationController::getUnitFormationPosition(const Vect2f& point, const FormationUnit* unit) const
{
	if(emptyFormationPattern())
		return point;
	return point + formation_->formationPattern->cells()[unit->formationIndex()] - formation_->formationPattern->cells()[leader()->formationIndex()];
}

///////////////////////////////////////////////////////////////

Vect2f FormationController::getUnitAttackPosition(const Vect2f& point, const FormationUnit* unit) const
{
	int index = unit->formationIndex() - leader()->formationIndex();
	float fireRadius = 0.0f;
	if(!unit->ownerUnit().fireTargetExist())
		return getUnitFormationPosition(position(), unit);
	if(unit->ownerUnit().canMoveToFireTarget(fireRadius)){
		Vect2f dir = point;
		do{
			dir = point;
			dir -= position();
			dir.normalize(1.0f);
			MatX2f pose;
			pose.rot.set(dir.y, dir.x, -dir.x, dir.y);
			pose.trans = point;
			++index;
			int i = index / 2;
			if(index % 2 == 1)
				i = -i;
			Mat2f rot(i * 0.1 * M_PI * (1.0f + 5.0f * clamp(100.0f - fireRadius, 0.0f, 100.0f) / 100.0f));
			dir.set(0.0f, -fireRadius);
			dir *= rot;
			dir *= pose;
		}while(!unit->rigidBody_->checkImpassability(dir) && index < 100);
		return Vect2f(dir.xi(), dir.yi());
	}
	return unit->ownerUnit().position2D();
}

///////////////////////////////////////////////////////////////

Vect2f FormationController::getUnitPosition(const Vect2f& point, const FormationUnit* unit, bool moving) const
{
	return moving || !alwaysMoving_ ? getUnitFormationPosition(point, unit) : point;
}

///////////////////////////////////////////////////////////////

Vect2f FormationController::getUnitLastPosition(const FormationUnit* unit) const
{
	if(unit->personalWayPoint())
		return unit->ownerUnit().formationController_.lastWayPoint();
	if(!wayPoints().empty())
		return getUnitFormationPosition(wayPoints().back(), unit);
	if(ptAction_.active())
		return getUnitFormationPosition(ptAction_.point(), unit);
	return getUnitFormationPosition(position(), unit);
}

///////////////////////////////////////////////////////////////

Vect2f FormationController::getUnitPositionDelta(const Vect2f& position, const FormationUnit* unit, bool moving) const
{
	return getUnitPosition(position, unit, moving) - unit->rigidBody_->position2D();
}

///////////////////////////////////////////////////////////////

MovementDirection FormationController::correctUnitDirection(const FormationUnit* unit) 
{
	Vect2f dir = Mat2f(float(unit->rigidBody_->angle() + unit->ptDirectionAngle())).invXform(getUnitPositionDelta(position(), unit, false));
	if(dir.norm2() > FLT_EPS){
		return clamp(MovementDirection(dir), -pathTrackingAngle(), pathTrackingAngle()); 
	}
	UnitLegionary* fireUnit = (!wayPointEmpty() && (unit->ownerUnit().unitState() != UnitReal::MOVE_MODE)) ? dynamic_cast<UnitLegionary*>(unit->ownerUnit().fireUnit()) : 0;
	if(fireUnit && fireUnit->formationUnit_.unitMove()){
		MovementDirection direct = fireUnit->rigidBody()->angle() - (unit->rigidBody_->angle() + unit->ptDirectionAngle());
		return clamp(direct, -pathTrackingAngle(), pathTrackingAngle());
	}
	return 0.0f;
}

///////////////////////////////////////////////////////////////

MovementDirection FormationController::correctUnitDirection(const FormationUnit* unit, const MovementDirection& direction) 
{
	Vect2f dir = Mat2f(float(unit->rigidBody_->angle() + unit->ptDirectionAngle())).invXform(getUnitPositionDelta(position(), unit));
	if(dir.norm2() > sqr(unit->rigidBody_->prm().is_point_reached_radius_max)){
		Vect2f direct = direction;
		dir *= 0.1f;
		return clamp(MovementDirection(direct + dir), -pathTrackingAngle(), pathTrackingAngle()); 
	}
	Vect2f direct = direction;
	MovementDirection correctOrientation = orientation() - (unit->rigidBody_->angle() + unit->ptDirectionAngle());
	if(fabsf(correctOrientation) > 1.e-5f){
		direct *= 0.5f;
		direct += correctOrientation;
	}
	return clamp(MovementDirection(direct), -pathTrackingAngle(), pathTrackingAngle());
}

///////////////////////////////////////////////////////////////

void FormationController::computePose()
{
	autoPosition_ = position_ = leader()->ownerUnit().position2D();
	orientation_ = leader()->rigidBody_->angle() + leader()->ptDirectionAngle();
}

///////////////////////////////////////////////////////////////

void FormationController::interpolatePose()
{
	if(positionInterpolationFactor_ < 1.0f)
		positionInterpolationFactor_ += 0.1f;
	else
		positionInterpolationFactor_ = 1.0f;
	autoPosition_ = position_.interpolate(position_, leader()->ownerUnit().position2D(), positionInterpolationFactor_);
	orientation_ = leader()->rigidBody_->angle() + leader()->ptDirectionAngle();
}

///////////////////////////////////////////////////////////////

void FormationController::computeRadius()
{
	float radius = 0.0f;
	FormationUnits::iterator ui;
	FOR_EACH(units_, ui){
		FormationUnit* unit = *ui;
		radius = max(radius, unit->rigidBody_->position2D().distance2(position()) + sqr(unit->rigidBody_->radius()));
	}
	radius_ = sqrtf(radius);
}

///////////////////////////////////////////////////////////////

bool FormationController::checkNumber(UnitFormationTypeReference unitType, int numberUnits) const
{
	if(emptyFormationPattern())
		return true;

	int number = 0;
	FormationPattern::Cells::const_iterator ci;
	FOR_EACH(formation_->formationPattern->cells(), ci)
		if(ci->type == unitType)
			++number;
	
	if(number == 0){
		xxassert(!units_.empty(), "�� ��������� ���������� ������ � ������");
		return units_.empty();
	}

	if(numberUnits > number)
		return false;

	return true;

}

///////////////////////////////////////////////////////////////

void FormationController::updateParameters()
{
	if(units_.empty())
		return;
	pathTrackingAngle_ = M_PI;
	obstacleCheck_ = false;
	disablePathTracking_ = false;
	canRotate_ = true;
	canMoveBack_ = true;
	impassability_ = 0;
	passability_ = 0xFFFFFFFF;
	alwaysMoving_ = false;
	FormationUnits::iterator ui;
	FOR_EACH(units_, ui){
		FormationUnit* unit = *ui;
		pathTrackingAngle_ = min(pathTrackingAngle_, G2R(unit->ownerUnit().attr().pathTrackingAngle));
		obstacleCheck_ |= unit->rigidBody_->prm().ptObstaclesCheck && unit->ownerUnit().attr().enablePathTracking;
		disablePathTracking_ |=  unit->ownerUnit().isSyndicateControl() && unit->ownerUnit().attr().disablePathTrackingInSyndicateControl;
		canRotate_ &= unit->ownerUnit().attr().enableRotationMode;
		canMoveBack_ &= unit->rigidBody_->prm().path_tracking_back;
		impassability_ |= (*ui)->rigidBody_->impassability();
		passability_ &= (*ui)->rigidBody_->calcPassabilityFlag();
		alwaysMoving_ |= (*ui)->rigidBody_->prm().alwaysMoving;
	}
	canRotate_ &= !alwaysMoving_;
	canMoveBack_ &= !alwaysMoving_;
	for(int i = 0; i < TERRAIN_TYPES_NUMBER; ++i)
		velocityFactorsByTerrain_[i] = leader()->ownerUnit().attr().velocityFactorsByTerrain[i];
	velocityMaxPermanent_ = leader()->rigidBody_->prm().forward_velocity_max;
	forwardAcceleration_ = leader()->rigidBody_->prm().forward_acceleration;
	if(units_.size() > 1){
		switch(formation_->velocityCorrection){
		case VELOCITY_MIN:
			FOR_OTHER(units_, ui){
				FormationUnit* unit = *ui;
				for(int i = 0; i < TERRAIN_TYPES_NUMBER; ++i)
					velocityFactorsByTerrain_[i] = min(velocityFactorsByTerrain_[i], unit->ownerUnit().attr().velocityFactorsByTerrain[i]);
				velocityMaxPermanent_ = min(velocityMaxPermanent_, unit->rigidBody_->prm().forward_velocity_max);
				forwardAcceleration_ = min(forwardAcceleration_, unit->rigidBody_->prm().forward_acceleration);
			}
			break;
		case VELOCITY_DEFAULT:
		case VELOCITY_AVERAGE:{
			FOR_OTHER(units_, ui){
				FormationUnit* unit = *ui;
				for(int i = 0; i < TERRAIN_TYPES_NUMBER; ++i)
					velocityFactorsByTerrain_[i] += unit->ownerUnit().attr().velocityFactorsByTerrain[i];
				velocityMaxPermanent_ += unit->rigidBody_->prm().forward_velocity_max;
				forwardAcceleration_ += unit->rigidBody_->prm().forward_acceleration;
			}
			float invUnitsNumber = 1.0f / units_.size();
			for(int i = 0; i < TERRAIN_TYPES_NUMBER; ++i)
				velocityFactorsByTerrain_[i] *= invUnitsNumber;
			velocityMaxPermanent_ *= invUnitsNumber;
			forwardAcceleration_ *=  logicPeriodSeconds * invUnitsNumber;
			break;}
		case VELOCITY_MAX:
			FOR_OTHER(units_, ui){
				FormationUnit* unit = *ui;
				for(int i = 0; i < TERRAIN_TYPES_NUMBER; ++i)
					velocityFactorsByTerrain_[i] = max(velocityFactorsByTerrain_[i], unit->ownerUnit().attr().velocityFactorsByTerrain[i]);
				velocityMaxPermanent_ = max(velocityMaxPermanent_, unit->rigidBody_->prm().forward_velocity_max);
				forwardAcceleration_ = max(forwardAcceleration_, unit->rigidBody_->prm().forward_acceleration);
			}
			break;
		}
	}
}

///////////////////////////////////////////////////////////////

void FormationController::addToFormation(FormationUnit* unit)
{
	if(emptyFormationPattern())
		return;

	for(int i=0; i< formation_->formationPattern->cells().size(); ++i)
		if(formation_->formationPattern->cells()[i].type 
		== unit->ownerUnit().attr().formationType && freePositions_[i]) {
			freePositions_[i] = false;
			unit->setFormationIndex(i);
			return;
		}

	xxassert(false, "������ ���� � ������ ��� ������� �� �����������������...");
	return;
}

///////////////////////////////////////////////////////////////

void FormationController::removeFromFormation(FormationUnit* unit)
{
	if(!emptyFormationPattern())
		freePositions_[unit->formationIndex()] = true;
}

///////////////////////////////////////////////////////////////

void FormationController::addUnit(FormationUnit* unit)
{
	if(emptyFormationPattern() || unit->formationIndex() == -1){
		addToFormation(unit);
		units_.push_back(unit);
		updateParameters();
	}
}

///////////////////////////////////////////////////////////////

void FormationController::removeUnit(FormationUnit* unit)
{
	FormationUnits::iterator ui;
	FOR_EACH(units_, ui)
		if(*ui == unit){
			removeFromFormation(unit);
			unit->setFormationIndex(-1);
			units_.erase(ui);
			break;
		}
	updateParameters();
}

///////////////////////////////////////////////////////////////

void FormationController::setMainUnit(FormationUnit* mainUnit)
{
	if(units_.empty())
		return;
	units_.setLeader(mainUnit);
}

///////////////////////////////////////////////////////////////

float FormationController::computeVelocityMax() const
{
	float velocity = leader()->ownerUnit().forwardVelocity(moveback());
	FormationUnits::const_iterator ui;
	if(units_.activeSize() > 1){
		switch(formation_->velocityCorrection){
		case VELOCITY_MIN:
			FOR_OTHER_ACTIVE(units_, ui)
				velocity = min(velocity, (*ui)->ownerUnit().forwardVelocity(moveback()));
			break;
		case VELOCITY_DEFAULT:
		case VELOCITY_AVERAGE:
			FOR_OTHER_ACTIVE(units_, ui)
				velocity += (*ui)->ownerUnit().forwardVelocity(moveback());
			velocity /= units_.activeSize();
			break;
		case VELOCITY_MAX:
			FOR_OTHER_ACTIVE(units_, ui)
				velocity = max(velocity, (*ui)->ownerUnit().forwardVelocity(moveback()));
			break;
		}
	}
	FOR_EACH_ACTIVE(units_, ui)
		(*ui)->rigidBody_->setForwardVelocity(velocity);
	return velocity;
}

///////////////////////////////////////////////////////////////

void FormationController::velocityProcessing()
{
	velocityMax_ = computeVelocityMax();
	float averageVelocity = 0.0f;
	int movingUnits = 0;
	FormationUnits::iterator ui;
	FOR_EACH_ACTIVE(units_, ui){
		FormationUnit* unit = *ui;
		UnitLegionary* fireUnit = (unit->ownerUnit().unitState() != UnitReal::MOVE_MODE) ? dynamic_cast<UnitLegionary*>(unit->ownerUnit().fireUnit()) : 0;
		if(followSquad_ || (fireUnit && fireUnit->formationUnit_.unitMove()) || !unit->isPointReached(getUnitLastPosition(unit))){
			float velocity = clamp(unit->ptVelocity() + forwardAcceleration(), 0.0f, velocityMax());
			unit->setPtVelocity(velocity);
			Vect2f dir(unit->movingDirection(moveback()));
			dir.normalize(velocity);
			float forwardVelocity = dir.dot(orientation());
			if(forwardVelocity > 0.0f){
				averageVelocity +=  forwardVelocity;
				++movingUnits;
			}
		}else
			unit->setPtVelocity(0.0f);
	}
	velocity_ = movingUnits ? clamp(averageVelocity / movingUnits, 0, velocityMax()) : 0.0f;
	Vect2f currentWayPoint = wayPoint();
	FOR_EACH_ACTIVE(units_, ui){
		FormationUnit* unit = *ui;
		if(unit->unitMove()){
			Vect2f dir(unit->movingDirection(moveback()));
			Vect2f delta = getUnitPositionDelta(currentWayPoint, unit);
			float wpointDist = delta.norm2() / fabsf(delta.dot(dir) + 0.001f) / logicPeriodSeconds;
			Vect2f positionDelta = getUnitPositionDelta(position(), unit);
			float positionDist = positionDelta.norm2() / (positionDelta.dot(dir) + 0.001f) / logicPeriodSeconds;
			float followVelocity = 0.0f;
			if(followSquad_)
				followVelocity = followSquad_->formationController_.velocity() / (Vect2f(dir).dot(followSquad_->formationController_.orientation()) + 0.001f);
			else{
				UnitLegionary* fireUnit = (unit->ownerUnit().unitState() != UnitReal::MOVE_MODE) ? dynamic_cast<UnitLegionary*>(unit->ownerUnit().fireUnit()) : 0;
				if(fireUnit)
					followVelocity = fireUnit->formationUnit_.ptVelocity() / (Vect2f(fireUnit->formationUnit_.movingDirection(moveback())).dot(dir) + 0.001f);
			}
			float velocityMax = max(min(followVelocity + wpointDist, velocity_ / (Vect2f(dir).dot(orientation()) + 0.001f)) + positionDist, 0.0f);
			unit->setPtVelocity(min(unit->ptVelocity(), velocityMax));
		}
	}
}

///////////////////////////////////////////////////////////////

void FormationController::stopMoving() 
{ 
	velocity_ = 0.0f;
	FormationUnits::iterator ui;
	FOR_EACH_ACTIVE(units_, ui){
		(*ui)->setPtVelocity(0.0f); 
		(*ui)->setRotationSpeed(0.0f);
	}
}

///////////////////////////////////////////////////////////////

void FormationController::setFormation(const AttributeSquad::Formation* formation)
{
	formation_ = formation;
	if(!emptyFormationPattern()){
		freePositions_.clear();
		freePositions_.resize(formation_->formationPattern->cells().size(), true);
	}
}

///////////////////////////////////////////////////////////////

void FormationController::changeFormation(const AttributeSquad::Formation* formation)
{
	FormationUnits::const_iterator ui;
	FOR_EACH(units_, ui)
		removeFromFormation(*ui);
	setFormation(formation);
	FOR_EACH(units_, ui)
		addToFormation(*ui);
}

///////////////////////////////////////////////////////////////

bool FormationController::personalWayPointsEmpty() const
{
	return units_.activeSize() == units_.size();
}

///////////////////////////////////////////////////////////////

void FormationController::wayPointsClear() 
{ 
	wayPointSet_ = false;
	wayPoints_.clear(); 
	FormationUnits::const_iterator ui;
	FOR_EACH_INACTIVE(units_, ui)
		(*ui)->clearPersonalWayPoint();
	units_.makeAllActive();
}

///////////////////////////////////////////////////////////////

void FormationController::findPath()
{
	WayPoints wayPointsTemp;
	pathFinder->findPath(position(), lastWayPoint(), passableObstacleTypes(), impassableTerrainTypes(), velocityFactorsByTerrain_, wayPointsTemp);
	recalcPathTimer_.start(2000);
	if(!wayPointsTemp.empty())
		wayPoints_.swap(wayPointsTemp); 
	else
		wayPointsClear();
}

///////////////////////////////////////////////////////////////

void FormationController::setWayPoint(const Vect2f& point)
{
	if(wayPointEmpty() || !recalcPathTimer_.busy() || !lastWayPoint().eq(point)){
		goingAroundObstacle_ = false;
		wayPoint_ = point;
		wayPointSet_ = true;
		if(disablePathTracking()){
			wayPoints_.clear();
			wayPoints_.push_back(lastWayPoint());
		}else
			findPath();
	}
}

///////////////////////////////////////////////////////////////

void FormationController::addWayPointS(const Vect2f& point)
{
	FormationUnits::iterator ui;
	FOR_EACH_INACTIVE(units_, ui)
		(*ui)->clearPersonalWayPoint();
	units_.makeAllActive();
	FOR_EACH_ACTIVE(units_, ui)
		(*ui)->ownerUnit().startMoving();
	autoPosition_ = point;
	setWayPoint(point);
}

///////////////////////////////////////////////////////////////

Se3f FormationController::getNewUnitPosition(FormationUnit* unit)
{
	return Se3f(orientation(), To3D(getUnitFormationPosition(position(), unit)));
}

///////////////////////////////////////////////////////////////

void FormationController::addWayPointAttack(const Vect2f& point)
{
	if(leader()->ownerUnit().unitState() == UnitReal::MOVE_MODE)
		leader()->ownerUnit().setUnitState(UnitReal::AUTO_MODE);
	leader()->clearPersonalWayPoint();
	setWayPoint(getUnitAttackPosition(point, leader()));
	FormationUnits::iterator ui;
	FOR_OTHER(units_, ui){
		if((*ui)->ownerUnit().unitState() == UnitReal::MOVE_MODE)
			(*ui)->ownerUnit().setUnitState(UnitReal::AUTO_MODE);
		(*ui)->setPersonalWayPoint(getUnitAttackPosition(point, *ui));
	}
}

///////////////////////////////////////////////////////////////

bool FormationController::isMidPointReached(const Vect2f& point)
{
	FormationUnits::iterator ui;
	FOR_EACH(units_, ui)
		if(!(*ui)->isPointReachedMid(getUnitFormationPosition(point, *ui)))
			return false;
	return true;
}

///////////////////////////////////////////////////////////////

bool FormationController::isLastPointReached()
{
	FormationUnits::iterator ui;
	FOR_EACH_ACTIVE(units_, ui)
		if(!(*ui)->isPointReached(getUnitLastPosition(*ui)))
			return false;
	return true;
}

///////////////////////////////////////////////////////////////

Vect2f FormationController::wayPointDir() const
{
	return wayPoint() - position();
}

///////////////////////////////////////////////////////////////

bool FormationController::wayPointFar() const
{
	return wayPointDir().norm2() > sqr(6 * velocityMax() * logicPeriodSeconds);
}

///////////////////////////////////////////////////////////////

MovementDirection FormationController::computePreferedDirection() const
{
	Vect2f localWayPoint = Mat2f((float)orientation()).invXform(wayPointDir());
	float dist2 = localWayPoint.norm2();
	if(dist2 < FLT_EPS)
		return 0.0f;
	if(localWayPoint.y < 0.0f || moveback() || disablePathTracking())
		return localWayPoint;
	float preferedDirection = clamp(-2.0f * localWayPoint.x * velocityMaxPermanent() * logicPeriodSeconds / dist2, -M_PI, M_PI);
	float invR = preferedDirection / (velocityMaxPermanent() * logicPeriodSeconds);
	float relativeAngle = fabsf(preferedDirection) / pathTrackingAngle();
	if(relativeAngle > 1.0f || dist2 * sqr(invR) > 3.0f * (2.0f - relativeAngle) * relativeAngle)
		return localWayPoint;
	return preferedDirection;
}

///////////////////////////////////////////////////////////////

PossibleSector FormationController::computeForwardSector() const
{
	return PossibleSector(-pathTrackingAngle(), pathTrackingAngle());
}

///////////////////////////////////////////////////////////////

PossibleSector FormationController::computeBackwardSector() const
{
	return PossibleSector(M_PI - pathTrackingAngle(), pathTrackingAngle() - M_PI);
}

///////////////////////////////////////////////////////////////

void FormationController::move(float rotSpeed, bool ignoreAngles) 
{
	FormationUnits::iterator ui;
	FOR_EACH_ACTIVE(units_, ui)
		if((*ui)->unitMove())
			(*ui)->setRotationSpeed(correctUnitDirection(*ui, rotSpeed * (*ui)->ptVelocity() / velocityMaxPermanent()));
		else
			(*ui)->rotate(correctUnitDirection(*ui, rotSpeed));
	Vect2f currentWayPoint = wayPoint();
	currentWayPoint -= position();
	float wpointDist = currentWayPoint.norm();
	float ptVelocity = velocity();
	if(ptVelocity * logicPeriodSeconds > wpointDist)
		ptVelocity = wpointDist / logicPeriodSeconds;
	orientation_ += ignoreAngles ? rotSpeed : rotSpeed * ptVelocity / velocityMaxPermanent();
	MovementDirection dir = orientation();
	Vect2f vel = dir;
	vel *= (moveback() ? -ptVelocity : ptVelocity) * logicPeriodSeconds;
	position_ += vel; 
}

///////////////////////////////////////////////////////////////

void FormationController::moveForward(const MovementDirection& direction, bool ignoreAngles)
{
	if(moveback()){
		debugMessage("PT: ���� ������");
		moveback_ = false;
		stopMoving();
		return;
	}
	move(direction, ignoreAngles);
}

///////////////////////////////////////////////////////////////

void FormationController::moveBackward(const MovementDirection& direction)
{
	MovementDirection dir = direction;
	dir.negate();
	move(dir);
}

///////////////////////////////////////////////////////////////

void FormationController::rotate(const MovementDirection& moveDirection) 
{
	FormationUnits::iterator ui;
	float rotSpeed = 0.0f;
	FOR_EACH_ACTIVE(units_, ui)
		rotSpeed += (*ui)->rotate(moveDirection);
	rotSpeed /= units_.activeSize();
	orientation_ += rotSpeed;
}

///////////////////////////////////////////////////////////////

Passability FormationController::checkPassability(Vect2f moveLine, const ObstacleList& obstacleList, float orientation, float rotAngle, int scanningDistance)
{
	MatX2f pose(Mat2f(orientation), position());
	Mat2f rot(rotAngle);
	moveLine *= velocityMaxPermanent() * logicPeriodSeconds;
	bool checkObstacles = obstacleCheck() && !obstacleList.empty();
	FormationUnits::const_iterator ui;
	if(checkObstacles){
		FOR_EACH_ACTIVE(units_, ui){
			if((*ui)->collidingObstacleList(getUnitFormationPosition(pose.trans, *ui), pose.rot, obstacleList, true)){
				checkObstacles = false;
				break;
			}
		}
	}
	Passability passability(checkObstacles);
	int maxLineTemp = -1;
	for(int j = 0; j < scanningDistance; ++j){
		Vect2f newPosition = moveLine;
		newPosition *= pose;
		pose.rot *= rot;
		if(wayPoint().distance2(newPosition) < 1.f)
			return passability;	
		FOR_EACH_ACTIVE(units_, ui){
			Vect2f unitPositionPrev(getUnitFormationPosition(pose.trans, *ui));
			Vect2f unitPosition(getUnitFormationPosition(newPosition, *ui));
			if(!(*ui)->computePassabilityFactor(unitPositionPrev, unitPosition)){
				passability.terrainPassability = j;
				return passability;
			}
			if(passability.obstaclePassability < 0 && (*ui)->collidingObstacleList(unitPosition, pose.rot, obstacleList, j == 0)){
				passability.obstaclePassability = j;
				break;
			}
		}
		if(showDebugRigidBody.showPathTracking_Lines)
			show_line(To3D(pose.trans), To3D(newPosition), !obstacleCheck() || obstacleList.empty() || passability.obstaclePassability < 0 ? Color4c::GREEN : Color4c::RED);
		pose.trans = newPosition;
	}
	return passability;
}

///////////////////////////////////////////////////////////////

bool FormationController::wayPointOnClosedGraph(const MovementDirection& moveDirection)
{
	return false;
}

///////////////////////////////////////////////////////////////

bool FormationController::checkPassabilityForward(const MovementDirection& preferedDirection, const ObstacleList& obstacleList, bool inForwardSector)
{
	if(goingAroundObstacle_ && ((!canRotate() && !inForwardSector) || wayPointOnClosedGraph(preferedDirection)))
		return false;
	if(inForwardSector)
		return checkPassability(preferedDirection, obstacleList, orientation(), preferedDirection, disablePathTracking() ? 1 : Passability::scanningDistance);
	return checkPassability(Vect2f(0.0f, 1.0f), obstacleList, orientation() + preferedDirection, 0.0f, disablePathTracking() || moveback() ? 1 : Passability::scanningDistance);
}

///////////////////////////////////////////////////////////////

bool FormationController::checkPassabilityBackward(MovementDirection& preferedDirection, const ObstacleList& obstacleList)
{
	if(wayPointFar())
		preferedDirection.negate();
	preferedDirection = computeBackwardSector().getPossibleDirection(preferedDirection);
	return checkPassability(preferedDirection, obstacleList, orientation(), preferedDirection + M_PI, 1 + int(velocityMax() / velocityMaxPermanent()));
}

///////////////////////////////////////////////////////////////

bool FormationController::rotateToDirection(const MovementDirection& moveDirection, const ObstacleList& obstacleList)
{
	if(!moveback()){
		if(canRotate()){
			debugMessage("PT: ��������");
			stopMoving();
			rotate(moveDirection);
			return true;
		}
		if(canMoveBack()){
			if(goingAroundBackObstacle_){
				MovementDirection preferedDirection = moveDirection;
				if(!checkPassabilityBackward(preferedDirection, obstacleList))
					return false;
			}
			debugMessage("PT: �������� �����");
			goingAroundBackObstacle_ = false;
			moveback_ = true;
			stopMoving();
			return true;
		}
		moveForward(moveDirection, true);
		return true;
	}
	MovementDirection preferedDirection = goingAroundObstacle_ ? MovementDirection(goingLeftAroundObstacle_ ? pathTrackingAngle() : -pathTrackingAngle()) : moveDirection;
	if(checkPassabilityBackward(preferedDirection, obstacleList)){
		moveBackward(preferedDirection);
	}else{
		debugMessage("PT: ����������� �����");
		goingAroundBackObstacle_ = true;	
		moveback_ = false;
		stopMoving();
	}
	return true;
}

///////////////////////////////////////////////////////////////

void FormationController::computeMoveLinesPassability(PossibleMovementDirections& directions, const ObstacleList& obstacleList)
{
	PossibleMovementDirections::iterator id;
	FOR_EACH(directions, id)
		(*id) = checkPassability(id->direction(), obstacleList, orientation(), id->direction());
}

///////////////////////////////////////////////////////////////

void FormationController::moveAlongWall(const ObstacleList& obstacleList)
{
	if(disablePathTracking()){
		stopMoving();
		return;
	}
	PossibleSector sector = computeForwardSector();
	PossibleMovementDirections directions(sector, 15);
	computeMoveLinesPassability(directions, obstacleList);
	if(!goingAroundObstacle_){
		debugMessage("PT: ��������� �����������");
		goingAroundObstacle_ = true;
		goingLeftAroundObstacle_ = directions.checkLeftBeterThenRight();
	}
	PossibleMovementDirection maxLine = directions.getOptimalDirection(goingLeftAroundObstacle_);
	if(maxLine.isFullyPassable()
	|| !rotateToDirection(maxLine.direction(), obstacleList))
		moveForward(maxLine.direction());
}

///////////////////////////////////////////////////////////////

class UnitMovePlaner 
{
public:

	bool removeWPoint;
	Vect2f newWPoint;
	vector<RigidBodyBase*> obstacleList;
	bool waitLeftObstacle_;

	UnitMovePlaner(FormationController* tracker, const Vect2f& localWPoint) 
		: formation_(tracker)
		, removeWPoint(false)
		, newWPoint(Vect2f::ZERO)
		, waitLeftObstacle_(false)
		, localWayPoint(localWPoint)
		, penetrationFound_(false)
		, maxDelta(-1.0f)
	{ 
	}

	void makeWay(UnitBase* p) 
	{
		if(!p->attr().isLegionary())
			return;

		FormationUnit* formationUnit = &(safe_cast<UnitLegionary*>(p)->formationUnit_);

		if(!formationUnit->rigidBody_->prm().controled_by_points || !p->attr().enablePathTracking 
		|| formationUnit->manualMoving() || safe_cast<UnitActing*>(p)->fireDistanceCheck())
			return;

		if((GlobalAttributes::instance().enableEnemyMakeWay || !p->isEnemy(&formation_->leader()->ownerUnit())) 
		&& !formationUnit->unitMove() && formation_->leader()->ownerUnit().radius() > p->radius()) {
			Vect2f movDir(1.0f, 0.0f);
			movDir *= formation_->leader()->ptPose().rot;
			Vect2f poseDelta = formation_->leader()->ptPose().rot.invXform(p->position2D() - formation_->leader()->ownerUnit().position2D());
			if(poseDelta.y > 0 && fabs(poseDelta.x) <= (p->radius()+formation_->leader()->ownerUnit().radius())) {
				formationUnit->setMovementMode(formationUnit->movementMode() | MODE_RUN);
				formationUnit->disableRotationMode();
				safe_cast<UnitLegionary*>(p)->squad()->formationController_.moveAction(p->position2D() + 
					movDir * SIGN(poseDelta.y) * (2.0f * p->radius() + formation_->leader()->ownerUnit().radius()), 
					FormationController::ACTION_PRIORITY_PATH_TRACKING);

			}
		}
	}

	bool checkRadius(UnitBase* p) 
	{
		Vect2f norm = formation_->wayPoint() - formation_->leader()->ownerUnit().position2D();
		norm.set(norm.y, -norm.x);
		Vect2f oDir = p->position2D() - formation_->leader()->ownerUnit().position2D();
		float dist = fabs(norm.dot(oDir)/norm.norm());
		return dist < formation_->leader()->ownerUnit().radius() * 1.2f + p->radius();
	}


	Vect2f newWpoint() 
	{
		Vect2f unitDir = formation_->wayPoint() - formation_->leader()->ownerUnit().position2D();
		float dist = unitDir.norm();
		if(dist < maxDelta)
			return formation_->leader()->ownerUnit().position2D();
		dist -= maxDelta;
		unitDir.normalize(dist);
		return formation_->leader()->ownerUnit().position2D() + unitDir;
	}

	bool stopFunctor(UnitBase* p) 
	{
		if(p->attr().isLegionary() && safe_cast<UnitLegionary*>(p)->squad()->formationController_.controlled())
			return false;

		if(!checkRadius(p))
			return false;

		Vect2f unitDir = formation_->leader()->ownerUnit().position2D() - formation_->wayPoint();
		Vect2f obstacleDir = p->position2D() - formation_->wayPoint();
		float distDelta = unitDir.dot(obstacleDir) / unitDir.norm();
		distDelta += p->radius();
		distDelta += formation_->leader()->ownerUnit().radius();

		if(distDelta < 0 || distDelta > unitDir.norm())
			return false;

		if(distDelta > maxDelta){
			maxDelta = distDelta;
			return true;
		}
		return false;
	}

	void operator () (UnitBase* p) 
	{
		if(!p->alive() || !p->rigidBody() || p->isDocked()
		|| (p->attr().isLegionary() && safe_cast<UnitLegionary*>(p)->squad() == formation_->leader()->ownerUnit().squad())
		|| formation_->isIgnoreUnit(p) || !p->checkInPathTracking(&formation_->leader()->ownerUnit()))
			return;

		if(GlobalAttributes::instance().enableMakeWay)
			makeWay(p);

		if(GlobalAttributes::instance().enablePathTrackingRadiusCheck 
		&& (GlobalAttributes::instance().enemyRadiusCheck || !formation_->leader()->ownerUnit().isEnemy(p)) 
		&& formation_->leader()->ownerUnit().radius() > GlobalAttributes::instance().minRadiusCheck 
		&& p->radius() <= GlobalAttributes::instance().minRadiusCheck)
			return;

		if(localWayPoint.x < 0 && p->attr().isLegionary()){
			FormationUnit* obstacleBody = &(safe_cast<UnitLegionary*>(p)->formationUnit_);
			if(obstacleBody->unitMove() && !obstacleBody->rigidBody_->isBoxMode() && obstacleBody->rigidBody_->position2D().distance2(formation_->leader()->rigidBody_->position2D()) < sqr(1.5f*obstacleBody->rigidBody_->radius() + 1.5f * formation_->leader()->rigidBody_->radius())){
				Vect2f obstaclePose = formation_->leader()->ptPose().invXform(p->position2D());
				if(obstaclePose.x < 0){
					Vect2f obstacleDir(0.0f , 1.0f);
					Vect2f trackerDir(0.0f, 1.0f);
					obstacleDir *= obstacleBody->ptPose().rot;
					trackerDir *= formation_->leader()->ptPose().rot;
					if(obstacleDir.dot(trackerDir) > 0.8f){
						waitLeftObstacle_ = true;
						return;
					}
				}
			}
		}

		obstacleList.push_back(p->rigidBody());

		if(p->rigidBody()->bodyIntersect(formation_->leader()->rigidBody_, formation_->leader()->rigidBody_->position2D(), formation_->leader()->ptPose().rot))
			penetrationFound_ = true;

		if(!p->rigidBody()->ptBoundCheck() && !formation_->leader()->rigidBody_->ptBoundCheck() && p->position2D().distance2(formation_->wayPoint()) < sqr(p->radius() + 0.5f * formation_->leader()->ownerUnit().radius())
		&& localWayPoint.norm2() < sqr(p->radius() + 1.4f * formation_->leader()->ownerUnit().radius())) {
			if(formation_->stopFunctorFirstStep != 0)
				newWPoint = formation_->leader()->ownerUnit().position2D();
			else 
				newWPoint = p->position2D() + Vect2f(p->radius() + 1.5f * formation_->leader()->ownerUnit().radius(), 0.0f);
			removeWPoint = true;
		}

		if(localWayPoint.norm2() < sqr(formation_->leader()->ownerUnit().attr().getPTStopRadius()) 
		&& formation_->leader()->ownerUnit().unitState() == UnitReal::MOVE_MODE && !penetrationFound_){
			if(stopFunctor(p)){
				if(formation_->stopFunctorFirstStep == 0)
					formation_->stopFunctorFirstStep = 1;
				else if(formation_->stopFunctorFirstStep == 20){
					newWPoint = newWpoint();
					removeWPoint = true;
					formation_->stopFunctorFirstStep = 0;
				}
			}
		}
	}

private:
	FormationController* formation_;
	Vect2f localWayPoint;
	bool penetrationFound_;
	float maxDelta;
};

///////////////////////////////////////////////////////////////

bool FormationController::isIgnoreUnit(UnitBase* p) 
{
	FormationUnits::const_iterator ui;
	FOR_EACH_ACTIVE(units_, ui)
		if(p == (*ui)->ignoreUnit_)
			return true;
	return false;
}

///////////////////////////////////////////////////////////////

void FormationController::pathTracking()
{
	ObstacleList obstacleList;

	if(obstacleCheck()){
		if(velocity() > FLT_EPS && stopFunctorFirstStep != 0)
			stopFunctorFirstStep++;

		Vect2f localWayPoint = MatX2f(Mat2f((float)orientation()), position()).invXform(wayPoint());
		UnitMovePlaner mp(this, localWayPoint);

		float scanRadius = radius() + velocity() * logicPeriodSeconds * 15;
		if(localWayPoint.norm2() < sqr(leader()->ownerUnit().attr().getPTStopRadius()))
			scanRadius = max(localWayPoint.norm(), scanRadius);

		universe()->unitGrid.Scan(position().xi(), position().yi(), round(scanRadius), mp);

		obstacleList.swap(mp.obstacleList);

		if(mp.waitLeftObstacle_){
			stopQuants_ = 1;
			stopMoving();
			return;
		}

		if(!disablePathTracking() && mp.removeWPoint) {
			if(ptAction_.active())
				ptAction_.setPoint(clampWorldPosition(mp.newWPoint, radius()));
			else if(wayPoints().size() == 1){
				Vect2f p2 = clampWorldPosition(mp.newWPoint, radius());
				wayPoints_.clear();
				wayPoints_.push_back(p2);
			}
			stopFunctorFirstStep = 0;
		}
	}
	if((!ptAction_.active() || manualMoving()) && leader()->ownerUnit().runMode())
		leader()->setMovementMode(leader()->ownerUnit().manualMovementMode);

	MovementDirection preferedDirection = computePreferedDirection();
	bool inForwardSector = computeForwardSector().checkInSector(preferedDirection);
    if(checkPassabilityForward(preferedDirection, obstacleList, inForwardSector)){
		goingAroundObstacle_ = false;
		if(inForwardSector || !rotateToDirection(preferedDirection, obstacleList))
			moveForward(preferedDirection);
	}else
		moveAlongWall(obstacleList);
}

///////////////////////////////////////////////////////////////

bool FormationController::controlled() const 
{ 
	if(wayPoints().size() > 1 || followSquad_ || ptAction_.active() || manualMoving())
		return true;

	FormationUnits::const_iterator ui;
	FOR_EACH_ACTIVE(units_, ui){
		FormationUnit* unit = *ui;
		if(unit->rigidBody_->prm().alwaysMoving || unit->rotationMode())
			return true;
		UnitLegionary* fireUnit = (!wayPointEmpty() && (unit->ownerUnit().unitState() != UnitReal::MOVE_MODE)) ? dynamic_cast<UnitLegionary*>(unit->ownerUnit().fireUnit()) : 0;
		if((fireUnit && fireUnit->formationUnit_.unitMove()) || !unit->isPointReached(getUnitLastPosition(unit)))
			return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////

void FormationController::stateQuant()
{
	if(disablePathTracking() || !alwaysMoving_) 
		computePose();
		
	if(alwaysMoving_ && (position().distance2(wayPoint()) > 10.0f * leader()->rigidBody_->prm().is_point_reached_radius_max))
		interpolatePose();
	else
		positionInterpolationFactor_ = 0.0f;

	FormationUnits::iterator ui;
	for(ui = units_.begin(); ui != units_.inactive(); ){
		if((*ui)->personalWayPoint() || (*ui)->unmovable())
			units_.makeInactive(ui);
		else
			++ui;
	}
	for(ui = units_.inactive(); ui != units_.end(); ++ui){
		if(!(*ui)->personalWayPoint() && !(*ui)->unmovable())
			units_.makeActive(ui);
	}
	switch(leader()->ownerUnit().unitState()){
	case UnitReal::ATTACK_MODE:
		if(!leader()->ownerUnit().fireTargetExist())
			break;
		if((leader()->ownerUnit().selectedWeaponID() || leader()->ownerUnit().isDefaultWeaponExist()) && !leader()->ownerUnit().fireTargetDocked()){
			addWayPointAttack(leader()->ownerUnit().firePosition());
			if(!wayPointEmpty())
				autoPosition_ = lastWayPoint();
		}else
			wayPointsClear();
		if(leader()->ownerUnit().isSyndicateControl() && leader()->ownerUnit().attr().rotToTarget){
			leader()->setPtDirection(FormationUnit::PT_DIR_FORWARD);
			if(disablePathTracking() || !alwaysMoving_)
				computePose();
		}
		break;
	case UnitReal::AUTO_MODE:
		if(followMainUnitInAutoMode_ && leader()->ownerUnit().autoAttackMode() == ATTACK_MODE_OFFENCE){
			if(leader()->ownerUnit().canAutoMove()){
				if(leader()->ownerUnit().fireTargetExist())
					addWayPointAttack(leader()->ownerUnit().firePosition());
				else
					setWayPoint(autoPosition_);
			}
		}
		if(leader()->ownerUnit().isSyndicateControl() && leader()->ownerUnit().attr().rotToTarget){
            leader()->setPtDirection(FormationUnit::PT_DIR_FORWARD);
			if(disablePathTracking() || !alwaysMoving_)
				computePose();
		}
		break;
	case UnitReal::MOVE_MODE:
		if(((wayPointEmpty() || wayPoints().empty()) && personalWayPointsEmpty())
		|| (alwaysMoving_ ? position().distance2(wayPoint()) < leader()->rigidBody_->prm().is_point_reached_radius_max : isLastPointReached())){
			wayPointsClear();
			FOR_EACH(units_, ui){
				FormationUnit* unit = *ui;
				unit->ownerUnit().setUnitState(UnitReal::AUTO_MODE);
				unit->ownerUnit().setTargetUnit(0);
			}
		}
		if(!wayPointEmpty() && !disablePathTracking())
			findPath();
		if(leader()->ownerUnit().isSyndicateControl() && leader()->ownerUnit().attr().rotToTarget){
			if(leader()->ownerUnit().fireTargetExist() && !leader()->ownerUnit().fireTargetDocked())
				leader()->rot2pointMoving(leader()->ownerUnit().firePosition());
			else{
				leader()->setPtDirection(FormationUnit::PT_DIR_FORWARD);
				if(disablePathTracking() || !alwaysMoving_)
					computePose();
			}
		}	
		break;
	}

	FOR_EACH(units_, ui){
		FormationUnit* unit = *ui;
		switch(unit->ownerUnit().unitState()){
		case UnitReal::AUTO_MODE:
			switch(unit->ownerUnit().autoAttackMode()){
			case ATTACK_MODE_OFFENCE:
				if((!unit->ownerUnit().isSyndicateControl() || !unit->ownerUnit().squad()->attr().disableMainUnitAutoAttack)
				&& (!followMainUnitInAutoMode_ && unit->ownerUnit().canAutoMove())){
					if(unit->ownerUnit().fireTargetExist()){
						unit->setPersonalWayPoint(unit->rigidBody_->prm().alwaysMoving 
							? unit->ownerUnit().firePosition() 
							: getUnitAttackPosition(unit->ownerUnit().firePosition(), unit));
						
					}else{
						if(!unit->ownerUnit().noiseTarget() && !unit->isPointReached(getUnitFormationPosition(position(), unit)))
							unit->clearPersonalWayPoint();
					}
				}
			case ATTACK_MODE_DEFENCE:
				UnitLegionary* fireUnit = (unit->ownerUnit().unitState() != UnitReal::MOVE_MODE) ? dynamic_cast<UnitLegionary*>(unit->ownerUnit().fireUnit()) : 0;
				if(!followSquad_ && (!fireUnit || !fireUnit->formationUnit_.unitMove()) && unit->isPointReached(getUnitLastPosition(unit)) && !unit->ownerUnit().rotToFireTarget())
					unit->ownerUnit().rotToNoiseTarget();
				break;
			}
			break;
		case UnitReal::ATTACK_MODE:
			UnitLegionary* fireUnit = (unit->ownerUnit().unitState() != UnitReal::MOVE_MODE) ? dynamic_cast<UnitLegionary*>(unit->ownerUnit().fireUnit()) : 0;
			if(!followSquad_ && (!fireUnit || !fireUnit->formationUnit_.unitMove()) && unit->isPointReached(getUnitLastPosition(unit)))
				unit->ownerUnit().rotToFireTarget();
			break;
		}
		if(!unit->ownerUnit().fireTargetExist())
			unit->ownerUnit().setAttackTargetUnreachable(false);
	}

	if(followSquad_){
		if(position().distance2(followSquad_->position2D()) > sqr((radius() + followSquad_->radius()) * 1.5f)){
			Vect2f point = position() - followSquad_->position2D();
			point.normalize((radius() + followSquad_->radius()) * 1.5f);
			point += followSquad_->position2D();
			addWayPointS(point);
		}
	}
}

void FormationController::quant()
{
	if(units_.activeSize() == 0)
		return;

	FormationUnits::iterator ui;
	FOR_EACH_ACTIVE(units_, ui)
		(*ui)->checkPenetration();

	if(wayPoints().size() > 1 && isMidPointReached(wayPoints().front()))
		wayPoints_.erase(wayPoints_.begin());

	ptAction_.checkFinish(this);

	if(!controlled()){
		stopQuants_ = 0;
		stopMoving();
		if(personalWayPointsEmpty())
			wayPointsClear();
	}else if(stopQuants_ && stopQuants_ < 4){
		++stopQuants_;
		stopMoving();
	}else{
		stopQuants_ = 0;

		velocityProcessing();

		FOR_EACH_ACTIVE(units_, ui)
			(*ui)->quant();

		if(position().distance2(wayPoint()) > sqr(leader()->rigidBody_->prm().is_point_reached_radius_max))
			pathTracking();
		else{
			FOR_EACH_ACTIVE(units_, ui)
				if((*ui)->unitMove())
					(*ui)->setRotationSpeed(correctUnitDirection(*ui));
		}
	}
	FOR_EACH_ACTIVE(units_, ui)
		(*ui)->postQuant(moveback());
}

///////////////////////////////////////////////////////////////

void FormationController::serialize(Archive& ar)
{
	ar.serialize(wayPoint_, "wayPoint", 0);
	ar.serialize(freePositions_, "freePositions", 0);
	ar.serialize(followSquad_, "followSquad", 0);
}

///////////////////////////////////////////////////////////////

void FormationController::showDebugInfo()
{
	if(showDebugRigidBody.wayPoints){
		Vect3f pos = To3D(position());
		Vect3f w = To3D(wayPoint());
		show_line(pos, w, Color4c::BLUE);
		show_vector(w, 1, Color4c::BLUE);
		if(!wayPointEmpty()){
			Vect3f pos = To3D(position());
			Vect3f w = To3D(lastWayPoint());
			show_line(pos, w, Color4c::GREEN);
			show_vector(w, 1, Color4c::GREEN);
		}
		if(!wayPoints().empty()){
			vector<Vect2f>::const_iterator i;
			FOR_EACH(wayPoints(), i){
				Vect3f p = To3D(*i);
				show_line(pos, p, Color4c::YELLOW);
				show_vector(p, 1, Color4c::YELLOW);
				pos = p;
			}
		}
	}

	if(showDebugSquad.squadToFollow && followSquad_)
		show_line(To3D(position()), followSquad_->position(), Color4c::MAGENTA);

	if(showDebugRigidBody.showPathTracking_VehicleRadius){
		float rad = velocityMaxPermanent() * logicPeriodSeconds / pathTrackingAngle();
		Vect2f n1(-rad, 0.0f);
		MatX2f pose(Mat2f((float)orientation()), position());
		n1 *= pose;
		show_vector(To3D(n1), rad, Color4c::RED);
		Vect2f n2(rad, 0.0f);
		n2 *= pose;
		show_vector(To3D(n2), rad, Color4c::RED);
	}

	if(showDebugRigidBody.autoPTPoint && ptAction_.active())
		show_line(To3D(position()), To3D(ptAction_.point()), Color4c::GREEN);

#ifndef _FINAL_VERSION_
	if(showDebugRigidBody.showDebugMessages) {
		XBuffer buf;
		buf <= universe()->quantCounter() < ":";
		string temp = debugMessageList_;
		temp += buf;
		show_text(To3D(position()), temp.c_str(), Color4c::WHITE);
	}

#endif
}

///////////////////////////////////////////////////////////////

#ifndef _FINAL_VERSION_
void FormationController::debugMessage(const char* text)
{
	XBuffer buf;
	buf <= universe()->quantCounter() < ": ";
	debugMessageList_ += buf;
	debugMessageList_ += text;
	debugMessageList_ += '\n';

	int numLines = count(debugMessageList_.begin(), debugMessageList_.end(), '\n');

	if(numLines > showDebugRigidBody.debugMessagesCount)
		debugMessageList_.erase(0, debugMessageList_.find('\n') + 1);

}
#endif

///////////////////////////////////////////////////////////////

/*void FormationController::computeManualDirection()
{
if(moveback())
return -manualPTLine * pathTrackingAngle_ / LINES_NUM;
return manualPTLine * pathTrackingAngle_ / LINES_NUM;
}

void FormationController::pathTrackingManual(const ObstacleList& obstacleList)
{
if(manualMoving()){
MovementDirection preferedDirection = computeManualDirection();
if(checkPassability(preferedDirection, obstacleList)){
moveForward(preferedDirection);
}else{
debugMessage("PT: ��������� ����������� � ������ ����������");
PossibleSector sector = computeForwardSector();
PossibleMovementDirections directions(sector, 15);
computeMoveLinesPassability(directions, obstacleList);
PossibleMovementDirection maxLine = directions.getOptimalDirection(directions.checkLeftBeterThenRight());
if(maxLine.isFullyPassable())
moveForward(maxLine.direction());
}
retrun;
}
}*/
