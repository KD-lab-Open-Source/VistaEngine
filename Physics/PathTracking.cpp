#include "StdAfx.h"
#include "RenderObjects.h"
#include "Universe.h"
#include "RigidBody.h"
#include "RealUnit.h"
#include "terra.h"
#include "PFTrap.h"
#include "NormalMap.h"

//=======================================================
namespace {

	Vect2f temp_localwpoint; // Для функции sort()
	
	int maxLineNum;
	const Vect2f* maxLinePointer;
	float maxLineI;

	const float DT = 0.1f;
	const int BACK = 10;
	const int STOP = 0;
}

//=======================================================

#define NO_PATH 100
#define NO_OBSTACLES -1

inline bool isFreeLine(RigidBody * vehicle, vector<RigidBody*>& obstacleList,  int index);
void ImpassabilityCheck(RigidBody * vehicle);

//=======================================================
//	PathTracking functor.
//	Обработка препятствий + заставляет юнитов уступать дорогу.
//=======================================================
class UnitMovePlaner 
{
public:

	bool removeWPoint;
	Vect2f wpoint;
	Vect3f newWPoint;
	float ignoreRadius;
	bool enemyIgnoreRadius;
	vector<RigidBody*> obstacleList;

	UnitMovePlaner(const UnitBase& tracker, const UnitBase* ignoreUnit, bool checkRadius, bool enableMakeWay, bool enemyMakeWay = true)
	: tracker_(tracker), checkRadius_(checkRadius), ignoreUnit_(ignoreUnit), enemyMakeWay_(enemyMakeWay), enableMakeWay_(enableMakeWay){ 
		maxLine_ = NO_OBSTACLES; 
		removeWPoint = false;
		wpoint = Vect2f::ZERO;
		newWPoint = Vect3f::ZERO;
		ignoreRadius = 0;
		bool enemyIgnoreRadius = true;
	}

	void makeWay(UnitBase* p) {

		if(p->attr().isEnvironment() || !p->rigidBody()->prm().controled_by_points || !p->rigidBody()->enablePathTracking_) 
			return;
		
		if(p->attr().isResourceItem() || p->attr().isInventoryItem())
			return;

		bool enemyTest = enemyMakeWay_ || !p->isEnemy(&tracker_);
		if(enemyTest && p->rigidBody() && p->rigidBody()->way_points.empty() && !tracker_.rigidBody()->isAutoPTDir && tracker_.radius() > p->radius()) {
			Vect3f movDir(Vect3f::I);
			tracker_.pose().xformVect(movDir);
            Vect3f poseDelta = p->position() - tracker_.position();
			tracker_.pose().invXformVect(poseDelta);
			if(poseDelta.y > 0 && fabs(poseDelta.x) <= (p->radius()+tracker_.radius())) {
				p->rigidBody()->isAutoPTDir = true;
				p->rigidBody()->enableRunMode_ = true;
				if(poseDelta.x > 0)
					p->rigidBody()->autoPTDir = p->position() + movDir*(2.0f*p->radius()+tracker_.radius());
				else
					p->rigidBody()->autoPTDir = p->position() - movDir*(2.0f*p->radius()+tracker_.radius());
			}
		}
	}

	void operator () (UnitBase* p) {
	
		if(!p->alive() || !p->rigidBody())
			return;

		if(p == &tracker_ || !p->checkInPathTracking(&tracker_) || p == ignoreUnit_ || p->isDocked()) 
			return;
        
		makeWay(p);

		if(checkRadius_ && (enemyIgnoreRadius || !tracker_.isEnemy(p)))
			if(p->radius() <= ignoreRadius && tracker_.radius() > p->radius())
				return;

		if(tracker_.rigidBody()->ptObstaclesCheck_ && p->position2D().distance2(tracker_.position2D()) > sqr(p->radius() + tracker_.radius()))
			obstacleList.push_back(p->rigidBody());
		else if(tracker_.rigidBody()->ptObstaclesCheck_) {
			Vect2f wpoint = tracker_.position2D() - p->position2D();
			wpoint.Normalize(tracker_.radius());
			wpoint += tracker_.position2D();
			Vect2f forwardDir(2.0f*tracker_.radius(),0);
			forwardDir *= tracker_.rigidBody()->ptPose().rot;
			wpoint += forwardDir;
			tracker_.rigidBody()->setAutoPTPoint(to3D(wpoint,0));
		}

		if(!p->rigidBody()->unitMove()
			&& p->position2D().distance2(wpoint) < sqr(p->radius() + 0.5f * tracker_.radius())
			&& tracker_.position2D().distance2(wpoint) < sqr(p->radius() + 1.4f * tracker_.radius())) {
			Vect2f point = p->position2D() + Vect2f(p->radius() + 1.5f * tracker_.radius(),0);
			newWPoint = Vect3f(point.x, point.y, 0);
			removeWPoint = true;
		}

	}

	int freeLine() const { return maxLine_; }

	void callFreeLine() {

		if(obstacleList.empty())
			return;

		maxLineI = maxLineNum = -1;

		for(int i=0; i<(RigidBody::LINES_NUM*2+1); i++)
			if(isFreeLine(tracker_.rigidBody(), obstacleList, i)) {
				maxLine_ = i;
				return;
			}

	//	if(!vehicle->isManualMode())
			if(maxLineI >= 0)
				maxLine_ = maxLineNum;
			else
				maxLine_ = NO_PATH;

		maxLine_ = NO_PATH;
	}

private:
	const UnitBase& tracker_;
	const UnitBase* ignoreUnit_;
	int maxLine_;
	bool checkRadius_;
	bool enemyMakeWay_;
	bool enableMakeWay_;
};

//=======================================================
//	PathTracking Analysis.
//=======================================================
void RigidBody::tracking_analysis()
{
	onImpassability = pathFinder->impassabilityCheck(position().xi(), position().yi(), round(radius()));

	if(isAutoPTDir) {
		isAutoPTDir = !is_point_reached_mid(Vect2f(autoPTDir.x, autoPTDir.y));
		if(!isAutoPTDir)
			stop_quants = 1;
	}

	if (!way_points.empty() || isAutoPTDir || manualMoving)
		if((rotationMode || isAutoPTRotation) && !manualMoving)
			ptRotationMode();
		else {
			Vect3f prevVelTemp = Vect3f(0,moveback?-ptVelocity:ptVelocity,0);
			pose().xformVect(prevVelTemp);
			if(manualMoving && ownerUnit().attr().rotToTarget)
				ptRotationMode();
			ptGetVelocityDir();
			Vect2f curVelocityDir_ = Vect2f(moveback ? -1.0f : 1.0f, 0.0f);
			curVelocityDir_ *= ptPose_.rot;
			
			if((!ptImpassabilityCheck_ || !ptObstaclesCheck_) && sumAvrVelocityCount_ > BACK * 2){
				ptImpassabilityCheck_ = prm().ptImpassabilityCheck;
				ptObstaclesCheck_ = prm().ptObstaclesCheck;
				sumAvrVelocity_ = Vect2f::ID;
				sumAvrVelocityCount_ = 0;
			}
				
			if(sumAvrVelocityCount_ > BACK * 2){
				if(sumAvrVelocity_.norm2() < sqr(0.2f * sumAvrVelocityCount_)){
					ptImpassabilityCheck_ = false;
					ptObstaclesCheck_ = false;
				}
				sumAvrVelocity_ = Vect2f::ID;
				sumAvrVelocityCount_ = 0;
			}

			sumAvrVelocity_ += curVelocityDir_;
			sumAvrVelocityCount_++;
			
			if(onIce) {
				pose().invXformVect(prevVelocity_);
				prevVelocity_.y = prevVelocity_.z = 0;
				pose().xformVect(prevVelocity_);
				pose_.trans().add(prevVelocity_ * 5 * DT);
			}
			prevVelocity_ = prevVelTemp;
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
void RigidBody::ptRotationMode()
{
	float rotSpeed = onWater?ownerUnit().attr().angleSwimRotationMode/(1800.0/M_PI):ownerUnit().attr().angleRotationMode/(1800.0/M_PI);
	rotationSide = ROT_RIGHT;

	if(!isAutoPTRotation) {
		if(isManualMode())
			wpoint = manualViewPoint;
		else if (isAutoPTDir)
			wpoint = autoPTDir;
		else
			wpoint = way_points.front();

		ptPose_.trans = pose_.trans();
		ptPose_.rot.set(ptPoseAngle + M_PI/2.0);
		temp_localwpoint = ptPose_.invXform(wpoint);

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
void RigidBody::ptGetVelocityDir()
{

	start_timer_auto(ptGetVelocityDir, STATISTICS_GROUP_PHYSICS);
	
	// Заехал на горку - езжай назад.
	if(prm().path_tracking_back && forwardVelocity() < 0.001f){
		if(prm().path_tracking_back){
			moveback = !moveback;
			ptVelocity = 0;
			return;
		} else {
			enableRotationMode();
			return;
		}
	}

	// Если остановился, стоит stop_quants.
	if(stop_quants && stop_quants < STOP) {
		stop_quants++;	
		return;
	}

	wpoint = Vect2f::ZERO;
	if(isAutoPTDir)
		wpoint = Vect2f(autoPTDir);
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

	bool enableMakeWay = (!(ownerUnit().attr().isLiveItem() || ownerUnit().attr().isResourceItem() || ownerUnit().attr().isInventoryItem()) && GlobalAttributes::instance().enableMakeWay);
	UnitMovePlaner mp(ownerUnit(), ignoreUnit_,
		GlobalAttributes::instance().enablePathTrackingRadiusCheck, enableMakeWay, GlobalAttributes::instance().enableEnemyMakeWay);

	mp.wpoint = wpoint;
	mp.enemyIgnoreRadius = GlobalAttributes::instance().enemyRadiusCheck;
	mp.ignoreRadius = GlobalAttributes::instance().minRadiusCheck;
	float scanRadius = radius() + ptVelocity * DT * 15;
//	show_vector(position(), scanRadius, BLUE);
	universe()->unitGrid.Scan(ptPose_.trans.xi(),ptPose_.trans.yi(), round(scanRadius), mp);
	
	if(mp.removeWPoint)
		if(isAutoPTDir)
			autoPTDir = mp.newWPoint;
		else {
			UnitReal* trackerReal = const_cast<UnitReal*>(safe_cast<const UnitReal*>(&ownerUnit()));
			trackerReal->wayPoints_.clear();
			trackerReal->wayPoints_.push_back(mp.newWPoint);
//			pointCanNotByReached = true;
			return;
		}

	// Логика выбора линии движения...
	if(!manualMoving)
		if (moveback)
			if (temp_localwpoint.x > 0) ptUnsortBackLines();
			else ptUnsortBackLines();
		else
			if (temp_localwpoint.x < 0) ptSortLines();
			else ptSortLines();
	else
		ptManualLines();

	if(ptImpassabilityCheck_)
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

	// Если линия долго не свободна - пробует изменить направление (взад/вперед)
	if ((stop_quants>STOP)&&(move_line == NULL)){
		stop_quants = 0;
		ptVelocity = 0;
		if(prm().path_tracking_back)
			moveback = !moveback;
		else
			enableAutoPTRotation();
//		setVelocity(Vect3f::ZERO);
		return;
	}

	// Предотвращает верчение вокруг (.)
	if (!moveback && (back_quants < BACK)&&(wpoint.distance(ptPose_.trans) < 2.5f * callcVehicleRadius(path_tracking_angle, ptVelocity * DT))&&(wpoint.distance(ptPose_.trans) > callcVehicleRadius(path_tracking_angle, ptVelocity * DT))&&(temp_localwpoint.x<0)) {
		if(prm().path_tracking_back) {
			moveback = true;
			ptVelocity = 0;
			return;
		} else if(ownerUnit().attr().enableRotationMode){
			enableRotationMode();
			return;
		}
	}

	// Если близко к (.) то останавливать
	if ((wpoint.distance(ptPose_.trans) < 0.5f * callcVehicleRadius(path_tracking_angle, ptVelocity * DT)) && (fabs(temp_localwpoint.x)<0.3)) {
		pointCanNotByReached = true;
		return;
	}

	// Ждет - если нет свободных линий
	if (move_line == NULL){
		if(prm().path_tracking_back) {
			stop_quants++;	
			ptVelocity = 0;
		} else if(ownerUnit().attr().enableRotationMode)
			enableAutoPTRotation();
//		setVelocity(Vect3f::ZERO);
		return;
	}


	// Если назад дальше нельзя - переключается на перед.
	if ((moveback)&&(move_line == NULL)&&(back_quants!=0)){
		moveback = false;
		ptVelocity = 0;
//		setVelocity(Vect3f::ZERO);
		return;
	}

	// Назад ненужно ехать всегда.. вперед лучше 
	if (moveback) back_quants++;
	if (!manualMoving && back_quants > BACK && temp_localwpoint.x > 0){
		moveback = false;
		ptVelocity = 0;
		back_quants = 0;
//		setVelocity(Vect3f::ZERO);
		return;
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
	}

	set_debug_color(CYAN);
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
void RigidBody::ptSortLines()
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
void RigidBody::ptSortBackLines()
{
	for(int i=0;i<(LINES_NUM*2+1);i++)
		sortlines[i] = &backlines[i];

	temp_localwpoint = ptPose_.invXform(wpoint);
	temp_localwpoint.Normalize(1.0f);
	sort(&sortlines[0],&sortlines[LINES_NUM*2],FarestLine);
}

//=======================================================
void RigidBody::ptUnsortBackLines()
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
void RigidBody::ptUnsortLines()
{
	temp_localwpoint = ptPose_.invXform(wpoint);
	temp_localwpoint.Normalize(1.0f);

	for(int i=0;i<(LINES_NUM);i++)
	{
		sortlines[i*2+0] = &lines[i];
		sortlines[i*2+1] = &lines[LINES_NUM*2-i];
	}

	sortlines[LINES_NUM*2] = &lines[LINES_NUM];
}

//=======================================================
//	Проверяет пересечение юнита с несколькими препятствиями.
//=======================================================
__forceinline bool pointInObstacleList(Vect2f& vehiclePose, float vehicleRadius, vector<RigidBody*>& obstacleList, bool firstStep)
{
	vector<RigidBody*>::iterator it;
	FOR_EACH(obstacleList, it)
		if((!(*it)->unitMove() || firstStep) && pointInCircle(vehiclePose, (*it)->position(), (*it)->radius() + vehicleRadius))
			return true;

    return false;
}

//=======================================================
//	проверка с другими юнитами.. true - если линия свободна.
//=======================================================
inline bool isFreeLine(RigidBody * vehicle, vector<RigidBody*>& obstacleList,  int index)
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

	// Подстраховка на проникновения
//	vector<RigidBody*>::iterator it;
//	FOR_EACH(obstacleList, it){
//		float distance1 = sqr(vehicle->pose().trans().x - (*it)->pose().trans().x) + sqr(vehicle->pose().trans().y - (*it)->pose().trans().y);
//		if(distance1 < sqr((*it)->radius()+vehicle->radius())) {
//			temp_v = temp;
//			temp_v *= ppose;
//			float distance2 =sqr(temp_v.x - (*it)->pose().trans().x) + sqr(temp_v.y - (*it)->pose().trans().y);
//			if(distance1 <= distance2)
//				return true;
//		}
//	}

	for(int i=0; i<20; i++) {

		temp_v = temp;
		temp_v *= ppose;

		if(showDebugRigidBody.showPathTracking_ObstaclesLines && (i%2))
			show_line(Vect3f(ppose.trans, vehicle->position().z), Vect3f(temp_v, vehicle->position().z), GREEN);

//		if(pointInObstacleList(temp_v, vehicle->radius()*(1.0+i*0.01f), obstacleList, i < 5)){
		if(pointInObstacleList(temp_v, vehicle->radius(), obstacleList, i < 5)){
			if(i > maxLineI) {
				maxLineI = i;
				maxLineNum = index;
			}
			return false;
		}
		
		ppose.rot *= Mat2f(ang);
		ppose.trans = temp_v;
	}

	return true;
}

//=======================================================
// Проверка на пересечение с зонами непроходимости.
//=======================================================
inline bool ImpassabilityLine(RigidBody * vehicle, int index)
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

	for(int i=0; i<15; i++) {

		temp_v = temp;
		temp_v *= ppose;

		if(i && temp_v.distance2(vehicle->position()) > temp_localwpoint.norm2())
			return true;

//		if(!(i%2)){
			if(showDebugRigidBody.showPathTracking_ImpassabilityLines)
				show_line(Vect3f(ppose.trans, vehicle->position().z), Vect3f(temp_v, vehicle->position().z), GREEN);

			if(!vehicle->checkImpassability(Vect3f(temp_v, 0), vehicle->radius()*(1.0+i*0.01f))) {
				float newFactor = pathFinder->checkWaterFactor(temp_v.x, temp_v.y, vehicle->radius());
				if(checkNext && (newFactor > waterFactor || newFactor == 1 || newFactor == 0)){
					float val = ((RigidBody::LINES_NUM * 2 + 1) - index) * 0.3f + i;
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

		ppose.rot *= Mat2f(ang);
		ppose.trans = temp_v;
	}

	return true;
}

//=======================================================
void ImpassabilityCheck(RigidBody * vehicle)
{
	if(	vehicle->prm().groundPass == RigidBodyPrm::PASSABILITY &&
		vehicle->prm().waterPass == RigidBodyPrm::PASSABILITY &&
		vehicle->prm().impassabilityPass == RigidBodyPrm::PASSABILITY)
		return;

	maxLineI = 0;
	maxLinePointer = 0;
	int nullLineCount = 0;

	for(int i=0;i<(RigidBody::LINES_NUM*2+1);i++)
		if(vehicle->sortlines[i] && !ImpassabilityLine(vehicle, i)){
			vehicle->sortlines[i] = NULL;
			nullLineCount++;
		}

	if(nullLineCount == (RigidBody::LINES_NUM * 2 + 1) && maxLineI > 0)
		vehicle->sortlines[0] = maxLinePointer;
}

//=======================================================
// Прямое управление.
//=======================================================
void RigidBody::ptManualLines()
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

void RigidBody::manualTurnLeft()
{
	manualPTLine -= 0.5f;
}

void RigidBody::manualTurnRight()
{
	manualPTLine += 0.5f;
}

void RigidBody::manualMoveForward()
{
	if(!moveback)
		ptVelocity += prm().forward_acceleration * DT;
	else {
		ptVelocity -= prm().forward_acceleration * DT;
		if(ptVelocity < 0) {
			moveback = false;
			ptVelocity = -ptVelocity;
		}
	}
}

void RigidBody::manualMoveBackward()
{
	if(!moveback) {
		ptVelocity -= prm().forward_acceleration * DT;
		if(ptVelocity < 0) {
			moveback = true;
			ptVelocity = -ptVelocity;
		}
	} else
		ptVelocity += prm().forward_acceleration * DT;

}

