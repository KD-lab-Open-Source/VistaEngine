#ifndef __FORMATION_CONTROLLER_H__
#define __FORMATION_CONTROLLER_H__

#include "XMath\SafeMath.h"
#include "Util\Timers.h"
#include "RigidBodyUnit.h"
#include "MovementDirection.h"
#include "AttributeSquad.h"

class UnitSquad;
class UnitLegionary;

///////////////////////////////////////////////////////////////
//
//    class PossibleSector
//
///////////////////////////////////////////////////////////////

class PossibleSector
{
public:
	PossibleSector(float left, float right);
	float size() const { return cycleAngle(right_ - left_); }
	float left() const { return left_; }
	bool checkInSector(const MovementDirection& direction) const;
	MovementDirection getPossibleDirection(const MovementDirection& direction) const;
	

private:
	float left_;
	float right_;
	bool shifted_;
};

///////////////////////////////////////////////////////////////
//
//    class Passability
//
///////////////////////////////////////////////////////////////

class Passability
{
public:
	Passability(bool obstacleCheck = true);
	operator bool();
	operator int() const;
	const Passability& operator &= (const Passability&  passability);
	bool isFullyPassable() const;

	int obstaclePassability;
	int terrainPassability;
	
	static const int scanningDistance = 10;
};

///////////////////////////////////////////////////////////////
//
//    class PossibleMovementDirection
//
///////////////////////////////////////////////////////////////

class PossibleMovementDirection : public Passability
{
public:
	PossibleMovementDirection(const MovementDirection& direction);
	const PossibleMovementDirection& operator = (const Passability&  passability);
	MovementDirection& direction() { return direction_; }
	
private:
	MovementDirection direction_;
};

///////////////////////////////////////////////////////////////
//
//    class PossibleMovementDirections
//
///////////////////////////////////////////////////////////////

class PossibleMovementDirections : public vector<PossibleMovementDirection>
{
public:
	PossibleMovementDirections(const PossibleSector& sector, int linesNum);
	const PossibleMovementDirection& getOptimalDirection(bool moveLeft) const;
	bool checkLeftBeterThenRight() const;
	
private:
	const_iterator getOptimalLeftDirection() const;
	const_reverse_iterator getOptimalRightDirection() const;
};

///////////////////////////////////////////////////////////////

typedef vector<RigidBodyBase*> ObstacleList;

///////////////////////////////////////////////////////////////
//
//    class FormationUnit
//
///////////////////////////////////////////////////////////////

class FormationUnit
{
public:
	//////////////////////////////////////////////////////////
	
	enum RotationMode 
	{
		ROT_NONE,
		ROT_LEFT,
		ROT_RIGHT
	};

	//////////////////////////////////////////////////////////

	enum PTDirection 
	{
		PT_DIR_FORWARD,
		PT_DIR_LEFT,
		PT_DIR_RIGHT,
		PT_DIR_BACK
	};

	//////////////////////////////////////////////////////////

	FormationUnit(RigidBodyUnit* rigidBody);
	bool personalWayPoint() const;
	void setPersonalWayPoint(const Vect2f& wayPoint);
	void clearPersonalWayPoint();
    bool onImpassability() const { return onImpassability_; }
	float rotSpeed() const { return rotSpeed_; }
	void setRotationSpeed(float rotSpeed) { rotSpeed_ = unmovable() ? 0.0f : rotSpeed; } 
	float rotate(const MovementDirection& moveDirection);
	bool collidingObstacleList(const Vect2f& vehiclePosition, const Mat2f& vehicleOrientation, const ObstacleList& obstacleList, bool firstStep) const;
	float computeTerrainVelocityFactor(bool moveback) const;
	int computePassabilityFactor(const Vect2f& position, const Vect2f& newPosition) const;
	void setAdditionalHorizontalRot(float additionalHorizontalRot) { additionalHorizontalRot_ = additionalHorizontalRot; }
	void serialize(Archive& ar);
	void setFormationIndex(int index) { formationIndex_ = index; }
	int formationIndex() const { return formationIndex_; }
	void setOwnerUnit(UnitLegionary* unit);
	UnitLegionary& ownerUnit() const { xassert(ownerUnit_); return *ownerUnit_; }
	void setIgnoreUnit(const UnitBase* ignoreUnit) { ignoreUnit_ = ignoreUnit; }
	MovementDirection movingDirection(bool moveback);

	void initPose();

	void show();

	void checkPenetration();

	void quant();

	const MatX2f& ptPose() const { return ptPose_; }
	void setMovementMode(int movementMode) { movementMode_ = movementMode; }
	int movementMode() const { return movementMode_; }
	bool isRunMode() { return (movementMode_ & MODE_RUN) != 0;}
	bool isPointReached(const Vect2i& point);
	bool isPointReachedMid(const Vect2i& point);

	bool unmovable() const { return unmovable_; }
	void makeStatic();
	void makeDynamic() { unmovable_ = false; }

	bool unitMove() const;
	bool unitRotate() const;

	void resolvePenetration(const Vect2f& position, float radius);

	RotationMode getRotationMode() const { return rotationToPoint_ ? rotationSide_ : ROT_NONE; }
	void disableRotationMode() { rotationToPoint_ = false; }
	bool rotationMode() { return rotationToPoint_; }

	PTDirection ptDirection() const { return ptDirection_; }
	float ptDirectionAngle() const;
	void setPtDirection(PTDirection direction) { ptDirection_ = direction; }

	void enableManualMode() 
	{
		manualMoving_ = true;
		manualPTLine = 0.0f;
		setPtVelocity(0.0f);
	}
	void disableManualMode() 
	{ 
		manualMoving_ = false;
		setPtDirection(PT_DIR_FORWARD);
	}
	bool manualMoving() const { return manualMoving_; }

	void manualTurn(float factor);
	void manualMove(float factor, bool backward = false);

	void setManualViewPoint(const Vect2f& point) { manualViewPoint = point; }

	bool rot2pointMoving(const Vect2f& point);

	bool rot2point(const Vect2f& point);

	bool rot2direction(MovementDirection angleNew);

	float ptVelocity() const;
	void setPtVelocity(float ptVelocity);
	void postQuant(bool moveback);
	
	bool penetrationFound() const { return penetrationFound_; }
	void setPenetrationFound(bool penetrationFound);

	RigidBodyUnit* rigidBody_;
	const UnitBase* ignoreUnit_;

private:
	void ptRotationMode();
	bool checkObstacles();
	float computeVerticalFactor(const Vect2f& position, const Vect2f& dir) const;

	UnitLegionary* ownerUnit_;
	float ptVelocity_;
	MatX2f ptPose_; 
	float manualPTLine;
	Vect2f manualViewPoint;
	PTDirection ptDirection_;
	float curVerticalFactor_;
	bool manualMoving_;
	bool unmovable_;
	bool onImpassability_;
	bool penetrationFound_;
	bool rotationToPoint_;
	float additionalHorizontalRot_;
	RotationMode rotationSide_;
	int movementMode_;
	Vect2f pointToRotate_;
	float rotSpeed_;
	int formationIndex_;

	static const int LINES_NUM = 5;
};

///////////////////////////////////////////////////////////////
//
//    class FormationController
//
///////////////////////////////////////////////////////////////

class FormationController
{
public:
	///////////////////////////////////////////////////////////

	enum PTActionPriority 
	{
		ACTION_PRIORITY_DEFAULT = 0,
		ACTION_PRIORITY_PATH_TRACKING = 1,
		ACTION_PRIORITY_HIGH = 2
	};

	///////////////////////////////////////////////////////////

	typedef vector<Vect2f> WayPoints;

	///////////////////////////////////////////////////////////

	FormationController(const AttributeSquad::Formation* formation, bool forceUnitsAutoAttack);
	void initPose(const Vect2f& position, const MovementDirection& orientation);
	void changeFormation(const AttributeSquad::Formation* formation);
	void computePose();
	void interpolatePose();
	void computeRadius();
	const Vect2f& position() const { return position_; }
	const MovementDirection& orientation() const { return orientation_; }
	float radius() { return radius_; }
	bool checkNumber(UnitFormationTypeReference unitType, int numberUnits) const;
	void addUnit(FormationUnit* unit);
	void setMainUnit(FormationUnit* mainUnit);
	Se3f getNewUnitPosition(FormationUnit* unit);
	void removeUnit(FormationUnit* unit);
	void setDisablePathTracking(bool disablePathTracking) { disablePathTracking_ = disablePathTracking; }
	void addWayPointS(const Vect2f& point);
	void setWayPoint(const Vect2f& point);
	bool wayPointEmpty() const { return !wayPointSet_; }
	const Vect2f& lastWayPoint() const { return wayPoint_; }
	const WayPoints& wayPoints() const { return wayPoints_; }
	void wayPointsClear();
	bool moveAction(Vect2f& point, PTActionPriority priority = ACTION_PRIORITY_DEFAULT) { return ptAction_.setMovePoint(point, priority); }
	void setFollowSquad(UnitSquad* squadToFollow) { followSquad_ = squadToFollow; }
	bool followSquad() const { return followSquad_ != 0; }
	int impassableTerrainTypes() const { return impassability_; }
	int passableObstacleTypes() const { return passability_; }
	void stateQuant();
	void quant();
	void showDebugInfo();
	void serialize(Archive& ar);

private:
	///////////////////////////////////////////////////////////
	//
	//    class PTAction
	//
	///////////////////////////////////////////////////////////

	class PTAction 
	{
	public:
		PTAction() 
			: mode_(ACTION_MODE_NONE)
			, priority_(ACTION_PRIORITY_DEFAULT)
			, point_(Vect2f::ZERO) 
		{
		}

		bool active() const { return mode_ != ACTION_MODE_NONE; }
		bool isMove() const {return mode_ == ACTION_MODE_MOVE; }
		const Vect2f& point() const { return point_; }
		void setPoint(const Vect2f& point) { point_ = point; }
		void checkFinish(FormationController* unit);
		bool setMovePoint(const Vect2f& point, PTActionPriority priority = ACTION_PRIORITY_DEFAULT);
		void stop(PTActionPriority priority = ACTION_PRIORITY_DEFAULT);

	private:
		///////////////////////////////////////////////////////

		enum PTActionMode 
		{
			ACTION_MODE_NONE,
			ACTION_MODE_MOVE
		};

		///////////////////////////////////////////////////////

		Vect2f point_;
		PTActionMode mode_;
		int priority_;
		LogicTimer timer_;
	};

	///////////////////////////////////////////////////////////////

	class FormationUnits : public vector<FormationUnit*>
	{
	public:
		FormationUnits()
			: firstInactive_(0)
			, leader_(0)
		{
		}

		bool active() const { return firstInactive_ > 0; }

		int activeSize() const { return firstInactive_; }

		iterator inactive()
		{
			return begin() + firstInactive_;
		}

		const_iterator inactive() const
		{
			return begin() + firstInactive_;
		}

		void makeInactive(iterator val)
		{
			xassert(val - begin() < firstInactive_);
			--firstInactive_;
			if(val - begin() != firstInactive_)
				std::swap(*val, *inactive());
			
		}

		void makeAllActive() 
		{ 
			firstInactive_ = size(); 
			if(empty() || leader_ == leader())
				return;
			iterator it = find(begin() + 1, inactive(), leader_);
			if(it != end())
				std::swap(front(), *it);
		}

		void makeActive(iterator val)
		{
			xassert(val - begin() >= firstInactive_);
			if(val - begin() != firstInactive_)
				std::swap(*val, *inactive());
			++firstInactive_;
			reference lastActive = *(inactive() - 1);
			if(activeSize() > 1 && lastActive == leader_)
				std::swap(front(), lastActive);
				
		}

		void push_back(FormationUnit* val)
		{	
			if(!leader_)
				leader_ = val;
			vector<FormationUnit*>::push_back(val);
			if(size() != firstInactive_ + 1)
				std::swap(back(), *inactive());
			++firstInactive_;
		}

		iterator erase(iterator val)
		{
			if(val - begin() < firstInactive_)
				--firstInactive_;
			return vector<FormationUnit*>::erase(val);
		}

		void setLeader(FormationUnit* leader)
		{
			if(leader_ == leader)
				return;
			iterator it = find(begin() + 1, inactive(), leader);
			if(it != end())
				std::swap(front(), *it);
			leader_ = leader;
		}

		FormationUnit* leader() const { return front(); }
        
	private:
		FormationUnit* leader_;
		unsigned int firstInactive_;
	};

	///////////////////////////////////////////////////////////

	bool controlled() const;
	FormationUnit* leader() const { return units_.leader(); }
	bool isIgnoreUnit(UnitBase* p);
	const Vect2f& wayPoint() const { return ptAction_.active() ? ptAction_.point() : wayPoints().empty() ? position() : wayPoints().front(); }
	bool emptyFormationPattern() const { return !formation_ || formation_->formationPattern == FormationPatternReference(); } 
	bool personalWayPointsEmpty() const;
	void addToFormation(FormationUnit* unit);
	void removeFromFormation(FormationUnit* unit);
	void updateParameters();
	float computeVelocityMax() const;
	float velocity() const { return velocity_; }
	bool moveback() const { return moveback_; }
	Vect2f getUnitFormationPosition(const Vect2f& point, const FormationUnit* unit) const;
	Vect2f getUnitAttackPosition(const Vect2f& point, const FormationUnit* unit) const;
	Vect2f getUnitPosition(const Vect2f& point, const FormationUnit* unit, bool moving) const;
	Vect2f getUnitLastPosition(const FormationUnit* unit) const;
    Vect2f getUnitPositionDelta(const Vect2f& position, const FormationUnit* unit, bool moving = true) const;
	MovementDirection correctUnitDirection(const FormationUnit* unit);
	MovementDirection correctUnitDirection(const FormationUnit* unit, const MovementDirection& direction);
	void velocityProcessing();
	void stopMoving();
	float velocityMax() const { return velocityMax_; }
	float velocityMaxPermanent() const { return velocityMaxPermanent_; }
	float forwardAcceleration() const { return forwardAcceleration_; }
	float pathTrackingAngle() const { return pathTrackingAngle_; }
	bool obstacleCheck() const { return obstacleCheck_; }
	bool disablePathTracking() const { return disablePathTracking_; }
	bool canRotate() const { return canRotate_; }
	bool canMoveBack() const { return canMoveBack_; }
	bool manualMoving() const { return false; }
	void setFormation(const AttributeSquad::Formation* formation);
	void findPath();
	void addWayPointAttack(const Vect2f& point);
	bool isMidPointReached(const Vect2f& point);
	bool isLastPointReached();
	Vect2f wayPointDir() const;
	bool wayPointFar() const;
    MovementDirection computePreferedDirection() const;
	PossibleSector computeForwardSector() const;
	PossibleSector computeBackwardSector() const;
	void move(float rotSpeed, bool ignoreAngles = false);
	void moveForward(const MovementDirection& direction, bool ignoreAngles = false);
	void moveBackward(const MovementDirection& direction);
	void rotate(const MovementDirection& moveDirection);
	Passability checkPassability(Vect2f moveLine, const ObstacleList& obstacleList, float orientation, float rotAngle, int scanningDistance = Passability::scanningDistance);
	bool wayPointOnClosedGraph(const MovementDirection& moveDirection);
	bool checkPassabilityForward(const MovementDirection& preferedDirection, const ObstacleList& obstacleList, bool inForwardSector);
	bool checkPassabilityBackward(MovementDirection& preferedDirection, const ObstacleList& obstacleList);
	bool rotateToDirection(const MovementDirection& moveDirection, const ObstacleList& obstacleList);
	void computeMoveLinesPassability(PossibleMovementDirections& directions, const ObstacleList& obstacleList);
	void moveAlongWall(const ObstacleList& obstacleList);
	void pathTracking();
	
	int stopFunctorFirstStep;
	UnitLink<UnitSquad> followSquad_;
	FormationUnits units_;
	Vect2f position_;
	MovementDirection orientation_;
	float radius_;
	float velocity_;
	float velocityMax_;
	float velocityMaxPermanent_;
	float forwardAcceleration_;
	float pathTrackingAngle_;
	float positionInterpolationFactor_;
	bool goingAroundObstacle_;
	bool goingAroundBackObstacle_;
	bool goingLeftAroundObstacle_;
	bool obstacleCheck_;
	bool disablePathTracking_;
	bool followMainUnitInAutoMode_;
	bool canRotate_;
	bool canMoveBack_;
	bool alwaysMoving_;
	bool moveback_;
	bool wayPointSet_;
	Vect2f wayPoint_;
	Vect2f autoPosition_;
	WayPoints wayPoints_;
	LogicTimer recalcPathTimer_;
	int impassability_;
	int passability_;
	float velocityFactorsByTerrain_[TERRAIN_TYPES_NUMBER];
	PTAction ptAction_;
	int stopQuants_;
	const AttributeSquad::Formation* formation_;
	vector<bool> freePositions_;
#ifndef _FINAL_VERSION_
	string debugMessageList_;
	void debugMessage(const char* text);
#else
	void debugMessage(const char* text) {}
#endif

	friend class UnitMovePlaner;
};

#endif // __FORMATION_CONTROLLER_H__