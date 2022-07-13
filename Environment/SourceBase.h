#ifndef __BASE_SOURCE_H__
#define __BASE_SOURCE_H__

#include "SerializationTypes.h"
#include "Handle.h"
#include "..\Units\CircleManagerParam.h"
#include "Range.h"
#include "TypeLibrary.h"
#include "..\Units\BaseUniverseObject.h"
#include "SafeCast.h"
#include "..\Util\Timers.h"
#include "..\Util\EffectReference.h"
#include "..\Units\EffectController.h"
#include "..\Units\WeaponEnums.h"
#include "Grid2D.h"
#include "UnitLink.h"

class SourceBase;

class WeaponSourcePrm;
class WeaponTarget;

class UnitBase;

class Player;

typedef StringTable<StringTableBasePolymorphic<SourceBase> > SourcesLibrary;
typedef StringTablePolymorphicReference<SourceBase, false> SourceReference;

enum SurfaceKind {
	/// ����������� 1 ����
	SURFACE_KIND_1 = 1,
	/// ����������� 2 ����
	SURFACE_KIND_2 = 1 << 1,
	/// ����������� 3 ����
	SURFACE_KIND_3 = 1 << 2,
	/// ����������� 4 ����
	SURFACE_KIND_4 = 1 << 3
};

class SourceAttribute;

enum SourceType {
	SOURCE_WATER,
	SOURCE_BUBBLE,
	SOURCE_LIGHT,
	SOURCE_BREAK,
	SOURCE_GEOWAVE,
	SOURCE_TERROOL,
	SOURCE_ICE,
	SOURCE_FREEZE,
	SOURCE_ZONE,
	SOURCE_IMPULSE,
	SOURCE_TORNADO,
	SOURCE_BLAST,
	SOURCE_LIGHTING,
	SOURCE_CAMERA_SHAKING,
	SOURCE_FLASH,
	SOURCE_DETECTOR,
	SOURCE_WATER_WAVE,
	SOURCE_SHIELD,
	SOURCE_TELEPORT,
	SOURCE_DELETE_GRASS,
	SOURCE_FLOCK,
	SOURCE_MAX
};

class SourceAttribute
{
public:
	SourceAttribute();

	void serialize(Archive& ar);

	bool isEmpty() const { return !sourceReference_; }
	const SourceBase* source() const { return sourceReference_; }
	const SourceReference attr() const { return sourceReference_; }

	float activationDelay() const { return activationDelay_; }
	float lifeTime() const { return lifeTime_; }

	const SourceAttribute* getByKey(const char* key) const;

protected:
	/// ���� ��� ������ �� ������
	string key_;
private:
	/// �������� ������
	float activationDelay_;
	/// ����� �����
	float lifeTime_;
	/// ������ �� �������� �� ����������
	SourceReference sourceReference_;
};

class SourceBase : public BaseUniverseObject, public GridElementType, public PolymorphicBase
{
	friend class Environment;
public:
    struct PathPosition : Vect3f {
        PathPosition()
        : velocity_(10.0f)
        {
            x = 0.0f;
            y = 0.0f;
			z = 0.0f;
        }
        float velocity_;
        void serialize(Archive& ar);
    };

    struct Path : std::vector<PathPosition> {
        Path() 
        : cycled_(false), position_(0.0f), segment_(0), speed_(1.0f)
        {}
        float length() const;
		float period() const;
        void serialize(Archive& ar);
		
		void quant();
        Vect3f currentPosition() const;
        bool cycled_;

		void init();

		float position_;
		float speed_;
		int segment_;
    };


	enum TargetClass{
		SOURCE_ATTACK_AIR,
		SOURCE_ATTACK_ALL,
		SOURCE_ATTACK_GROUND
	};

	/// �����������, ��� �������� ����� ��������
	enum SurfaceClass {
		/// �����
		SOURCE_SURFACE_ANY,
		/// ������ �� �����
		SOURCE_SURFACE_GROUND,
		/// ������ �� ����
		SOURCE_SURFACE_WATER,
		/// ������ �� ����
		SOURCE_SURFACE_ICE
	};

	enum {
		/// ������������ ����� ����� ���������, ������������ �� ���������
		MAX_DEFAULT_LIFETIME = 10000
	};

	enum PlacementMode {
		// �� �������
		PLACE_NONE,
		// ������� �� �����
		PLACE_TO_GROUND,
		// ������� �� ����
		PLACE_TO_WATER
	};

	/// ��� �����������, ��� �������� ����� ��������
	typedef BitVector<SurfaceKind> SurfaceKindBitVector;

	static const char* MapName[SOURCE_MAX];

    SourceBase();
    SourceBase(const SourceBase& src);
    virtual ~SourceBase();
	
	virtual SourceType type() const = 0;
	virtual SourceBase* clone() const = 0;

	virtual void quant();
	virtual void showEditor() const;

	virtual void apply(UnitBase* target);
	virtual bool canApply(const UnitBase* target) const;

	void setPose(const Se3f& pos, bool init);
	virtual Se3f currentPose() {return pose_;}

	virtual void setRadius(float _radius);

	void setActivity(bool makeActive);
	bool active() const { return active_; }

	bool enabled() const { return enabled_; }

	void setName(const char* name) { name_ = name; }
	const char* key() const { return name_.c_str(); }

	const char* label() const { return label_.c_str(); }

	time_type getActivationTime() const { return activationTimer_.was_started() ? global_time() - activationTimer_.get_start_time() : 0; }
	void setActivationTime(int time){ activationTimer_.start(time); }
	bool needActivation() const { return !active_ && isAlive() && activationTimer_(); }

	bool isKillTimerStarted()const { return (killTimer_.was_started() || (!isUseKillTimer())); }
	void setKillTimer(int time){ lifeTime_ = time; killTimer_.start(time); }
	int lifeTime() const { return lifeTime_; }
	bool isAlive() const { return deadQuant_ == 0; }

	void kill();
	bool isDead() const { return deadQuant_ > 4; }

	Player* player() const{
		return player_ ? player_ : worldPlayer();
	}
	void setPlayer(Player* player) { player_ = player; }

	void setOwner(UnitBase* unit);
	UnitBase* owner() const;

	virtual void setWaitingTarget(bool wait){ waiting_target_ = wait; }
	bool waiting_target() const { return waiting_target_; }

	virtual void setTargetClass(TargetClass tc){ targetClass_ = tc; }
	TargetClass targetClass() const { return targetClass_; }

	virtual void serialize(Archive& ar);
	static string getDisplayName(SourceType type);

	virtual void showDebug() const;

	bool followPath() const { return followPath_; }
	Path& path() { return path_; }
	const Vect3f& origin() { return origin_; }

	SurfaceClass surfaceClass() const { return surfaceClass_; }
	unsigned char surfaceKind() const { return surfaceKind_; }

	void setAffectMode(AffectMode mode);

	void setDetected();
	bool detected() const { return showTimer_(); }

	virtual bool getParameters(WeaponSourcePrm& prm) const { return false; }
	virtual bool setParameters(const WeaponSourcePrm& prm, const WeaponTarget* target = 0){ return false; }

	UniverseObjectClass objectClass() const{ return UNIVERSE_OBJECT_SOURCE; }

	void mapUpdate(float x0,float y0,float x1,float y1);

	const SourceAttribute* getChildSourceByKey(const char* key) const;

	/// ���������� � ������������ ����������
	struct ChildSource{
		ChildSource() : generationDelay_(0.f) {}
		/// ����� ����� �����������
		float generationDelay_;
		DelayTimer activationTimer_;
		/// ������ �� �������� �� ����������
		SourceAttribute source_;

		void serialize(Archive &ar);
	};
	typedef std::list<ChildSource> ChildSources;

	const ChildSources& childSources() const{ return childSources_; }
protected:
	enum{
		ENVIRONMENT_POINTS_COUNT = 7
	};
	virtual void start();
	virtual void stop();

	bool isDetonator(const UnitBase* target) const;

	/// �������� � �������� ����������� ������� � ����� ������������
	void serializationApply(const Archive& ar);

	void soundInit();

	AffectMode affectMode_;
	AffectMode activatorMode_;
	bool active_;
	/// � ���� ����
	bool targetInZone;
	/// ���� ��������� ��� ��������� ����� � ����
	bool waiting_target_;
	/// �������������� ������ �����������
	bool activate_by_detonator_;
	/// �������������� ��� ��������� �������
	bool activate_by_projectile_;
	/// ��� ������������� ������� ������������ ��������
	bool killOwner_;
	/// �������� ���������
	bool activation_started_;
	/// �������� ������
	bool move_by_wind_;
	/// �������� �����
	bool move_by_water_;
	/// ��������� ���������� �����������
	bool mapUpdateDeactivate_;
	bool mapUpdateActivate_;
	bool breakWhenApply_;
	/// ���� ������� �� �������� �����
	TargetClass targetClass_;
	/// ����, ����������� � ����
	SoundReference soundReference_;
	SoundController sound_;

	Rangef windSensitivity_;
	
	Vect3f origin_;
	/// ����������
	Path path_;
	/// �������� �� � ����������
	bool followPath_;
	/// ��������� �� �������� �� ����
	bool enabled_;
	/// ����� ���������
	string label_;
	/// �������� ���
	string name_;

	int deadQuant_;
	/// �������� ��������� ���������
	DelayTimer activationTimer_;
	/// �������� �������� ���������
	DelayTimer killTimer_;
	/// ����� �����, msec
	int lifeTime_;

	/// �������� ����� � ����������, -1 ���� ��������
	int interfaceLightIndex_;

	/// ��� �����������, �� ������� ����� �������� ��������
	/// ����������� ���� ��� - ��� �������� ��������� � ����
	/// ���� ����������� ������������, �� �������� �� ��������
	SurfaceClass surfaceClass_;
	/// ����������, �� ��� �����������
	SurfaceKindBitVector surfaceKind_;

	PlacementMode placementMode_;


	ChildSources childSources_;

	void setScanEnvironment(bool needScan) { scanEnvironment_ = needScan; }
	virtual bool killRequest(){ return true; }
	virtual bool isUseKillTimer() const { return true; }
	const Vect3f* environmentPoints() const { return environmentPoints_; }

private:
	void enable();
	void incDeadCnt(void){ ++deadQuant_; }

	Player* worldPlayer() const;
	Player* player_;

	UnitLink<UnitBase> owner_;

	bool checkTarget(AffectMode mode, const UnitBase* target) const;

	/// �����, � ������� �������� �������� ����� ����� "���������"
	float  visibleTime_;
	/// ������ �� ���������� ���������
	DurationTimer showTimer_;

	/// ������ ��� ������������ ��������� � ������ ��������
	EffectAttribute waitingEffectAttribute_;
	EffectController waitingEffectController_;
	void waitingEffectStart();
	void waitingEffectStop();
	
	bool scanEnvironment_;

	Vect3f* environmentPoints_;
	Vect3f velocity_;
	void recalcEnvironmentPoints();
	void environmentAnalysis();

};

/// �������� ����������, ������� ��������� �������.
class SourceWeaponAttribute: public SourceAttribute
{
public:
	SourceWeaponAttribute();

	void serialize(Archive& ar);

	const Vect2f& positionDelta() const { return positionDelta_; }

	bool isAutonomous() const { return isAutonomous_; }

	const CircleEffect& showParam() const { return showColor_; }

	const string& key() const { return key_; }

private:
	/// ����� ���������
	Vect2f positionDelta_;
	/// ���� true, �� �������� �� ��������� ������� �����
	bool isAutonomous_;
	/// ���� ��������� ��������� �� ������
	CircleEffect showColor_;
};

typedef std::vector<SourceWeaponAttribute> SourceWeaponAttributes;

#endif //__BASE_SOURCE_H__
