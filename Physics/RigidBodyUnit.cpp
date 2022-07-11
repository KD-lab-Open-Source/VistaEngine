#include "stdafx.h"
#include "RigidBodyPrm.h"
#include "RigidBodyTrail.h"
#include "GlobalAttributes.h"
#include "PFTrap.h"
#include "SecondMap.h"
#include "normalMap.h"
#include "WhellController.h"
#include "PositionGeneratorCircle.h"
#include "UnitActing.h"

REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_UNIT, RigidBodyUnit)

//=======================================================
namespace {

	Vect2f temp_localwpoint; // Для функции sort()
	
	int maxLineNum;
	const Vect2f* maxLinePointer;
	float maxLineI = 0.f;

	const float DT = 0.1f;
	const int BACK = 6;
	const int STOP = 10;
}

//=======================================================

#define NO_PATH 100
#define NO_OBSTACLES -1

inline bool isFreeLine(RigidBodyUnit * vehicle, vector<RigidBodyUnit*>& obstacleList,  int index);
void ImpassabilityCheck(RigidBodyUnit * vehicle);
__forceinline bool collidingObstacleList(const RigidBodyBase* vehicle, const MatX2f& vehiclePose, vector<RigidBodyBase*>& obstacleList, bool firstStep);

//=======================================================

void RigidBodyUnit::clearPathTracking()
{
	maxLineI = 0;
	maxLineNum = 0;
	temp_localwpoint = Vect2f::ZERO;
	maxLinePointer = 0;
}

//=======================================================

class StopFunctor {
public:

	bool newWpointEnable;

	StopFunctor(const UnitBase* unit_, Vect2f& wpoint_):unit(unit_), wpoint(wpoint_) {
		maxDelta = -10000.0f;
		newWpointEnable = false;
	}

	bool checkRadius(UnitBase* p) {
		Vect2f norm = wpoint - unit->position2D();
		norm.set(norm.y, -norm.x);
		Vect2f oDir = p->position2D() - unit->position2D();
		float dist = fabs(norm.dot(oDir)/norm.norm());
		return dist < unit->radius() * 1.2f + p->radius();
	}

	void operator () (UnitBase* p) {

		if(p == unit || !p->checkInPathTracking(unit) || p->isDocked()) 
			return;
		
		if(p->position2D().distance2(unit->position2D()) < sqr(p->radius() + unit->radius()))
			return;

		if(!p->rigidBody()->isUnit())
			return;

		if(safe_cast<RigidBodyUnit*>(p->rigidBody())->controlled() || !safe_cast<UnitReal*>(p)->wayPoints().empty())
			return;

		if(!checkRadius(p))
			return;

		Vect2f unitDir = unit->position2D() - wpoint;
		Vect2f obstacleDir = p->position2D() - wpoint;
		float distDelta = unitDir.dot(obstacleDir) / unitDir.norm();
		distDelta += p->radius();
		distDelta += unit->radius();

		if(distDelta < 0 || distDelta > unitDir.norm())
			return;

		newWpointEnable = true;
		if(distDelta > maxDelta)
			maxDelta = distDelta;
	}

	Vect2f newWpoint() {
		Vect2f unitDir = wpoint - unit->position2D();
		float dist = unitDir.norm();
		
		if(dist < maxDelta)
			return unit->position2D();
		
		dist -= maxDelta;
		unitDir.Normalize(dist);
		return unit->position2D() + unitDir;
	}

private:

	Vect2f wpoint;
	const UnitBase* unit;
	float maxDelta;
};

//=======================================================
//	PathTracking functor.
//	Обработка препятствий + заставляет юнитов уступать дорогу.
//=======================================================
class UnitMovePlaner 
{
public:

	bool removeWPoint;
	Vect3f newWPoint;
	float ignoreRadius; // Пороговый радиус.
	bool enemyIgnoreRadius; // Враги игнорируют меньших?
	vector<RigidBodyBase*> obstacleList;
	RigidBodyUnit* trackerBody;
	bool waitLeftObstacle_;

	UnitMovePlaner(const UnitBase& tracker, const UnitBase* ignoreUnit, Vect2f& wpoint_, bool checkRadius, bool enableMakeWay, bool enemyMakeWay = true)
	: tracker_(tracker), checkRadius_(checkRadius), ignoreUnit_(ignoreUnit), enemyMakeWay_(enemyMakeWay), enableMakeWay_(enableMakeWay), sf(&tracker, wpoint_){ 
		maxLine_ = NO_OBSTACLES; 
		removeWPoint = false;
		wpoint = wpoint_;
		newWPoint = Vect3f::ZERO;
		ignoreRadius = 0;
		bool enemyIgnoreRadius = true;
		trackerBody = safe_cast<RigidBodyUnit*>(tracker_.rigidBody());
		waitLeftObstacle_ = false;
	}

	Vect2f callcFlowVector() {
		Vect2f vehicleDir(1,0);
		vehicleDir *= trackerBody->ptPose().rot;

		if(obstacleList.empty())
			return vehicleDir;

		float distMin = 1e10;
		Vect2f retVector(Vect2f::ZERO);
		vector<RigidBodyBase*>::iterator oi;
		FOR_EACH(obstacleList, oi){
			float dist = (*oi)->position2D().distance(trackerBody->position2D()) - trackerBody->radius() - (*oi)->radius();
			if(dist < distMin) {
				distMin = dist;
				Vect2f obstacleDir = trackerBody->position2D() - (*oi)->position2D();
				Vect2f n1(-obstacleDir.y, obstacleDir.x);
				Vect2f n2(obstacleDir.y, -obstacleDir.x);
				Vect2f flow;
				if(n1.dot(vehicleDir) > 0)
					flow = n1;
				else
					flow = n2;
				flow.Normalize(trackerBody->ptVelocity * DT);
				Vect2f newFlow = obstacleDir + flow;
				newFlow.Normalize(obstacleDir.norm());
				
				retVector = newFlow - obstacleDir;
			}
		}

		return retVector;
	}

	void makeWay(UnitBase* p) {

		if(!p->rigidBody()->isUnit())
			return;

		RigidBodyUnit* rigidBodyUnit = safe_cast<RigidBodyUnit*>(p->rigidBody());

		if(rigidBodyUnit->isManualMode())
			return;

		if(p->attr().isEnvironment() || !rigidBodyUnit->prm().controled_by_points || !rigidBodyUnit->enablePathTracking_) 
			return;
		
		if(p->attr().isResourceItem() || p->attr().isInventoryItem())
			return;

		if(p->attr().isActing() && safe_cast<UnitActing*>(p)->fireDistanceCheck())
			return;

		bool enemyTest = enemyMakeWay_ || !p->isEnemy(&tracker_);
		if(enemyTest && rigidBodyUnit->way_points.empty() && !trackerBody->ptAction_.isMove() && tracker_.radius() > p->radius()) {
			Vect3f movDir(Vect3f::I);
			tracker_.pose().xformVect(movDir);
            Vect3f poseDelta = p->position() - tracker_.position();
			tracker_.pose().invXformVect(poseDelta);
			if(poseDelta.y > 0 && fabs(poseDelta.x) <= (p->radius()+tracker_.radius())) {
				rigidBodyUnit->enableRunMode_ = true;
				rigidBodyUnit->debugMessage("PT: Уступаю дорогу");
				rigidBodyUnit->disableRotationMode();
				if(poseDelta.x > 0)
					rigidBodyUnit->turnMoveAction(p->position() + movDir*(2.0f*p->radius()+tracker_.radius()), RigidBodyUnit::ACTION_PRIORITY_PATH_TRACKING);
				else
					rigidBodyUnit->turnMoveAction(p->position() - movDir*(2.0f*p->radius()+tracker_.radius()), RigidBodyUnit::ACTION_PRIORITY_PATH_TRACKING);
			}
		}
	}

	void operator () (UnitBase* p) {
	
		if(!p->alive() || !p->rigidBody())
			return;

		if(p == &tracker_ || !p->checkInPathTracking(&tracker_) || p == ignoreUnit_ || p->isDocked()) 
			return;

		if(p->rigidBody() == trackerBody->trail_ && trackerBody->unitOnTrail_)
			return;
        
		if(tracker_.rigidBody()->isTrail() && p->rigidBody()->isUnit() && safe_cast<RigidBodyUnit*>(p->rigidBody())->trail() == safe_cast<RigidBodyTrail*>(tracker_.rigidBody()))
			return;

		makeWay(p);

		if(checkRadius_ && (enemyIgnoreRadius || !tracker_.isEnemy(p)))
			if(tracker_.radius() > ignoreRadius && p->radius() <= ignoreRadius)
				return;

		// Проверка на движущаюся помеху слева.
		if(temp_localwpoint.y > 0 && p->rigidBody()->isUnit()){
			RigidBodyUnit* obstacleBody = safe_cast<RigidBodyUnit*>(p->rigidBody());
			if(obstacleBody->unitMove() && !obstacleBody->isBoxMode() && obstacleBody->position2D().distance2(trackerBody->position2D()) < sqr(1.5f*obstacleBody->radius() + 1.5f * trackerBody->radius())){
				Vect2f obstaclePose(p->position2D());
				obstaclePose = trackerBody->ptPose().invXform(obstaclePose);
				if(obstaclePose.y > 0){
					Vect2f obstacleDir(1,0);
					Vect2f trackerDir(1,0);
					obstacleDir *= obstacleBody->ptPose().rot;
					trackerDir *= trackerBody->ptPose().rot;
					if(obstacleDir.dot(trackerDir) > 0.8f){
						waitLeftObstacle_ = true;
						return;
					}
				}
			}
		}

		bool penetrationFound = false;

		if((trackerBody->ptObstaclesCheck_ || p->rigidBody()->prm().controled_by_points) && !p->rigidBody()->bodyIntersect(trackerBody, trackerBody->ptPose()))
			obstacleList.push_back(p->rigidBody());
		else {
			penetrationFound = true;
			if(trackerBody->ptObstaclesCheck_ && p->rigidBody()->prm().controled_by_points && !trackerBody->ptAction_.isMove()) {
				Vect2f wpoint = tracker_.position2D() - p->position2D();
				wpoint.Normalize(2.0f * tracker_.radius());
				wpoint += tracker_.position2D();
				Vect2f forwardDir(3.0f * tracker_.radius(),0);
				forwardDir *= trackerBody->ptPose().rot;
				wpoint += forwardDir;
				trackerBody->turnMoveAction(To3D(wpoint), RigidBodyUnit::ACTION_PRIORITY_PATH_TRACKING);
			}
		}

		if(p->position2D().distance2(wpoint) < sqr(p->radius() + 0.5f * tracker_.radius())
			&& tracker_.position2D().distance2(wpoint) < sqr(p->radius() + 1.4f * tracker_.radius())) {
			Vect2f point = p->position2D() + Vect2f(p->radius() + 1.5f * tracker_.radius(),0);
			if(trackerBody->stopFunctorFirstStep != 0)
				newWPoint = tracker_.position();
			else 
				newWPoint = Vect3f(point.x, point.y, 0);
			removeWPoint = true;
		}

		if(tracker_.position2D().distance2(wpoint) < sqr(tracker_.attr().getPTStopRadius()) &&
			safe_cast<const UnitReal*>(&tracker_)->getUnitState() == UnitReal::MOVE_MODE && !penetrationFound){
			sf(p);
			if(sf.newWpointEnable && (trackerBody->stopFunctorFirstStep == 20)) {
				newWPoint = To3D(sf.newWpoint());
				removeWPoint = true;
				trackerBody->stopFunctorFirstStep = 0;
				trackerBody->debugMessage("PT:StopFunctor 2");
			}
			else if(sf.newWpointEnable && (trackerBody->stopFunctorFirstStep == 0)) {
				trackerBody->stopFunctorFirstStep = 1;
				trackerBody->debugMessage("PT:StopFunctor 1");
			}
		}
	}

	int freeLine() const { return maxLine_; }

	void callFreeLine() {

		if(obstacleList.empty())
			return;

		maxLineI = maxLineNum = -1;

		for(int i=0; i<(RigidBodyUnit::LINES_NUM*2+1); i++)
			if(isFreeLine(trackerBody, obstacleList, i)) {
				maxLine_ = i;
				return;
			}

//		if(!trackerBody->isManualMode())
			if(maxLineI >= 0)
				maxLine_ = maxLineNum;
			else
				maxLine_ = NO_PATH;

		maxLine_ = NO_PATH;
	}

private:
	const UnitBase& tracker_;
	const UnitBase* ignoreUnit_;
	StopFunctor sf;
	Vect2f wpoint;
	int maxLine_;
	bool checkRadius_; // Использовать пороговое значение для разделения по радиусам?.
	bool enemyMakeWay_; // Враги уступают дорогу?.
	bool enableMakeWay_; // Уступать дорогу?.
};

//=======================================================
bool linesIntersect(const Vect2f& l1p1, const Vect2f& l1p2, const Vect2f& l2p1, const Vect2f& l2p2)
{
	Vect2f l1norm(l1p2-l1p1);
	l1norm.set(l1norm.y, -l1norm.x);
    
	Vect2f vpp1(l2p1 - l1p1);
	Vect2f vpp2(l2p2 - l1p1);

	if(l1norm.dot(vpp1) * l1norm.dot(vpp2) > 0)
		return false;

	Vect2f l2norm(l2p2-l2p1);
	l2norm.set(l2norm.y, -l2norm.x);
    
	vpp1 = l1p1 - l2p1;
	vpp2 = l1p2 - l2p1;

	if(l2norm.dot(vpp1) * l2norm.dot(vpp2) > 0)
		return false;

    return true;
}

//=======================================================
//	Рассчитывает линии для PathTracking-а по значению угла.
//=======================================================
void RigidBodyUnit::callcPathTrackingLines()
{

	float dangle = path_tracking_angle/LINES_NUM;
	for(int i=0;i<(LINES_NUM*2+1);i++){
		lines[i].x = cos(-path_tracking_angle+dangle*i);
		lines[i].y = sin(-path_tracking_angle+dangle*i);
		backlines[i].x = -cos(-path_tracking_angle+dangle*i);
		backlines[i].y = sin(-path_tracking_angle+dangle*i);
	}
}

RigidBodyUnit::RigidBodyUnit()
{
	back_quants = 0;
	stop_quants = 0;
	wpoint = Vect2f::ZERO;
	ptPose_ = MatX2f::ID;
	ptVelocity = 0;
	path_tracking_angle = 0;
	onWater = false;
	onLowWater = false;
	onDeepWater = false;
	deepWaterChanged = false;
	onWaterPrev = false;
	onSecondMap = false;
	onIce = false;
	isFrozen_ = false;
	isUnderWaterSilouette = false;
	isUnderWaterSilouettePrev = false;
	onImpassability = false;
	onImpassabilityFull = false;
	enablePathTracking_ = false;
	rotationMode = false;
	rotationSide = ROT_LEFT;
	enableRunMode_ = false;
	ignoreUnit_ = 0;
	isAutoPTRotation = false;
	manualViewPoint = Vect3f::ZERO;
	lastPTAngle = 0;
	unmovableXY_ = false;
	prevVelocity_ = Vect3f::ZERO;
	manualMoving = false;
	manualPTLine = 0;
	awakeCounter_ = 0;
	ptDirection_ = PT_DIR_FORWARD;
	pointAreaAnalize_ = false;
	curVerticalFactor_ = 1.0f;

	groundZ = 0;
	groundNormal = Vect3f::ZERO;
	ptPoseAngle = 0;
	ptAdditionalRot = QuatF::ID;
	whellController = 0;
	flyDownMode = false;
	flyDownSpeed_ = 0.0f;
	flyDownTime = 1000;
	flyingHeightCurrent_ = 0.0f;

	ptObstaclesCheck_ = true;
	ptImpassabilityCheck_ = true;
	sumAvrVelocity_ = Vect2f::ID;
	sumAvrVelocityCount_ = 0;
	obstaclesFound_ = false;
	isBoxMode_ = false;
	ptPose_ = MatX2f::ID;
	ownerUnit_ = 0;
	stopFunctorFirstStep = 0;
	trail_ = 0;
	unitOnTrail_ = false;
	localPose = Vect3f::ZERO;
	rigidBodyType = RIGID_BODY_UNIT;
	waterLevelPrev = 0.0f;
}

void RigidBodyUnit::build(const RigidBodyPrm& prmIn, const Vect3f& boxMin_, const Vect3f& boxMax_, float mass)
{
	__super::build(prmIn, boxMin_, boxMax_, mass);

	setColor(CYAN);

	setForwardVelocity(prm().forward_velocity_max);
	setFlyingHeight(prm().flying_height);

	colliding_ = 0;
	flying_mode = 0;
	flyingModePrev = 0;
	back_quants = 0;
	stop_quants = 0;
	moveback = false;

	flyingHeight_ = prm().flying_height;
	ptObstaclesCheck_ = prm().ptObstaclesCheck;
	ptImpassabilityCheck_ = prm().ptImpassabilityCheck;
}

void RigidBodyUnit::initPose(const Se3f& pose)
{
	setPose(pose);
	

	if(flyDownMode || isBoxMode()){
		posePrev_ = pose_;
		return;
	}

	float groundZNew = 0;
	checkGround();
	QuatF groundQuat = placeToGround(groundZNew);

	groundZ = groundZNew;
	if(flyingMode()){
		pose_.trans().z = groundZ + flyingHeightCurrent_;
		awake();
	}
	else {
		colliding_ = onWater ? WATER_COLLIDING : GROUND_COLLIDING;
		pose_.trans().z = groundZ;
	}

	if(!prm().sleepedAtStart)
		awake();
	
	posePrev_ = pose_;
}

bool RigidBodyUnit::evolve(float dt)
{
	start_timer_auto();

	if(isBoxMode()){
		if(!asleep() || unmovable()){
			if(!isFrozen())
				isUnderWaterSilouette = onWater && (environment->water()->GetZFast(position().xi(), position().yi()) - position().z) > 2.0f * extent().z;
			__super::evolve(dt);
			return true;
		}
		disableBoxMode();
		if(safe_cast<const UnitReal*>(&ownerUnit())->currentState() == CHAIN_FALL){
			makeStaticXY();
		}else{
			enableWaterAnalysis();
			setFlyingMode(prm().flyingMode);
		}
	}

	if(trail_)
		awake();

	ptAction_.checkFinish(this);

	if(!onSecondMap && !prm().flyingMode)
		checkGround();
	else{
		onLowWater = false;
		onWaterPrev = onWater;
		onWater = false;
		onIce = false;
		isFrozen_ = false;
	}
	// Течение воды.
	if(onWater && waterWeight() && !isUnderEditor()) {
		Vect2f waterVelocity(environment->water()->GetVelocity(pose().trans().xi(), pose().trans().yi()));
		waterVelocity *= waterWeight();
		if(waterVelocity.norm2() > 1e-5f){
			pose_.trans() += Vect3f(waterVelocity, 0.0f);
			awake();
		}
	}
	
	if(asleep() && controlled() && !unmovable())
		awake();

	if(asleep() && !isFrozen() && onWater){
		float waterLevel = environment->water()->GetZFast(position().xi(), position().yi());
		if(waterLevel - waterLevelPrev < 1e-2f){
			waterLevelPrev = waterLevel;
			awake();
		}
	}

	RigidBodyBase::evolve(dt);
	
	QuatF prevOrientation = orientation();
	posePrev_ = pose();
	if(!unmovable() && !isFrozen() && (!asleep() || isFlyDownMode())){ 
		ptUnitEvolve();
		return true;
	}
	return false;

}

void RigidBodyUnit::show()
{
	__super::show();

	if(showDebugRigidBody.wayPoints && !way_points.empty()){
		Vect3f pos = To3D(position());
		Vect3fList::iterator i;
		FOR_EACH(way_points, i){
			Vect3f p=To3D(*i);
			show_line(pos, p, YELLOW);
			show_vector(p, 1, YELLOW);
			pos = p;
		}
	}

	if(showDebugRigidBody.autoPTPoint && ptAction_.active())
		show_line(position(), ptAction_.point(), GREEN);

	if(showDebugRigidBody.localFrame){
		show_vector(position(), rotation().xcol(), X_COLOR);
		show_vector(position(), rotation().ycol(), Y_COLOR);
		show_vector(position(), rotation().zcol(), Z_COLOR);
		}

	if(showDebugRigidBody.ptVelocityValue){
		XBuffer buf;
		buf.SetDigits(2);
		buf <= ptVelocity;
		show_text(position(), buf, MAGENTA);
	}

	if(showDebugRigidBody.average_movement){
		XBuffer buf;
		buf.SetDigits(2);
		float movement = averagePosition_.distance2(position()) + averageOrientation_.distance2(Vect3f(orientation()[0], orientation()[1], orientation()[2]))*100;
		buf <= movement;
		show_text(position(), buf, YELLOW);
	}

	if(showDebugRigidBody.ground_colliding){
		XBuffer buf;
		buf < "colliding: " <= colliding();
		show_text(position(), buf, YELLOW);
	}
	
	if(showDebugRigidBody.onLowWater){
		XBuffer buf;
		buf < "On: " < (onSecondMap ? "secondMap " : "") < (onWater ? "water " : "") < (onLowWater ? "lowWater " : "") < (onIce ? "ice " : "") < (isFrozen() ? "frozen " : "");
		show_text(position(), buf, YELLOW);
	}
	
	if(showDebugRigidBody.showMode && onSecondMap)
		show_text(position(), "onSecondMap", YELLOW);
}

bool RigidBodyUnit::is_point_reached(const Vect2f& point)
{ 
	Vect2f p0 = posePrev().trans();
	Vect2f p1 = position();
	Vect2f axis = p1 - p0;
	float norm2 = axis.norm2();
	float dist2 = 0;
	if(norm2 < 0.1){
		dist2 = p0.distance2(point);
	}
	else{
		Vect2f dir = point - p0;
		float dotDir = dot(axis, dir);
		if(dotDir < 0)
			dist2 = p0.distance2(point);
		else if(dot(axis, point - p1) > 0)
			dist2 = p1.distance2(point);
		else
			dist2 = (dir - axis*(dotDir/norm2)).norm2();
	}

	rotationMode = ownerUnit().attr().enableRotationMode?!(!rotationMode && !(dist2 < sqr(prm().is_point_reached_radius_max))):false;
	bool reached = dist2 < sqr(0.7*radius() + prm().is_point_reached_radius_max);
	if(reached)
		stopFunctorFirstStep = 0;
	return reached;
}

bool RigidBodyUnit::is_point_reached_mid(const Vect2f& point)
{ 
	bool reached = position2D().distance2(point) < sqr(prm().is_point_reached_radius_max + forwardVelocity() * 0.1f);
	if(reached)
		stopFunctorFirstStep = 0;
	return reached;
}

bool RigidBodyUnit::groundCheck( int xc, int yc, int r ) const
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

bool RigidBodyUnit::submerged() const 
{ 
	if(!environment->water())
		return false;
	
	cWater& water = *environment->water();

	int xc = round(position().x);
	int yc = round(position().y);
	float z = position().z;
	int r = round(radius()/2); 

	int xL=(xc-r)>>water.GetCoordShift();
	int xR=(xc+r)>>water.GetCoordShift();
	int yT=(yc-r)>>water.GetCoordShift();
	int yD=(yc+r)>>water.GetCoordShift();
	
	if(xL < 0)
		xL = 0;
	if(xR >= water.GetGridSizeX())
		xR = water.GetGridSizeX() - 1;
	if(yT < 0)
		yT = 0;
	if(yD >= water.GetGridSizeY())
		yD = water.GetGridSizeY() - 1;

	for(int y=yT; y<=yD; y++){
		for(int x=xL; x<=xR; x++){
			const cWater::OnePoint& op = environment->water()->Get(x,y);
			if(op.z && op.realHeight() > z)
				return true;
		}
	}
	return false;
}

//=======================================================
// Если помещается на втором уровне - true.
//=======================================================
bool RigidBodyUnit::secondMapCheck( int xc, int yc, int r ) const
{
	if(!universe()->secondMap->mapPresent())
		return false;

	int xL = universe()->secondMap->w2mFloor(xc-r);
	int xR = universe()->secondMap->w2mFloor(xc+r);
	int yT = universe()->secondMap->w2mFloor(yc-r);
	int yD = universe()->secondMap->w2mFloor(yc+r);

	int x,y;
	int a=0;
	for(y=yT; y<=yD; y++){
		for(x=xL; x<=xR; x++){
			a = (*universe()->secondMap)(x,y);
			if(!a) return false;
		}
	}
	return true;
}

//=======================================================
// Если сталкивается со вторым уровнем - true.
//=======================================================
bool RigidBodyUnit::secondMapNegativeCheck( int xc, int yc, int r ) const
{
	if(!universe()->secondMap->mapPresent())
		return false;

	int xL = universe()->secondMap->w2mFloor(xc-r);
	int xR = universe()->secondMap->w2mFloor(xc+r);
	int yT = universe()->secondMap->w2mFloor(yc-r);
	int yD = universe()->secondMap->w2mFloor(yc+r);

	int x,y;
	int a = 0;
	Vect3f maxPoint = extent();
	orientation().xform(maxPoint);
	maxPoint.add(position());
	for(y=yT; y<=yD; y++){
		for(x=xL; x<=xR; x++){
			if((*universe()->secondMap)(x,y).getElevator()) continue;
			a = (*universe()->secondMap)(x,y);
			if(!a) continue;
			if(a < maxPoint.z ) return true;
		}
	}
	return false;
}

bool RigidBodyUnit::checkImpassability(const Vect3f& pos, float radius) const
{
	if(pos.x-(radius+2.0f)<0 || pos.x+(radius+2.0f) >= vMap.H_SIZE) return false;
	if(pos.y-(radius+2.0f)<0 || pos.y+(radius+2.0f) >= vMap.V_SIZE) return false;

	bool passability = true;
	bool ground = true;
	bool water = true;
	bool second_map = false;
	bool ptEnvironment = true;

//	bool curPtEnvironment = !pathFinder->checkFlag((~unsigned int(ownerUnit().attr().environmentDestruction)) & 0x0000FFFF, position().xi(), position().yi());
//	if(curPtEnvironment)
//		ptEnvironment = !pathFinder->checkFlag((~unsigned int(ownerUnit().attr().environmentDestruction)) & 0x0000FFFF, pos.xi(), pos.yi());

	if(prm().waterPass == RigidBodyPrm::IMPASSABILITY)
//		water = waterCheck(pos.xi(), pos.yi(), round(radius));
		water = !checkWater(pos.xi(), pos.yi(), 0);

	if(prm().impassabilityPass == RigidBodyPrm::IMPASSABILITY)
//		passability = onImpassability  || (!water) ? true : (!pathFinder->impassabilityCheck(pos.xi(), pos.yi(), 8.0f));
		passability = onImpassability || (!pathFinder->impassabilityCheck(pos.xi(), pos.yi(), (water ? 4.0f : 7.0f)));

	if(prm().groundPass == RigidBodyPrm::IMPASSABILITY)
		ground = groundCheck(pos.xi(), pos.yi(), round(radius));

	if(prm().groundPass == RigidBodyPrm::PASSABILITY)
		second_map = secondMapCheck(pos.xi(), pos.yi(), round(radius));

	if(!onSecondMap && universe()->secondMap->mapPresent()){
		int smx = universe()->secondMap->w2mFloor(pos.xi());
		int smy = universe()->secondMap->w2mFloor(pos.yi());
		bool elevator = (*universe()->secondMap)(smx, smy).getElevator();
		if(!elevator && secondMapNegativeCheck(pos.xi(), pos.yi(), round(radius)))
			return false;
	}

	return  ((ptEnvironment && passability && ground && water && !onSecondMap) || (second_map && onSecondMap));
}

bool RigidBodyUnit::checkImpassabilityStatic(const Vect3f& pos) const
{
	if(pos.x-radius()<0 || pos.x+radius() >= vMap.H_SIZE) return false;
	if(pos.y-radius()<0 || pos.y+radius() >= vMap.V_SIZE) return false;

	bool passability = true;
	bool ground = true;
	bool water = true;
	bool second_map = false;

	if(prm().waterPass == RigidBodyPrm::IMPASSABILITY)
//		water = waterCheck(pos.xi(), pos.yi(), round(radius()));
		water = !checkWater(pos.xi(), pos.yi(), 0);

	if(prm().impassabilityPass == RigidBodyPrm::IMPASSABILITY)
		passability = !water ? true : !pathFinder->impassabilityCheck(pos.xi(), pos.yi(), 6.0f);

	if(prm().groundPass == RigidBodyPrm::IMPASSABILITY)
		ground = groundCheck(pos.xi(), pos.yi(), round(radius()));

//	if(prm().groundPass == RigidBodyPrm::PASSABILITY)
//		second_map = secondMapCheck(pos.xi(), pos.yi(), round(radius()));

	return passability && ground && water || second_map;
}

//=======================================================
// Растояние от точки до линии..
//=======================================================
float pointToLineDistance(const Vect3f& p, const Vect3f& l1, const Vect3f& l2 )
{
	Vect3f diff(p - l1);
	Vect3f m(l2 - l1);
	float t = m.dot(diff) / m.dot(m);
	if(t<0) {
//		midPoint = l1;
		return l1.distance(p);
	}
	if(t>1) {
//		midPoint = l2;
		return l2.distance(p);
	}

//	midPoint = l1 + m*t;
	diff = diff - m*t;
	return diff.norm();
};


void RigidBodyUnit::wayPointsChecker()
{
	if(way_points.size() > 1) {
		float dist = pointToLineDistance(position(), way_points[0], way_points[1]);
		
		if(dist < way_points[0].distance(position()))
			way_points.erase(way_points.begin());
	}
}

void RigidBodyUnit::enableWaterAnalysis() 
{ 
	setWaterAnalysis(prm().waterAnalysis);
	onDeepWater = false;
	deepWaterChanged = false;
	if(!flying_mode && !flyDownMode){
		groundZ += flyingHeightCurrent_;
		flyingHeightCurrent_ = 0;
	}
}

void RigidBodyUnit::disableWaterAnalysis() 
{ 
	waterAnalysis_ = false;
	onDeepWater = false;
	deepWaterChanged = false;
	groundZ += flyingHeightCurrent_;
	flyingHeightCurrent_ = 0;
}

void RigidBodyUnit::setFlyingMode(bool mode, int time) 
{	
	flying_mode = mode;
	if(flying_mode){
		deepWaterChanged = false;
		colliding_ = 0;
	}
	if(flyingModePrev != flyingMode()){
		flyingModePrev = flyingMode();
		enableFlyDownModeByTime(time ? time : flyDownTime);
	}
}

void RigidBodyUnit::setPose(const Se3f& pose)
{
	__super::setPose(pose);
	
	if(flyingMode())
		groundZ = position().z - flyingHeightCurrent_;
	else
		groundZ = position().z;

	ptPoseAngle = -atan2f(rotation()[0][1], rotation()[1][1]);
}

void RigidBodyUnit::callcVerticalFactor()
{
	curVerticalFactor_ = 1.0;

	if(onImpassabilityFull)
		return;

	if(onSecondMap || (onWater && prm().waterAnalysis))
		return;

	if(ownerUnit().attr().enableVerticalFactor) {
		
		Vect3f dir(Vect3f::J);
		Se3f ptPose_ = pose();

		float ptAngleDelta = 0;
		switch(ptDirection()){
		case PT_DIR_RIGHT:
			ptAngleDelta = M_PI/2.0f;
			break;
		case PT_DIR_LEFT:
			ptAngleDelta = -M_PI/2.0f;
			break;
		case PT_DIR_BACK:
			ptAngleDelta = M_PI;
			break;
		}
		
		if(moveback)
			ptAngleDelta += M_PI;

		QuatF r2(ptAngleDelta, Vect3f(0,0,1));
		ptPose_.rot().postmult(r2);
		ptPose_.xformVect(dir);

		dir.z = 0;
		dir.Normalize();
		Vect3f terNormal = normalMap->normalLinear(position().x, position().y);
		float dotVect = dir.dot(terNormal);
		curVerticalFactor_ = 1 + prm().verticalFactor*dotVect;
	}
}

void RigidBodyUnit::computeUnitVelocity()
{
	velocity_.linear().set(0, ptVelocity, 0);
	pose().xformVect(velocity_.linear());
}

const Vect3f& RigidBodyUnit::velocity()
{ 
	if(!isBoxMode())
		computeUnitVelocity();
	return velocity_.getLinear();
}

void RigidBodyUnit::enableFlyDownModeByTime(int time) 
{ 
	flyDownMode = true;
	flyDownSpeed_ = flyingHeightCurrent_;
	if(flyingMode())
		flyDownSpeed_ -= flyingHeight();
	flyDownSpeed_ *= logicTimePeriod / (float)time;
	if(fabsf(flyDownSpeed_) <= FLT_EPS){
		flyDownMode = false;
		flyingHeightCurrent_ = flyingMode() ? (prm().flying_height_relative ? flyingHeight() : flyingHeight() - groundZ) : 0.0f;
	}
}

bool RigidBodyUnit::getTrailPoint( const Vect2f position, float& z )
{
	Vect3f A(position.x, position.y, 0);
	Vect3f B(Vect3f::K);

	trail_->pose().invXformPoint(A);
	trail_->pose().invXformVect(B);

	float t = (trail_->boxMax().z - A.z)/B.z;
	Vect3f point = A + B * t;

	if(point.x < trail_->boxMin().x || point.x > trail_->boxMax().x) return false;
	if(point.y < trail_->boxMin().y || point.y > trail_->boxMax().y) return false;

	trail_->pose().xformPoint(point);
	z = point.z;

	return true;
}

Vect3f RigidBodyUnit::xformTrailToGlobal( const Vect2f position )
{
	Vect3f A(position.x, position.y, 0);
	Vect3f B(Vect3f::K);

	trail_->pose().invXformVect(B);

	float t = (trail_->boxMax().z - A.z)/B.z;

	Vect3f point = A + B * t;
	trail_->pose().xformPoint(point);

	return point;
}

bool RigidBodyUnit::pointInTrail( const Vect2f& point, const Se3f& trailPose )
{
	Vect3f A(point.x, point.y, 0);
	Vect3f B(Vect3f::K);

	trailPose.invXformPoint(A);
	trailPose.invXformVect(B);

	float t = (trail_->boxMax().z - A.z)/B.z;
	Vect3f pointL = A + B * t;

	if(pointL.x < trail_->boxMin().x + radius() || pointL.x > trail_->boxMax().x - radius()) return false;
	if(pointL.y < trail_->boxMin().y + radius() || pointL.y > trail_->boxMax().y - radius()) return false;

	return true;
}

void RigidBodyUnit::setDeepWater(bool deepWater)
{
	if(!flying_mode && onDeepWater != deepWater)
		deepWaterChanged = true;
	onDeepWater = deepWater;
}

float RigidBodyUnit::checkDeepWater(float soilZ, float waterZ)
{
	float minRelativeZ(waterZ - position().z);
	float maxRelativeZ(waterZ - soilZ);
	if(!flyingMode()) {
		if(prm().flying_down_without_way_points) {
			setDeepWater(maxRelativeZ > 0.0f);
			isUnderWaterSilouette = min(minRelativeZ, maxRelativeZ) > 2.0f * extent().z;
		} else {
			if(onDeepWater || onIcePrev)
				setDeepWater(maxRelativeZ > extent().z);
			else
				setDeepWater(min(minRelativeZ, maxRelativeZ) > 1.8f * extent().z);
			if(waterAnalysis())
				isUnderWaterSilouette = min(minRelativeZ, maxRelativeZ) > 2.0f * extent().z;
			else
				isUnderWaterSilouette = onDeepWater;				
		}
	} else {
		setDeepWater(maxRelativeZ > 0.0f);
		isUnderWaterSilouette = false;
	}
	if(onDeepWater && waterAnalysis())
		return max(soilZ, waterZ - waterLevel());
	return soilZ;
}

QuatF RigidBodyUnit::placeToGround(float& groundZNew)
{
	int smx = universe()->secondMap->w2mFloor(position().xi());
	int smy = universe()->secondMap->w2mFloor(position().yi());

	if(universe()->secondMap->inside(Vect2i(smx, smy)) && (*universe()->secondMap)(smx, smy).getElevator())
		onSecondMap = secondMapCheck(position().xi(), position().yi(), round(radius()));;

	isUnderWaterSilouette = false;

	if(onSecondMap){
		groundZNew = (*universe()->secondMap)(smx, smy); //sm;
		return QuatF::ID;
	}

	Vect2i center(position().xi(), position().yi());
	
	waterLevelPrev = environment->water()->GetZFast(center.x, center.y);
	Vect3f avrNormal(Vect3f::K);

	bool pointAreaAnalize(pointAreaAnalize_ || (prm().canEnablePointAnalyze && radius() < GlobalAttributes::instance().analyzeAreaRadius));
	if(whellController) {
		Se3f poseNew;
		poseNew.trans() = position();
		poseNew.rot().set(ptPoseAngle, Vect3f::K, false);
		avrNormal = whellController->placeToGround(poseNew, groundZNew, onIce);
		if(!onIce && (onLowWater || onWater)){
			if(pointAreaAnalize)
				groundZNew = checkDeepWater(groundZNew, waterLevelPrev);
			else{
				Vect3f waterNormal(Vect3f::K);
				groundZNew = checkDeepWater(groundZNew, analyzeWaterFast(center, round(radius()), waterNormal));
				if(onDeepWater && waterAnalysis())
					avrNormal = waterNormal;
			}
		}else
			setDeepWater(false);
	}
	else if(pointAreaAnalize){
		if(onIce){
			setDeepWater(false);
			groundZNew = max(groundZNew, environment->water()->GetZ(center.x, center.y));
		}else{
			groundZNew = vMap.GetApproxAlt(center.x, center.y);
			if(onLowWater || onWater || prm().flyingMode && checkLowWater(position(), radius()))
				groundZNew = checkDeepWater(groundZNew, waterLevelPrev);
			else
				setDeepWater(false);
		}
	}
	else
		groundZNew = analyzeAreaFast(center, round(radius()), avrNormal);

	if(!prm().moveVertical && !flyingMode() && !isFlyDownMode()){

		if(pointAreaAnalize && (!whellController || (onDeepWater && waterAnalysis())))
			avrNormal = normalMap->normalLinear(position().x, position().y);
		
		Vect3f cross = Vect3f::K % avrNormal;
		float len = cross.norm();

		QuatF poseRot(QuatF::ID);
		
		float avrNormalNorm(avrNormal.norm() + 1e-5f);

		if(!onImpassabilityFull && !ownerUnit().attr().isBuilding() && (avrNormal.z / avrNormalNorm < 0.5f))
			enableBoxMode();

		if(len > 0.001f)
			poseRot.set(acosf(dot(Vect3f::K, avrNormal) / avrNormalNorm), cross);

		return poseRot;
	}

	return QuatF::ID;
}

void RigidBodyUnit::ptUnitUpdateZPose(QuatF& poseQuat)
{
	colliding_ = 0;

	if(flyingMode() && !prm().flying_height_relative && !isFlyDownMode()) {
		groundZ = 0;
		pose_.trans().z = flyingHeight();
	}
	else {
		float groundZNew = 0;
		QuatF groundQuat = placeToGround(groundZNew);

		if(flyDownMode){
			if(F2DW(flyDownSpeed_) == 0){
				flyDownMode = false;
			}
			else{
				flyingHeightCurrent_ -= flyDownSpeed_;
				float flyingHeightMax(flyingMode() ? (prm().flying_height_relative ? flyingHeight() : flyingHeight() - groundZ) : 0.0f);
					
				if((flyingHeightCurrent_ > flyingHeightMax) == (flyDownSpeed_ < 0.0f)){
					flyingHeightCurrent_ = flyingHeightMax;
					flyDownSpeed_ = 0.0f;
				}
			}
		}

		if(!flyDownMode && waterAnalysis() && deepWaterChanged){
			groundZ = groundZNew;
			flyingHeightCurrent_ = position().z - groundZ;
		}else{
			if(whellController)
				groundZ = groundZNew;
			else
				groundZ = groundZ * (1 - prm().position_tau) + groundZNew * prm().position_tau;
			pose_.trans().z = groundZ + flyingHeightCurrent_;
		}
		
		if(flyingHeightCurrent_ > FLT_EPS)
			colliding_ = 0;
		else
			colliding_ = onWater ? WATER_COLLIDING : GROUND_COLLIDING;

		poseQuat.postmult(groundQuat);
	}
}

void RigidBodyUnit::ptUnitEvolve()
{
	start_timer_auto();

	setColor(BLUE);

	onImpassabilityFull = vMap.isImpassability(Vect2i(position2D().xi(), position2D().yi()), round(radius()));

	callcVerticalFactor();

	// Обработка локальной позиции и вэйпойнтов.
	if(unitOnTrail_){
		Vect3f positionNew = localPose;
		trail_->pose().xformPoint(positionNew);
		pose_.trans() = positionNew;
		ptPoseAngle += trail_->ptPoseAngle;

		if(!way_points.empty() && pointInTrail(Vect2f(way_points[0]), trail_->posePrev())){
			Vect3f pointNew = way_points[0];
			trail_->posePrev().invXformPoint(pointNew);
			trail_->pose().xformPoint(pointNew);
			way_points[0] = pointNew;
			safe_cast<UnitReal*>(ownerUnit_)->wayPoints()[0] = pointNew;
		}
		else if(!trail_->isDocked()){
			safe_cast<UnitReal*>(ownerUnit_)->stop();
			way_points.clear();
		}
		else if(!way_points.empty() && !checkTrailWayPoint(Vect2f(way_points[0]))){
			setAutoPTPoint(trail_->getEnterPoint());
		}
	}

	QuatF poseQuat(QuatF::ID);
	QuatF ptAdditionalRotPrev = ptAdditionalRot;
	ptAdditionalRot = QuatF::ID;

	// Направляем к точке входа.
	if(trail_ && trail_->isDocked() && !unitOnTrail_ && pointInTrail(Vect2f(way_points[0]), trail_->pose()))
		if(!checkTrailWayPoint(Vect2f(way_points[0])) && trail_->position2D().distance2(position2D()) > sqr(trail_->radius()))
			setAutoPTPoint(trail_->getEnterPoint());
		else {
			clearAutoPT();
			setIgnoreUnit(&trail_->ownerUnit());
		}

	if(flying_mode && prm().flying_down_without_way_points && !controlled())
		obstaclesFound_ = checkObstacles();
	else
		obstaclesFound_ = false;

	if(flyingModePrev != flyingMode()){
		flyingModePrev = flyingMode();
		enableFlyDownModeByTime(flyDownTime);
	}

	if(prm().controled_by_points && !unmovableXY_ && enablePathTracking_)
		tracking_analysis();

	Vect3f poseNew = position();
	float r(maxRadius() + 2.0f);
	poseNew.x = clamp(position().x, r, vMap.H_SIZE - 1 - r);
	poseNew.y = clamp(position().y, r, vMap.V_SIZE - 1 - r);
	pose_.trans() = poseNew;

	// Проверка на сход с trail
	if(unitOnTrail_ &&
	 !way_points.empty() &&
	 !pointInTrail(Vect2f(way_points[0]), trail_->pose()) &&
	 trail_->isDocked() &&
	 !pointInTrail(position2D(), trail_->pose())) {
		unitOnTrail_ = false;
		trail_ = 0;
	 }
	
	// Заход на trail.
	if(trail_ && trail_->isDocked() && !unitOnTrail_)
		if(pointInTrail(position2D(), trail_->pose())){
			unitOnTrail_ = true;
			setIgnoreUnit(0);
			clearAutoPT();
		}

	// Выставляем Z и запоминаем локальную позицию.
	if(unitOnTrail_){
		clampUnitOnTrailPosition();
		if(getTrailPoint(position2D(), pose_.trans().z)){
			localPose = position();
			trail_->pose().invXformPoint(localPose);
		}
		else
			ptUnitUpdateZPose(poseQuat);
	}
	else
		ptUnitUpdateZPose(poseQuat);

	if(unitOnTrail_)
		ptPoseAngle -= trail_->ptPoseAngle;

	QuatF ptQuat(ptPoseAngle, Vect3f::K, false);
	poseQuat.postmult(ptQuat);

	QuatF ptAdditionalRotNew;
	ptAdditionalRotNew.slerp(ptAdditionalRotPrev, ptAdditionalRot, prm().orientation_tau);
	poseQuat.postmult(ptAdditionalRotNew);
	ptAdditionalRot = ptAdditionalRotNew;

	if(unitOnTrail_)
		poseQuat.premult(trail_->orientation());

	poseQuat.normalize();
	pose_.rot() = poseQuat;
}

void RigidBodyUnit::ptUnitUpdatePose()
{
	QuatF ptQuat(ptPoseAngle, Vect3f::K, false);
	QuatF poseQuat(QuatF::ID);

	ptUnitUpdateZPose(poseQuat);

	poseQuat.postmult(ptQuat);

	QuatF poseQuatNew = poseQuat;
	poseQuatNew.slerp(pose_.rot(), poseQuat, prm().orientation_tau);

	pose_.rot() = poseQuatNew;
}

//=============================================================

class WaterScan {
	
	int tile00;
	int tile01;
	int tile11;
	int tile10;

	int tileDeltaX;
	int tileDeltaY;

	int startX;
	int tileDeltaXStart;

	int x;
	int y;

	int tileHLeft;
	int tileHRight;

	int tileScale;
	int tileShift;

	int tileDeltaHX;

public:

	WaterScan(int x_, int y_, int tileShift_){
        tileShift = tileShift_;
		tileScale = 1 << tileShift_;
		startX = x = x_ >> tileShift_;
		y = y_ >> tileShift_;
		tileDeltaX = tileDeltaXStart = x_ - (x << tileShift_);
		tileDeltaY = y_ - (y << tileShift_);

		tile00 = environment->water()->Get(x,y).z;
		tile10 = environment->water()->Get(x+1,y).z;
		tile01 = environment->water()->Get(x,y+1).z;
		tile11 = environment->water()->Get(x+1,y+1).z;

		tileHLeft = tile00 + (tile10-tile00)/tileScale*tileDeltaY;
		tileHRight = tile10 + (tile11-tile10)/tileScale*tileDeltaY;

		tileDeltaHX = (tileHRight - tileHLeft)/tileScale;

	}

	int goNextX(){
		int valueH = tileHLeft + tileDeltaHX * tileDeltaX;
		tileDeltaX++;
		if(tileDeltaX == tileScale){
			tile00 = tile10;
			tile01 = tile11;
			x++;
			tile10 = environment->water()->Get(x+1,y).z;
			tile11 = environment->water()->Get(x+1,y+1).z;
			tileHLeft = tile00 + ((tile10-tile00)>>tileShift)*tileDeltaY;
			tileHRight = tile10 + ((tile11-tile10)>>tileShift)*tileDeltaY;
			tileDeltaHX = (tileHRight - tileHLeft)>>tileShift;
			tileDeltaX = 0;
		}
		return valueH;
	}

	void goNextY(){
		tileDeltaY++;
		x = startX;
		tileDeltaX = tileDeltaXStart;
		
		if(tileDeltaY == tileScale){
			y++;
			tileDeltaY = 0;
		}

		tile00 = environment->water()->Get(x,y).z;
		tile10 = environment->water()->Get(x+1,y).z;
		tile01 = environment->water()->Get(x,y+1).z;
		tile11 = environment->water()->Get(x+1,y+1).z;

		tileHLeft = tile00 + (tile10-tile00)/tileScale*tileDeltaY;
		tileHRight = tile10 + (tile11-tile10)/tileScale*tileDeltaY;

		tileDeltaHX = (tileHRight - tileHLeft)/tileScale;

	}
};

//=============================================================

float RigidBodyUnit::analyzeAreaFast(const Vect2i& center, int r, Vect3f& normalNonNormalized)
{
	int Sz = 0;
	int Sxz = 0;
	int Syz = 0;
	int SWaterz = 0;
	int SWaterxz = 0;
	int SWateryz = 0;
	int delta = max(vMap.w2m(r), 1);
	int xc = clamp(vMap.w2m(center.x), delta, vMap.GH_SIZE - delta - 4);
	int yc = clamp(vMap.w2m(center.y), delta, vMap.GV_SIZE - delta - 4);

	float t = delta * (delta + 1.0f);
	float N = 4.0f * t + 1.0f;
	t = 3.0f / sqr(2.0f * delta + 1.0f) / t;
    float groundZNew(0);

	if(onIce){
		setDeepWater(false);
		WaterScan waterScan(xc - delta, yc - delta, 2);
		for(int y = -delta;y <= delta;y++){
			for(int x = -delta;x <= delta;x++){
				int z = vMap.GVBuf[vMap.offsetGBuf(x + xc, y + yc)];
				z += max(0, (waterScan.goNextX() >> 16));
				Sz += z;
				Sxz += x*z;
				Syz += y*z;
			}
			waterScan.goNextY();
		}
	}else if(onLowWater || onWater || prm().flyingMode && checkLowWater(position(), radius())){
		WaterScan waterScan(xc - delta, yc - delta, 2);
		for(int y = -delta;y <= delta;y++){
			for(int x = -delta;x <= delta;x++){
				int z = vMap.GVBuf[vMap.offsetGBuf(x + xc, y + yc)];
				Sz += z;
				Sxz += x*z;
				Syz += y*z;
				z = max(0, (waterScan.goNextX() >> 16));
				SWaterz += z;
				SWaterxz += x*z;
				SWateryz += y*z;
			}
			waterScan.goNextY();
		}
		groundZNew = checkDeepWater((float)Sz / N, (float)(Sz + SWaterz) / N);
	}else{
		setDeepWater(false);
		for(int y = -delta;y <= delta;y++){
			for(int x = -delta;x <= delta;x++){
				int z = vMap.GVBuf[vMap.offsetGBuf(x + xc, y + yc)];
				Sz += z;
				Sxz += x*z;
				Syz += y*z;
			}
		}
	}
	
	if(onDeepWater && waterAnalysis()){
		Sxz += SWaterxz;
		Syz += SWateryz;
	}else
		groundZNew = (float)Sz / N;
	
	float A(Sxz * t);
	float B(Syz * t);
	normalNonNormalized.set(-A, -B, 4);
	return groundZNew;
}

float RigidBodyUnit::analyzeWaterFast(const Vect2i& center, int r, Vect3f& normalNonNormalized)
{
	int Sz = 0;
	int Sxz = 0;
	int Syz = 0;
	int delta = max(vMap.w2m(r), 1);
	int xc = clamp(vMap.w2m(center.x), delta, vMap.GH_SIZE - delta - 4);
	int yc = clamp(vMap.w2m(center.y), delta, vMap.GV_SIZE - delta - 4);

	float t = delta * (delta + 1.0f);
	float N = 4.0f * t + 1.0f;
	t = 3.0f / sqr(2.0f * delta + 1.0f) / t;
		
	WaterScan waterScan(xc - delta, yc - delta, 2);
	for(int y = -delta;y <= delta;y++){
		for(int x = -delta;x <= delta;x++){
			int z = vMap.GVBuf[vMap.offsetGBuf(x + xc, y + yc)];
			z += max(0, (waterScan.goNextX() >> 16));
			Sz += z;
			Sxz += x*z;
			Syz += y*z;
		}
		waterScan.goNextY();
	}
	float A(Sxz * t);
	float B(Syz * t);
	normalNonNormalized.set(-A, -B, 4);
	return (float)Sz / N;
}

class ObstaclesFinder {
public:
	bool found;
	const UnitBase* owner_;
	ObstaclesFinder(const UnitBase* owner):found(false), owner_(owner) {}
	void operator () (UnitBase* p) {
		if(owner_ != p && (p->attr().isLegionary() || p->attr().isBuilding()))
			if(owner_->position2D().distance2(p->position2D()) < sqr(owner_->radius() + p->radius()))
				if(!p->rigidBody()->isUnit() || !safe_cast<RigidBodyUnit*>(p->rigidBody())->flyingMode())
					found = true;
	}
};

bool RigidBodyUnit::checkObstacles()
{
	ObstaclesFinder of(&ownerUnit());
	universe()->unitGrid.Scan(Vect2i(position().x, position().y), radius(), of);
	return of.found;
}

//=======================================================
//	PathTracking Analysis.
//=======================================================
void RigidBodyUnit::tracking_analysis()
{
	onImpassability = pathFinder->impassabilityCheck(position().xi(), position().yi(), 3.0f);
//	onImpassability = pathFinder->impassabilityCheck(position().xi(), position().yi(), round(radius()));

	if (!way_points.empty() || ptAction_.active() || manualMoving) {
		Vect2f curVelocityDir_(Vect2f::ZERO);
		if((rotationMode || isAutoPTRotation || ptAction_.isRotation()) && !manualMoving)
			ptRotationMode();
		else {
			Vect3f prevVelTemp = Vect3f(0,moveback?-ptVelocity:ptVelocity,0);
			pose().xformVect(prevVelTemp);
			if(manualMoving && ownerUnit().attr().rotToTarget)
				ptRotationMode();
			if(ptGetVelocityDir()){
				curVelocityDir_ = Vect2f(moveback ? -1.0f : 1.0f, 0.0f);
				curVelocityDir_ *= ptPose_.rot;
			}
				
			if(onIce) {
				pose().invXformVect(prevVelocity_);
				prevVelocity_.y = prevVelocity_.z = 0;
				pose().xformVect(prevVelocity_);
				pose_.trans().add(prevVelocity_ * 5 * DT);
			}
			prevVelocity_ = prevVelTemp;
		}
/*
		if((!ptImpassabilityCheck_ || !ptObstaclesCheck_) && sumAvrVelocityCount_ > BACK * 2){
			debugMessage("PT: Не игнорирую зоны и юнитов");
//			ptImpassabilityCheck_ = prm().ptImpassabilityCheck;
			ptObstaclesCheck_ = prm().ptObstaclesCheck;
			sumAvrVelocity_ = Vect2f::ID;
			sumAvrVelocityCount_ = 0;
		}
*/			
		if(sumAvrVelocityCount_ > BACK * 2 && !manualMoving){
			if(sumAvrVelocity_.norm2() < sqr(0.2f * sumAvrVelocityCount_)){
				debugMessage("PT: Застрял - кручусь");
				enableAutoPTRotation();
//				turnRotationAction(ptAction_.isMove() ? ptAction_.point() : way_points.front(), RigidBodyUnit::ACTION_PRIORITY_PATH_TRACKING);
			}
			sumAvrVelocity_ = Vect2f::ID;
			sumAvrVelocityCount_ = 0;
		}

		sumAvrVelocity_ += curVelocityDir_;
		sumAvrVelocityCount_++;
	}
	else {
		stop_quants = 0;
		prevVelocity_ = Vect3f::ZERO;
		ptVelocity = 0;
	}

}

//=======================================================
//  Разварот а месте...
//=======================================================
void RigidBodyUnit::ptRotationMode()
{
	debugMessage("PT:RotationMode");

	float rotSpeed = onWater?ownerUnit().attr().angleSwimRotationMode/(1800.0/M_PI):ownerUnit().attr().angleRotationMode/(1800.0/M_PI);
	rotationSide = ROT_RIGHT;

	if(!isAutoPTRotation) {
		if(isManualMode())
			wpoint = manualViewPoint;
		else if(ptAction_.active()){
			wpoint = ptAction_.point();
			debugMessage("PT: ptAction.Rotation");
		}
		else if(!way_points.empty())
			wpoint = way_points.front();

		ptPose_.trans = pose_.trans();
		ptPose_.rot.set(ptPoseAngle + M_PI/2.0);
		temp_localwpoint = ptPose_.invXform(wpoint);
		rotationMode = true;

		float angle = atan2(temp_localwpoint.y, temp_localwpoint.x);
		if (fabs(angle) < rotSpeed + FLT_EPS) {
			rotationMode = false;
			ptPoseAngle += angle;
			return;
		} else if (angle < 0) {
			rotSpeed = -rotSpeed;
			rotationSide = ROT_LEFT;
		}
	} else
		isAutoPTRotation = false;

	ptPoseAngle += rotSpeed;
	return;
}

//=======================================================
// Пересечение точка-сфера. true - если не пересекает..
//=======================================================
__forceinline bool pointInCircle(const Vect2f& point, const Vect2f & circleCenter, float circleRadius = 1.0f)
{
	return circleCenter.distance2(point) <= circleRadius*circleRadius;
}

//=======================================================
// Пересечение точка-box. false - если не пересекает..
//=======================================================
__forceinline bool pointInBox(const Vect2f& point, const Vect2f & box_min, const Vect2f & box_max)
{
	return point.x > box_min.x && point.x < box_max.x && point.y > box_min.y && point.y < box_max.y;
}

//=======================================================
// Пересечение линия-сфера. false - если не пересекает..
//=======================================================
inline bool CheckLine(const Vect3f & l1, const Vect3f & l2, const Vect3f & center, float radius = 1.0)
{
	
	Vect2f D = l2-l1;
	D.normalize(1.0);
	Vect2f delta = l1 - center;

	float dotD_Delta = D.dot(delta);
	
	float sigma = (dotD_Delta*dotD_Delta) - (delta.norm()*delta.norm()-radius*radius);
	
	if (sigma < 0) return true;
	if (sigma == 0) return true;

	float t1 = (-dotD_Delta-sqrt(sigma));

	if (t1<0) return true;

	D.normalize(t1);

	if (D.norm() > (l2-l1).norm()) return true;

	return false;
}

//=======================================================
// Расчет радиуса разворота.
//=======================================================
__forceinline float callcVehicleRadius(float angle, float lineLen)
{
	return lineLen/angle;
}

//=======================================================
// Обработка движения. Логика выбора линий движения.
//=======================================================
bool RigidBodyUnit::ptGetVelocityDir()
{

	start_timer_auto();
	
	// Заехал на горку - езжай назад.
	if(forwardVelocity() < 1.0f){
		if(prm().path_tracking_back){
			moveback = !moveback;
			ptVelocity = 0;
		} else {
			enableAutoPTRotation();
			ptRotationMode();
			Vect2f point(radius()*3.0f, 0);
			point *= ptPose();
			setAutoPTPoint(To3D(point));
		}
	}

	// Если остановился, стоит stop_quants.
	if(stop_quants && stop_quants < STOP) {
		stop_quants++;	
		return false;
	}
	else
		stop_quants = 0;

	wpoint = Vect2f::ZERO;
	if(ptAction_.isMove())
		wpoint = Vect2f(ptAction_.point());
	else if(!manualMoving)
		wpoint = Vect2f(way_points.front());


	float ptAngleDelta = M_PI/2.0;
	switch(ptDirection()){
	case PT_DIR_RIGHT:
		ptAngleDelta = M_PI;
		break;
	case PT_DIR_LEFT:
		ptAngleDelta = 0.0f;
		break;
	case PT_DIR_BACK:
		ptAngleDelta = -M_PI/2.0;
		break;
	}

	ptPose_.trans = (Vect2f(pose_.trans()));
	ptPose_.rot.set(ptPoseAngle + ptAngleDelta);
	temp_localwpoint = ptPose_.invXform(wpoint);

//	if (temp_localwpoint.x < 0) moveback = true;

	if(!manualMoving) {
		float wpointDist = ptPose_.trans.distance(wpoint);
//		if((wpointDist - prm().is_point_reached_radius_max) <= ptVelocity*ptVelocity/(2.0*prm().forward_acceleration) && way_points.size() == 1)
//			ptVelocity -= prm().forward_acceleration * DT;
//		else if(ptVelocity < forwardVelocity())
		if(ptVelocity < forwardVelocity())
			ptVelocity += prm().forward_acceleration * DT;
//			ptVelocity = forwardVelocity();

		if(ptVelocity > forwardVelocity())
			ptVelocity = forwardVelocity();

		if(ptVelocity < 0.0f)
			ptVelocity = 0.0f;

		if(ptVelocity * DT > wpointDist)
			ptVelocity = wpointDist / DT;
	}

	bool enableMakeWay = (!(ownerUnit().attr().isResourceItem() || ownerUnit().attr().isInventoryItem()) && GlobalAttributes::instance().enableMakeWay);
	UnitMovePlaner mp(ownerUnit(), ignoreUnit_, wpoint,
		GlobalAttributes::instance().enablePathTrackingRadiusCheck, enableMakeWay, GlobalAttributes::instance().enableEnemyMakeWay);

	mp.enemyIgnoreRadius = GlobalAttributes::instance().enemyRadiusCheck;
	mp.ignoreRadius = GlobalAttributes::instance().minRadiusCheck;
	float scanRadius = radius() + ptVelocity * DT * 15;
	if(position2D().distance2(wpoint) < sqr(ownerUnit().attr().getPTStopRadius()))
		scanRadius = max(position2D().distance(wpoint), scanRadius);
//	show_vector(position(), scanRadius, BLUE);
	universe()->unitGrid.Scan(ptPose_.trans.xi(),ptPose_.trans.yi(), round(scanRadius), mp);
	
	// Движущаяся помеха слева. Подождать.
	if(mp.waitLeftObstacle_){
		stop_quants++;
		ptVelocity = 0;
		debugMessage("PT: Помеха слева");
		return false;
	}

	if(mp.removeWPoint) {
		if(ptAction_.isMove())
			ptAction_.point() = mp.newWPoint;
		else if(way_points.size() == 1){
			UnitReal* trackerReal = const_cast<UnitReal*>(safe_cast<const UnitReal*>(&ownerUnit()));
			Vect2f manualPoint(trackerReal->getManualUnitPosition().x, trackerReal->getManualUnitPosition().y);
			Vect2f wayPoint(way_points.front().x, way_points.front().y);
			if(manualPoint.distance2(wayPoint) < sqr(1.5f*radius())){
				trackerReal->setWayPoint(mp.newWPoint);
				way_points.push_back(mp.newWPoint);
			}
			else{
				Vect3f p2 = clampWorldPosition(mp.newWPoint, radius());
				way_points.clear();
				way_points.push_back(p2);
				trackerReal->wayPoints().clear();
				trackerReal->wayPoints().push_back(p2);
			}
			debugMessage("PT: Точка не доступна");
//			return false;
		}
		stopFunctorFirstStep = 0;
	}

	// Если точка недалеко сзади - ехать назад.
	if(prm().path_tracking_back && !moveback && temp_localwpoint.x < 0 && temp_localwpoint.norm2() < sqr(2.0f * radius()) && !wpointInVehicleRadius()) {
		moveback = true;
		back_quants = BACK - 1;
		stop_quants = 0;
		ptVelocity = 0;
		return false;
	}

	// Логика выбора линии движения...
	if(!manualMoving)
		if (moveback)
			if(wpointInVehicleRadius())
				ptSortBackLines();
			else
				ptUnsortBackLines();
//		else if(wpointInVehicleRadius()) {
//			debugMessage("PT: Точка в радиусе разворота");
//			ptUnsortLines();
//		}
		else
			ptSortLines();
	else
		ptManualLines();

	if(ptImpassabilityCheck_ && (!manualMoving || GlobalAttributes::instance().checkImpassabilityInDC) && !unitOnTrail_)
		ImpassabilityCheck(this);

	const Vect2f * move_line = NULL;

	int i = 0;
	while(sortlines[i] == NULL && i<LINES_NUM*2) i++;
	move_line = sortlines[i];
	
	mp.callFreeLine();

	if(mp.freeLine() != NO_OBSTACLES)
		if(mp.freeLine() != NO_PATH)
			move_line = sortlines[mp.freeLine()];
		else
			move_line = NULL;

	// Обтекание препятствия
	if(manualMoving && move_line == NULL && !GlobalAttributes::instance().checkImpassabilityInDC){
		Vect2f flowVector = mp.callcFlowVector();
		show_vector(position(), To3D(flowVector * 10.0f), RED);
		MatX2f poseNew = ptPose_;
		poseNew.trans += flowVector;
		if(!collidingObstacleList(this, poseNew, mp.obstacleList, false)) {
			pose_.trans().x = poseNew.trans.x;
			pose_.trans().y = poseNew.trans.y;

			flowVector = ptPose_.rot.invXform(flowVector);
			float angle = atan2(flowVector.y, flowVector.x);
			if(fabs(angle) < path_tracking_angle)
				ptPoseAngle += angle;
			else if(angle < 0)
				ptPoseAngle -= path_tracking_angle;
			else
				ptPoseAngle += path_tracking_angle;

			debugMessage("PT: Обтекание препятствия");
			return true;
		}
	}

	// Если линия долго не свободна - пробует изменить направление (взад/вперед)
	if ((stop_quants>STOP)&&(move_line == NULL)){
		debugMessage("PT: Меняем направление");
		stop_quants = 0;
		ptVelocity = 0;
		if(prm().path_tracking_back)
			moveback = !moveback;
		else
			enableAutoPTRotation();
//		setVelocity(Vect3f::ZERO);
		return false;
	}

	// Предотвращает верчение вокруг (.)
//	if (!moveback && (back_quants < BACK)&&(wpoint.distance(ptPose_.trans) < 2.5f * callcVehicleRadius(path_tracking_angle, ptVelocity * DT))&&(wpoint.distance(ptPose_.trans) > callcVehicleRadius(path_tracking_angle, ptVelocity * DT))&&(temp_localwpoint.x<0)) {
	if((ptAction_.isMove() || way_points.size() == 1) && !moveback && temp_localwpoint.x < 0 && wpointInVehicleRadius()) {
		debugMessage("PT: Верчение вокруг точки");
		if(prm().path_tracking_back) {
			moveback = true;
			ptVelocity = 0;
			return false;
		} else if(ownerUnit().attr().enableRotationMode){
			enableRotationMode();
			return false;
		}
	}

	// Ждет - если нет свободных линий
	if (move_line == NULL){
		debugMessage("PT: Нет свободных линий");
		if(prm().path_tracking_back && !manualMoving) {
			stop_quants++;
			moveback = !moveback;
			ptVelocity = 0;
		} else if(ownerUnit().attr().enableRotationMode && !manualMoving)
			enableAutoPTRotation();
//		setVelocity(Vect3f::ZERO);
		return false;
	}


	// Если назад дальше нельзя - переключается на перед.
	if ((moveback)&&(move_line == NULL)&&(back_quants!=0)){
		debugMessage("PT: Назад нельзя - едит вперед");
		moveback = false;
		ptVelocity = 0;
//		setVelocity(Vect3f::ZERO);
		return false;
	}

	// Назад ненужно ехать всегда.. вперед лучше 
	if (moveback) back_quants++;
	if (!manualMoving && back_quants > BACK && temp_localwpoint.x > 0){
		debugMessage("PT: Едит вперед");
		moveback = false;
		ptVelocity = 0;
		back_quants = 0;
//		setVelocity(Vect3f::ZERO);
		return false;
	}

	stop_quants = 0;

	if(ptVelocity > FLT_EPS) {
		
		// Движение по найденной линии - простейшая модель
		float ang = atan2(move_line->y, move_line->x);

		if(moveback)
			if(ang > 0)
				ang -= M_PI;
			else
				ang += M_PI;

		ang *= ptVelocity / forwardVelocity();

		Vect2f temp_v(*move_line);
		
		temp_v *= ptPose_.rot;
		temp_v.Normalize(ptVelocity * DT);

		ptPoseAngle += ang;
		pose_.trans().x += temp_v.x;
		pose_.trans().y += temp_v.y;
		lastPTAngle = ang;
		
		if(flyingMode()){
			QuatF forvardRot(ptVelocity/forwardVelocity() * prm().additionalForvardRot, moveback ? Vect3f::I : Vect3f::I_);
			QuatF horizontRot(ang/ownerUnit().attr().pathTrackingAngle * prm().additionalHorizontalRot, Vect3f::J_);
			ptAdditionalRot.postmult(forvardRot);
			ptAdditionalRot.postmult(horizontRot);
		}

		setColor(CYAN);
		if(stopFunctorFirstStep != 0)
			stopFunctorFirstStep++;
		return true;
	}

	return false;
}

//=======================================================
bool NearestLine(const Vect2f * v1, const Vect2f * v2)
{
	if(temp_localwpoint.dot(*v1) > temp_localwpoint.dot(*v2))
		return true;
	else 
		return false;
}

//=======================================================
bool FarestLine(const Vect2f * v1, const Vect2f * v2)
{
	float dist1 = temp_localwpoint.distance2(*v1);
	float dist2 = temp_localwpoint.distance2(*v2);

	if (dist1 >= dist2) return true; else return false;
}

//=======================================================
void RigidBodyUnit::ptSortLines()
{
//	for(int i=0;i<(LINES_NUM*2+1);i++)
//		sortlines[i] = &lines[i];
//	ptPose_.invXformPoint(wpoint,temp_localwpoint);
//	sort(&sortlines[0],&sortlines[LINES_NUM*2],NearestLine);

	float maxDot = -1e10;
	int maxI = 0;

	temp_localwpoint = ptPose_.invXform(wpoint);
	Vect2f tempDir = temp_localwpoint;
	tempDir.Normalize(1.0f);
	tempDir += Vect2f(1,0);

	for(int i=0;i<(LINES_NUM*2+1);i++){
		float curDot = tempDir.dot(lines[i]);
		if(curDot > maxDot){
			maxI = i;
			maxDot = curDot;
		}
	}
	
	int k = maxI;
	int m = maxI;
	sortlines[0] = &lines[maxI];
	int it = 1;
	while(it < (LINES_NUM*2+1)){
		if(k > 0){
			k--;
			sortlines[it] = &lines[k];
			it++;
		}
		if(m < LINES_NUM * 2){
			m++;
			sortlines[it] = &lines[m];
			it++;
		}
	}
}

//=======================================================
void RigidBodyUnit::ptSortBackLines()
{
	for(int i=0;i<(LINES_NUM*2+1);i++)
		sortlines[i] = &backlines[i];

	temp_localwpoint = ptPose_.invXform(wpoint);
	temp_localwpoint.Normalize(1.0f);
	sort(&sortlines[0],&sortlines[LINES_NUM*2],FarestLine);
}

//=======================================================
// Бедет длого кружить, не сможет подьехать к близкой точке.
//=======================================================
void RigidBodyUnit::ptUnsortBackLines()
{
/*	for(int i=0;i<(LINES_NUM);i++)
	{
		sortlines[i*2+0] = &backlines[i];
		sortlines[i*2+1] = &backlines[LINES_NUM*2-i];
	}
	sortlines[LINES_NUM*2] = &backlines[LINES_NUM];
*/
	for(int i=0;i<(LINES_NUM*2+1);i++)
		sortlines[i] = &backlines[i];
}

//=======================================================
void RigidBodyUnit::ptUnsortLines()
{
	float minDot = 1e10;
	int minI = 0;

	temp_localwpoint = ptPose_.invXform(wpoint);
	Vect2f tempDir = temp_localwpoint;
	tempDir.Normalize(1.0f);
	tempDir += Vect2f(1,0);

	for(int i=0;i<(LINES_NUM*2+1);i++){
		float curDot = tempDir.dot(lines[i]);
		if(curDot < minDot){
			minI = i;
			minDot = curDot;
		}
	}
	
	int k = minI;
	int m = minI;
	sortlines[0] = &lines[minI];
	int it = 1;
	while(it < (LINES_NUM*2+1)){
		if(k > 0){
			k--;
			sortlines[it] = &lines[k];
			it++;
		}
		if(m < LINES_NUM * 2){
			m++;
			sortlines[it] = &lines[m];
			it++;
		}
	}
}

//=======================================================
void RigidBodyUnit::ptFrontMoveLines()
{
	sortlines[0] = &lines[LINES_NUM];

	for(int i=0; i<(LINES_NUM); i++){
		sortlines[i*2+1] = &lines[LINES_NUM - (i + 1)];
		sortlines[i*2+2] = &lines[LINES_NUM + (i + 1)];
	}
}

//=======================================================
//	Проверяет пересечение юнита с несколькими препятствиями.
//=======================================================
__forceinline bool collidingObstacleList(const RigidBodyBase* vehicle, const MatX2f& vehiclePose, vector<RigidBodyBase*>& obstacleList, bool firstStep)
{
	vector<RigidBodyBase*>::iterator it;
	FOR_EACH(obstacleList, it){
		bool unitMove = (*it)->isUnit() ? safe_cast<RigidBodyUnit*>(*it)->unitMove() : false;
		if((!unitMove || firstStep) && (*it)->bodyIntersect(vehicle, vehiclePose))
/*		
		Vect2f boxMin(Vect2f((*it)->boxMin()) - Vect2f(vehicleRadius, vehicleRadius));
		Vect2f boxMax(Vect2f((*it)->boxMax()) + Vect2f(vehicleRadius, vehicleRadius));

		float angle = -atan2f((*it)->rotation()[0][1], (*it)->rotation()[1][1]);
		MatX2f obstaclePose(Mat2f(angle), (*it)->position());
		Vect2f point = vehiclePose;
		point = obstaclePose.invXform(point);
		
		if((!unitMove || firstStep) && pointInBox(point, boxMin, boxMax))*/
			return true;
	}

    return false;
}

//=======================================================
//	проверка с другими юнитами.. true - если линия свободна.
//=======================================================
inline bool isFreeLine(RigidBodyUnit * vehicle, vector<RigidBodyBase*>& obstacleList,  int index)
{
	if(!vehicle->sortlines[index]) return false;

	float vel = vehicle->forwardVelocity() * DT;
//	float vel = vehicle->ptVelocity * DT; // Если сделать так, то юнит подьезжая к дрогому - дергается.
	
	Vect2f temp = *vehicle->sortlines[index];
	temp.Normalize(vel);

	float ang = atan2(vehicle->sortlines[index]->y,vehicle->sortlines[index]->x);
	if(vehicle->moveback)ang += M_PI;
	MatX2f ppose = vehicle->ptPose();
	Vect2f temp_v;

	for(int i=0; i<12; i++) {

		temp_v = temp;
		temp_v *= ppose;

		if(i && temp_v.distance2(vehicle->position()) > temp_localwpoint.norm2())
			return true;

		if(showDebugRigidBody.showPathTracking_ObstaclesLines && (i%2))
			show_line(Vect3f(ppose.trans, vehicle->position().z), Vect3f(temp_v, vehicle->position().z), GREEN);

		ppose.rot *= Mat2f(ang);
		ppose.trans = temp_v;

		if(collidingObstacleList(vehicle, ppose, obstacleList, i < 5)){
			if(i > maxLineI) {
				maxLineI = i;
				maxLineNum = index;
			}
			return false;
		}
	}

	return true;
}

//=======================================================
// Проверка на пересечение с зонами непроходимости.
//=======================================================
inline bool ImpassabilityLine(RigidBodyUnit * vehicle, int index)
{
	float vel = vehicle->forwardVelocity() * DT;
//	float vel = vehicle->prm().forward_acceleration * DT;
//	float vel = vehicle->ptVelocity * DT;
	
	Vect2f temp = *vehicle->sortlines[index];
	temp.Normalize(vel);

	float ang = atan2(vehicle->sortlines[index]->y,vehicle->sortlines[index]->x);
	if(vehicle->moveback)ang += M_PI;
	MatX2f ppose = vehicle->ptPose();
	Vect2f temp_v;
	bool checkNext = false;
	float waterFactor = 0;

	for(int i=0; i<10; i++) {

		temp_v = temp;
		temp_v *= ppose;

		if(i && temp_v.distance2(vehicle->position()) > temp_localwpoint.norm2() && (vehicle->way_points.size() == 1.0 || vehicle->ptAction_.isMove()))
			return true;

//		if(!(i%2)){

			if(showDebugRigidBody.showPathTracking_ImpassabilityLines && (i%2))
				show_line(Vect3f(ppose.trans, vehicle->position().z), Vect3f(temp_v, vehicle->position().z), GREEN);

/*			if(!vehicle->checkImpassability(Vect3f(temp_v, 0), vehicle->radius()*(1.0+i*0.01f))) {
				float newFactor = pathFinder->checkWaterFactor(temp_v.x, temp_v.y, vehicle->radius());
				if(checkNext && (newFactor > waterFactor || newFactor == 1 || newFactor == 0)){
					float val = ((RigidBodyUnit::LINES_NUM * 2 + 1) - index) * 0.3f + i;
					if(val > maxLineI && i > 1) {
						maxLineI = val;
						maxLinePointer = vehicle->sortlines[index];
					}
					return false;
				}
				checkNext = true;
				waterFactor = newFactor;
			} else
				checkNext = false;
//		}
*/
		if(!vehicle->checkImpassability(Vect3f(temp_v, 0), vehicle->radius()*(1.0+i*0.01f))) {
			float val = ((RigidBodyUnit::LINES_NUM * 2 + 1) - index) * 0.3f + i;
			if(val > maxLineI && i > 1) {
				maxLineI = val;
				maxLinePointer = vehicle->sortlines[index];
			}
			return false;
		}

		ppose.rot *= Mat2f(ang);
		ppose.trans = temp_v;
	}

	return true;
}

//=======================================================
void ImpassabilityCheck(RigidBodyUnit * vehicle)
{
	if(	vehicle->prm().groundPass == RigidBodyPrm::PASSABILITY &&
		vehicle->prm().waterPass == RigidBodyPrm::PASSABILITY &&
		vehicle->prm().impassabilityPass == RigidBodyPrm::PASSABILITY)
		return;

	maxLineI = 0;
	maxLinePointer = 0;
	int nullLineCount = 0;

	for(int i=0;i<(RigidBodyUnit::LINES_NUM*2+1);i++)
		if(vehicle->sortlines[i] && !ImpassabilityLine(vehicle, i)){
			vehicle->sortlines[i] = NULL;
			nullLineCount++;
		}

	if(nullLineCount == (RigidBodyUnit::LINES_NUM * 2 + 1) && maxLineI > 0)
		vehicle->sortlines[0] = maxLinePointer;
}

//=======================================================
// Прямое управление.
//=======================================================
void RigidBodyUnit::ptManualLines()
{
	if(manualPTLine < 0)
		manualPTLine += 0.4f;
	else if(manualPTLine > 0)
		manualPTLine -= 0.4f;

	if(ptVelocity > 0)
		ptVelocity -= 3.0f*prm().forward_acceleration * DT / 4.0f;
	
	if(ptVelocity > forwardVelocity())
		ptVelocity = forwardVelocity();

	if(ptVelocity < 0)
		ptVelocity = 0;

//	if(ptVelocity == 0)
//		manualPTLine = 0;

	for(int i=0; i<(LINES_NUM*2+1); i++)
		sortlines[i] = NULL;

	manualPTLine = clamp(manualPTLine, (float)-(LINES_NUM-2), (float)LINES_NUM-2);
	int clampPTLine = round(manualPTLine);

	if(moveback) {
		sortlines[0] = &backlines[LINES_NUM + clampPTLine];
		sortlines[1] = &backlines[LINES_NUM + clampPTLine-1];
		sortlines[2] = &backlines[LINES_NUM + clampPTLine+1];
		sortlines[3] = &backlines[LINES_NUM + clampPTLine-2];
		sortlines[4] = &backlines[LINES_NUM + clampPTLine+2];
	} else {
		sortlines[0] = &lines[LINES_NUM + clampPTLine];
		sortlines[1] = &lines[LINES_NUM + clampPTLine-1];
		sortlines[2] = &lines[LINES_NUM + clampPTLine+1];
		sortlines[3] = &lines[LINES_NUM + clampPTLine-2];
		sortlines[4] = &lines[LINES_NUM + clampPTLine+2];
	}
}

void RigidBodyUnit::manualTurnLeft(float factor)
{
	manualPTLine -= 0.5f * factor;
}

void RigidBodyUnit::manualTurnRight(float factor)
{
	manualPTLine += 0.5f * factor;
}

void RigidBodyUnit::manualMoveForward(float factor)
{
	if(isBoxMode() || unmovable() || unmovableXY())
		return;

	if(!moveback)
		ptVelocity += prm().forward_acceleration * factor * DT;
	else {
		ptVelocity -= prm().forward_acceleration * factor * DT;
		if(ptVelocity < 0) {
			moveback = false;
			ptVelocity = -ptVelocity;
		}
	}
}

void RigidBodyUnit::manualMoveBackward(float factor)
{
	if(isBoxMode() || unmovable() || unmovableXY())
		return;

	if(!moveback) {
		ptVelocity -= prm().forward_acceleration * factor * DT;
		if(ptVelocity < 0) {
			moveback = true;
			ptVelocity = -ptVelocity;
		}
	} else
		ptVelocity += prm().forward_acceleration * factor * DT;

}

bool RigidBodyUnit::setAutoPTPoint( const Vect3f& point )
{
	if(checkImpassabilityStatic(point))
		return ptAction_.setMovePoint(point);

	return false;
}

bool RigidBodyUnit::wpointInVehicleRadius()
{
	float rad = callcVehicleRadius(path_tracking_angle, ptVelocity * DT) * 1.1f;

	Vect2f n1(0,-rad);
	n1 *= ptPose_;
	if(showDebugRigidBody.showPathTracking_VehicleRadius)
		show_vector(To3D(n1), rad, RED);
	if(n1.distance2(wpoint) < sqr(rad * 1.1f))
		return true;

	Vect2f n2(0, rad);
	n2 *= ptPose_;
	if(showDebugRigidBody.showPathTracking_VehicleRadius)
		show_vector(To3D(n2), rad, RED);
	if(n2.distance2(wpoint) < sqr(rad * 1.1f))
		return true;

	return false;
}

void RigidBodyUnit::clampUnitOnTrailPosition()
{
	Vect3f A(position().x, position().y, 0);
	Vect3f B(Vect3f::K);

	trail_->pose().invXformPoint(A);
	trail_->pose().invXformVect(B);

	float t = (trail_->boxMax().z - A.z)/B.z;
	Vect3f pointL = A + B * t;

	pointL.x = clamp(pointL.x, trail_->boxMin().x + radius(), trail_->boxMax().x - radius());
	pointL.y = clamp(pointL.y, trail_->boxMin().y + radius(), trail_->boxMax().y - radius());

	trail_->pose().xformPoint(pointL);

	pose_.trans().x = pointL.x;
	pose_.trans().y = pointL.y;
}

bool RigidBodyUnit::checkTrailWayPoint( Vect2f& point )
{
	Vect2f tP1;
	Vect2f tP2;

	if(!trail_->getEnterLine(tP1, tP2))
		return false;

	return linesIntersect(tP1, tP2, position2D(), point);
}

bool RigidBodyUnit::rot2point( const Vect3f& point )
{
	Vect3f temp_localwpoint;
	Se3f tmp_pose = pose();
	QuatF r2(M_PI/2.0, Vect3f(0,0,1));
	tmp_pose.rot().postmult(r2);
	tmp_pose.invXformPoint(point,temp_localwpoint);
	float angle = atan2(temp_localwpoint.y, temp_localwpoint.x);

	if(fabs(angle) > 0.1) {
		if(ptAction_.setRotationPoint(point))
			enableRotationMode();
		return false;
	}

	return true;
}

void RigidBodyUnit::PTAction::checkFinish(RigidBodyUnit* unit)
{
	if(!timer_)
		mode_ = RigidBodyUnit::ACTION_MODE_NONE;

	switch(mode_) {
	case RigidBodyUnit::ACTION_MODE_ROTATION: {
		Vect3f temp_localwpoint;
		Se3f tmp_pose = unit->pose();
		QuatF r2(M_PI/2.0, Vect3f(0,0,1));
		tmp_pose.rot().postmult(r2);
		tmp_pose.invXformPoint(point_,temp_localwpoint);
		float angle = atan2(temp_localwpoint.y, temp_localwpoint.x);
		if(fabs(angle) < 0.1)
			mode_ = RigidBodyUnit::ACTION_MODE_NONE;
		}
		break;
	case RigidBodyUnit::ACTION_MODE_MOVE:
		if(unit->is_point_reached_mid(Vect2f(point_.x, point_.y)) || !unit->checkImpassability(point_, unit->radius()))
			mode_ = RigidBodyUnit::ACTION_MODE_NONE;
		if(unit->unitOnTrail_ && !unit->trail()->isDocked() && !unit->pointInTrail(point_, unit->trail()->pose()))
			mode_ = RigidBodyUnit::ACTION_MODE_NONE;
		break;
	}
}

void RigidBodyUnit::enableBoxMode() 
{
	ptAction_.stop(ACTION_PRIORITY_HIGH);
	setFlyingMode(false);
	flyingHeightCurrent_ = 0;
	isBoxMode_ = true; 
	if(!angularEvolve)
		setAngle(ptPoseAngle);
	computeUnitVelocity();
	awake();
}

void RigidBodyUnit::disableBoxMode()
{ 
	isBoxMode_ = false;
	if(angularEvolve)
		ptPoseAngle = -atan2f(rotation()[0][1], rotation()[1][1]);
	else
		ptPoseAngle = angle();
	angularEvolve = !prm().moveVertical;
	groundZ = pose_.trans().z;
	awake();
}

void RigidBodyUnit::serialize(Archive& ar)
{
	ar.serialize(isBoxMode_, "isBoxMode", 0);
	if(isBoxMode_){
		ar.serialize(awakeCounter_, "awakeCounter", 0);
		if(!prm().moveVertical)
			ar.serialize(angularEvolve, "angularEvolve", 0);
		ar.serialize(velocity_, "velocity", 0);
	}else{
		ar.serialize(flyDownMode, "flyDownMode", 0);
		if(flyDownMode)
			ar.serialize(flyDownSpeed_, "flyDownSpeed", 0);
		ar.serialize(flyingHeightCurrent_, "flyingHeightCurrent", 0);
	}
	if(prm().waterAnalysis)
		ar.serialize(waterAnalysis_, "waterAnalysis", 0);
	ar.serialize(onDeepWater, "onDeepWater", 0);
	if(!flying_mode)
		ar.serialize(deepWaterChanged, "deepWaterChanged", 0);
	if(prm().flyingMode)
		ar.serialize(flying_mode, "flying_mode", 0);
	ar.serialize(isFrozen_, "isFrozen", 0);
	if(isFrozen_)
		ar.serialize(iceLevel_, "iceLevel", 0);
}
