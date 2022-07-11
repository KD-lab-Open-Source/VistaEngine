#ifndef __RIGID_BODY_UNIT_H__
#define __RIGID_BODY_UNIT_H__

#include "RigidBodyBox.h"
#include "Timers.h"

class WhellController;
class RigidBodyTrail;

///////////////////////////////////////////////////////////////
// RigidBodyUnit
///////////////////////////////////////////////////////////////

class RigidBodyUnit : public RigidBodyBox {
public:
	
	RigidBodyUnit();

	enum RotationMode {
		ROT_NONE,
		ROT_LEFT,
		ROT_RIGHT
	};
	
	enum PTDirection {
		PT_DIR_FORWARD,
		PT_DIR_LEFT,
		PT_DIR_RIGHT,
		PT_DIR_BACK
	};
	
	enum PTActionMode {
		ACTION_MODE_NONE,
		ACTION_MODE_ROTATION,
		ACTION_MODE_MOVE
	};

	enum PTActionPriority {
		ACTION_PRIORITY_DEFAULT = 0,
		ACTION_PRIORITY_PATH_TRACKING = 1,
		ACTION_PRIORITY_HIGH = 2
	};

	class PTAction {

		Vect3f point_;
		PTActionMode mode_;
		int priority_;
		DurationTimer timer_;
	
	public:

		PTAction():mode_(RigidBodyUnit::ACTION_MODE_NONE), priority_(RigidBodyUnit::ACTION_PRIORITY_DEFAULT), point_(Vect3f::ZERO) {}

		bool active() const { return mode_ != RigidBodyUnit::ACTION_MODE_NONE; }
		bool isRotation() const {return mode_ == RigidBodyUnit::ACTION_MODE_ROTATION; }
		bool isMove() const {return mode_ == RigidBodyUnit::ACTION_MODE_MOVE; }

		Vect3f& point() { return point_; }

		void checkFinish(RigidBodyUnit* unit);

		bool set(PTActionMode mode, const Vect3f& point, PTActionPriority priority)
		{
			if(!active() || priority_ <= priority){
				priority_ = priority;
				point_ = point;
				mode_ = mode;
				timer_.start(10000);
				return true;
			}
			
			return false;
		}

		bool setMovePoint(const Vect3f& point, PTActionPriority priority = RigidBodyUnit::ACTION_PRIORITY_DEFAULT) { return set(RigidBodyUnit::ACTION_MODE_MOVE, point, priority); }
		bool setRotationPoint(const Vect3f& point, PTActionPriority priority = RigidBodyUnit::ACTION_PRIORITY_DEFAULT) { return set(RigidBodyUnit::ACTION_MODE_ROTATION, point, priority); }

		void stop(PTActionPriority priority = RigidBodyUnit::ACTION_PRIORITY_DEFAULT)
		{
			if(active() && priority_ <= priority) {
				mode_ = RigidBodyUnit::ACTION_MODE_NONE;
				timer_.stop();
			}
		}
	};

	void build(const RigidBodyPrm& prm, const Vect3f& boxMin_, const Vect3f& boxMax_, float mass);

	void setOwnerUnit(UnitBase* unit) { ownerUnit_ = unit; }
	void setIgnoreUnit(const UnitBase* ignoreUnit) { ignoreUnit_ = ignoreUnit; }
	const UnitBase& ownerUnit() const { xassert(ownerUnit_); return *ownerUnit_; }

	bool evolve(float dt); // возвращает, было ли перемещение
	void ptUnitUpdateZPose(QuatF& poseQuat);
	void ptUnitEvolve();
	void ptUnitUpdatePose();

	void setPose(const Se3f& pose);
	void initPose(const Se3f& pose); // Вызывать при первой установке
	
	const MatX2f& ptPose() const { return ptPose_; }
	
	float forwardVelocity() const { return forwardVelocity_; }
	void setForwardVelocity(float v){ forwardVelocity_ = v; }
	void setRunMode(bool enableRM = true) { enableRunMode_ = enableRM; }
	bool isRunMode() { return enableRunMode_;}
	bool is_point_reached(const Vect2f& point);
	bool is_point_reached_mid(const Vect2f& point);
	void setWhellController(WhellController* controller) { whellController = controller; }

	/// Принудительное включение точечного анализа поверхности.
	void setPointAreaAnalize() { pointAreaAnalize_ = true; }
	/// Включение анализа поверхности по умолчанию.
	void setDefaultAreaAnalize() { pointAreaAnalize_ = false; }
	
	void enableWaterAnalysis();
	void disableWaterAnalysis();

	// States
	bool groundColliding() const { return colliding_ & GROUND_COLLIDING; } // сталкивается, стоит на земле
	bool waterColliding() const { return colliding_ & WATER_COLLIDING; } // сталкивается, стоит на воде
	bool submerged() const;

	bool unmovableXY() const { return unmovableXY_; } // В данный момент просто отключает PathTracking.. нет движения в XY плоскости.
	void makeStaticXY() { unmovableXY_ = true;  }
	void makeDynamicXY() { unmovableXY_ = false; }
	
	void setFlyingMode(bool mode, int time = 0); // включает полет
	bool flyingMode() const { return flying_mode && (controlled() || !prm().flying_down_without_way_points || obstaclesFound_); }
	float flyingHeight() const { return flyingHeight_; }
	void setFlyingHeight(float flyingHeight) { flyingHeight_ = prm().undergroundMode ? -flyingHeight : flyingHeight; }
	void setFlyDownTime(int time) { flyDownTime = time; }

	bool checkImpassability(const Vect3f& pos, float radius) const;
	bool checkImpassabilityStatic(const Vect3f& pos) const;

	void setEnablePathTracking(bool enablePathTracking) { enablePathTracking_ = enablePathTracking; }
	void setPathTrackingAngle(float angle) { path_tracking_angle = angle; callcPathTrackingLines();	}

	void callcPathTrackingLines();
	bool unitMove() const { return !unmovable_ && !unmovableXY_ && prm().controled_by_points && ((manualMoving && ptVelocity > 0) || ((!way_points.empty() || ptAction_.isMove()) && !stop_quants && !manualMoving)); }
	bool unitMoveOrRotate() const { return !unmovable_ && !unmovableXY_ && prm().controled_by_points && ((manualMoving && ptVelocity > 0) || ((!way_points.empty() || ptAction_.active()) && !stop_quants && !manualMoving)); }

	RotationMode getRotationMode() const { if(rotationMode) return rotationSide; else return ROT_NONE; }
	void enableRotationMode() { rotationMode = true; }
	void disableRotationMode() { rotationMode = false; }

	void clearAutoPT(PTActionPriority priority = ACTION_PRIORITY_DEFAULT) { ptAction_.stop(priority); }
	bool isAutoPTMode() { return ptAction_.isMove(); }
	bool setAutoPTPoint(const Vect3f& point);
	
	bool turnAction(PTActionMode mode, Vect3f& point, PTActionPriority priority = ACTION_PRIORITY_DEFAULT) { return ptAction_.set(mode, point, priority); }
	bool turnMoveAction(Vect3f& point, PTActionPriority priority = ACTION_PRIORITY_DEFAULT) { return ptAction_.setMovePoint(point, priority); }
	bool turnRotationAction(Vect3f& point, PTActionPriority priority = ACTION_PRIORITY_DEFAULT) { return ptAction_.setRotationPoint(point, priority); }
	void stopAction(PTActionPriority priority = ACTION_PRIORITY_DEFAULT) { ptAction_.stop(priority); }
	
	float getWhellAngle() { return (moveback ? -3 : 3) * lastPTAngle; }
	float getWhellVelocity() { return unmovable_ || unmovableXY_ ? 0 : (moveback ? -ptVelocity : ptVelocity); }

	PTDirection ptDirection() const { return ptDirection_; }
	void setPtDirection(PTDirection direction) { ptDirection_ = direction; }
	
	void computeUnitVelocity();

	/// Возвращает скорость юнита с учетом всех режимов движения.
	const Vect3f& velocity();

	void disableFlyDownMode() { flyDownMode = false; flyingHeightCurrent_ = 0.0f; }
	void enableFlyDownModeByTime(int time);
	void setFlyingHeightCurrent(float flyingHeightCurrent) { flyingHeightCurrent_ = flyingHeightCurrent; }
	bool isFlyDownMode() { return flyDownMode; }
	float flyDownSpeed() { return flyDownSpeed_; }
	float flyingHeightCurrent() { return flyingHeightCurrent_; }
	// Control
	vector<Vect3f> way_points;
	
	// PathTracking		
	enum { LINES_NUM = 5 };
	const Vect2f* sortlines[LINES_NUM*2+1];
	void wayPointsChecker();

	// State flags
	bool onSecondMap;
	bool onDeepWater;
	bool deepWaterChanged;
	bool isUnderWaterSilouette;
	bool isUnderWaterSilouettePrev;
	bool onImpassability;
	bool onImpassabilityFull;
	bool manualMoving;

	void enableManualMode() { manualMoving = true; manualPTLine = 0; ptVelocity = 0; }
	void disableManualMode() { manualMoving = false; ptDirection_ = PT_DIR_FORWARD; }
	bool isManualMode() { return manualMoving; }

	void manualTurnLeft(float factor);
	void manualTurnRight(float factor);
	void manualMoveForward(float factor);
	void manualMoveBackward(float factor);

	void setManualViewPoint(const Vect3f& point) { manualViewPoint = point; }

	void callcVerticalFactor();
	float verticalFactor() const { return curVerticalFactor_; }
	void ptRotationMode();

	bool isBoxMode() const { return isBoxMode_; }
	virtual void enableBoxMode();
	virtual void disableBoxMode();
	float getPoseAngle() { return ptPoseAngle; }
	bool rot2point(const Vect3f& point); // false - не повернут к точке и включает поворот.

	void setTrail(RigidBodyTrail* trail) { trail_ = trail; }
	RigidBodyTrail* trail() { return trail_; }

	void show();

	static void clearPathTracking();

	void serialize(Archive& ar);

protected:
	float groundZ;
	Vect3f groundNormal;
		
	UnitBase* ownerUnit_;
	bool isBoxMode_;

	//=======================================================
	//	Trail system
	//=======================================================
	RigidBodyTrail* trail_;
	bool unitOnTrail_;
	Vect3f localPose;

	bool getTrailPoint(const Vect2f position, float& z); // Возвращает высоту по баунду. Значения в глобальных коорд.
	Vect3f xformTrailToGlobal(const Vect2f position); // Параметр в локальных коорд.
	bool pointInTrail(const Vect2f& point, const Se3f& trailPose );
	void clampUnitOnTrailPosition();
	bool checkTrailWayPoint(Vect2f& point); // Разрешенно ли идти к этой точке.

	//=======================================================
	//	Параметры для PathTracking-а
	//=======================================================
	bool moveback;
	int back_quants;
	int stop_quants;
	Vect2f wpoint;
	MatX2f ptPose_; 
	float ptVelocity;
	Vect2f lines[LINES_NUM*2+1];
	Vect2f backlines[LINES_NUM*2+1];
	float path_tracking_angle;
	bool rotationMode;
	RotationMode rotationSide;
	PTAction ptAction_;
	const UnitBase* ignoreUnit_;
	Vect3f prevVelocity_;
	float manualPTLine;
	Vect3f manualViewPoint;
	PTDirection ptDirection_;
	float curVerticalFactor_;
	float ptPoseAngle;
	QuatF ptAdditionalRot; // Дополнительное вращение вызванное PT. (заносы, скальжение и т.п. )
	WhellController * whellController;
	bool flyDownMode;
	float flyDownSpeed_;

	bool ptObstaclesCheck_;
	bool ptImpassabilityCheck_;
	Vect2f sumAvrVelocity_;
	int sumAvrVelocityCount_;
	int stopFunctorFirstStep;

	float lastPTAngle;
	bool isAutoPTRotation;
	void enableAutoPTRotation() { isAutoPTRotation = true; }

	bool pointAreaAnalize_; // true - принудительный точечный анализ поверхности.
	
	float forwardVelocity_;
	bool enableRunMode_;
	float flyingHeight_;
	float flyingHeightCurrent_;
	int flyDownTime;
	
	bool unmovableXY_;
	bool holdOrientation_;
	bool flying_mode; // включает полет
	bool flyingModePrev;
	bool obstaclesFound_;

	bool enablePathTracking_;

	float waterLevelPrev;

	bool controlled() const { return !way_points.empty() || ptAction_.active() || manualMoving; }

	//=======================================================
	//	PathTracking
	//=======================================================
	void tracking_analysis();
	bool ptGetVelocityDir(); // true - позиция изменилась.
	void ptSortLines();
	void ptSortBackLines();
	void ptUnsortBackLines();
	void ptManualLines();
	void ptUnsortLines();
	void ptFrontMoveLines();
	bool wpointInVehicleRadius();
	friend inline bool isFreeLine(RigidBodyUnit * vehicle, vector<RigidBodyBase*>& obstacleList,  int index);
	friend inline bool ImpassabilityLine(RigidBodyUnit * vehicle, int index);
	friend class UnitMovePlaner;
	friend class MakeWayPlaner;
	friend class ClearRegionOp;
	friend class StopFunctor;
	QuatF placeToGround(float& groundZNew);
	void setDeepWater(bool deepWater);
	float checkDeepWater(float soilZ, float waterZ);


	//=======================================================
	//	Проверка зон проходимости.
	//=======================================================
	bool groundCheck(int xc, int yc, int r) const;// возвращает false если вода не покрывает весь регион.
	bool secondMapCheck(int xc, int yc, int r) const;
	bool secondMapNegativeCheck(int xc, int yc, int r) const;
	
	float analyzeAreaFast(const Vect2i& center, int r, Vect3f& normalNonNormalized);
	float analyzeWaterFast(const Vect2i& center, int r, Vect3f& normalNonNormalized);

	bool checkObstacles();

};

#endif // __RIGID_BODY_UNIT_H__
