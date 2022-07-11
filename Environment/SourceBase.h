#ifndef __BASE_SOURCE_H__
#define __BASE_SOURCE_H__

#include "Serialization\SerializationTypes.h"
#include "Handle.h"
#include "Units\CircleManagerParam.h"
#include "Serialization\Range.h"
#include "Serialization\StringTableReferencePolymorphic.h"
#include "Units\BaseUniverseObject.h"
#include "XTL\SafeCast.h"
#include "Timers.h"
#include "EffectReference.h"
#include "Units\EffectController.h"
#include "Units\WeaponEnums.h"
#include "Grid2D.h"
#include "UnitLink.h"

class SourceBase;

class WeaponSourcePrm;
class WeaponTarget;

class UnitBase;

class Player;

typedef StringTable<StringTableBasePolymorphic<SourceBase> > SourcesLibrary;
typedef StringTableReferencePolymorphic<SourceBase, false> SourceReference;
typedef vector<SourceReference> SourceReferences;
enum SurfaceKind {
	/// Поверхность 1 рода
	SURFACE_KIND_1 = 1,
	/// Поверхность 2 рода
	SURFACE_KIND_2 = 1 << 1,
	/// Поверхность 3 рода
	SURFACE_KIND_3 = 1 << 2,
	/// Поверхность 4 рода
	SURFACE_KIND_4 = 1 << 3
};

enum SurfaceClass {
	/// только на земле
	SOURCE_SURFACE_GROUND	= 1 << 0,
	/// только на воде
	SOURCE_SURFACE_WATER	= 1 << 1,
	/// только на льду
	SOURCE_SURFACE_ICE		= 1 << 2,
	/// все возможные
	SOURCE_SURFACE_ANY	= SOURCE_SURFACE_GROUND | SOURCE_SURFACE_WATER | SOURCE_SURFACE_ICE,
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
	/// ключ для ссылки из типсов
	string key_;
private:
	/// задержка старта
	float activationDelay_;
	/// время жизни
	float lifeTime_;
	/// ссылка на источник из библиотеки
	SourceReference sourceReference_;
};

enum TerrainType;
class SourceBase : public BaseUniverseObject, public GridElementType, public PolymorphicBase
{
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

	enum {
		/// максимальное время жизни источника, выставляемое по умолчанию
		MAX_DEFAULT_LIFETIME = 10000
	};

	enum PlacementMode {
		// не трогать
		PLACE_NONE,
		// ставить на землю
		PLACE_TO_GROUND,
		// ставить на воду
		PLACE_TO_WATER
	};

	/// род и тип поверхности, где источник может работать
	//typedef BitVector<SurfaceKind> SurfaceKindBitVector;
	typedef BitVector<TerrainType> SurfaceKindBitVector;
	typedef BitVector<SurfaceClass> SurfaceClassBitVector;

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

	void setName(const char* name) { libraryKey_ = name; }
	const char* libraryKey() const { return libraryKey_.c_str(); }

	const char* label() const { return label_.c_str(); }

	const CircleManagerParam& radiusCircle() const { return radiusCircle_; }

	time_type getActivationTime() const { return activationTimer_.time(); }
	void setActivationTime(int time){ activationTimer_.start(time); }
	bool needActivation() const { return !active_ && isAlive() && activationTimer_.finished(); }

	bool isKillTimerStarted()const { return (killTimer_.started() || (!isUseKillTimer())); }
	void setKillTimer(int time){ lifeTime_ = time; killTimer_.start(time); }
	int lifeTime() const { return lifeTime_; }
	void setLifeTime(int time){ lifeTime_ = time; }
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

	int surfaceInstallClass() const { return surfaceInstallClass_; }
	int surfaceClass() const { return surfaceClass_; }
	int surfaceKind() const { return surfaceKind_; }

	void setAffectMode(AffectMode mode);

	void setDetected();
	bool detected() const { return showTimer_.busy(); }

	virtual bool getParameters(WeaponSourcePrm& prm) const { return false; }
	virtual bool setParameters(const WeaponSourcePrm& prm, const WeaponTarget* target = 0){ return false; }

	UniverseObjectClass objectClass() const{ return UNIVERSE_OBJECT_SOURCE; }

	void mapUpdate(float x0,float y0,float x1,float y1);

	const SourceAttribute* getChildSourceByKey(const char* key) const;

	/// информация о производимых источниках
	struct ChildSource{
		ChildSource() : generationDelay_(0.f) {}
		/// время между появлениями
		float generationDelay_;
		LogicTimer activationTimer_;
		/// ссылка на источник из библиотеки
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

	/// вызывать в конечный производных классах в конце сериализации
	void serializationApply(const Archive& ar);

	void soundInit();

	AffectMode affectMode_;
	AffectMode activatorMode_;
	bool active_;
	/// в зоне враг
	bool targetInZone;
	/// флаг включения при попадании юнита в зону
	bool waiting_target_;
	/// флаг включения при попадании зоны на нужную высоту
	bool waiting_height_;
	/// активироваться только детонатором
	bool activate_by_detonator_;
	/// активироваться при попадании снаряда
	bool activate_by_projectile_;
	/// При автоактивации убивать поставившего источник
	bool killOwner_;
	/// запущена активация
	bool activation_started_;
	/// Сносится ветром
	bool move_by_wind_;
	/// Сносится водой
	bool move_by_water_;
	/// Проверять измененния поверхности
	bool mapUpdateDeactivate_;
	bool mapUpdateActivate_;
	bool breakWhenApply_;
	/// флаг реакции на летающие юниты
	TargetClass targetClass_;
	/// звук, привязанный к зоне
	SoundReference soundReference_;
	SoundController sound_;

	Rangef windSensitivity_;
	
	Vect3f origin_;
	/// траектория
	Path path_;
	/// привязан ли к траектории
	bool followPath_;
	/// находится ли источник на мире
	bool enabled_;
	/// метка источника
	string label_;
	/// ключевое имя
	string libraryKey_;

	int deadQuant_;
	/// задержка включения источника
	LogicTimer activationTimer_;
	/// задержка удаления источника
	LogicTimer killTimer_;
	/// время жизни, msec
	int lifeTime_;

	/// источник света в интерфейсе, -1 если отключен
	int interfaceLightIndex_;

	/// высота выше которой активируется
	short activationHeight_;
	/// высота ниже которой активируется
	short activationBottomHeight_;

	/// тип поверхности, на которой может быть установлен источник
	SurfaceClassBitVector surfaceInstallClass_;
	/// тип поверхности, на которой может работать источник, при попадании/выходе - включается/выключается
	SurfaceClassBitVector surfaceClass_;
	/// аналогично, но род поверхности
	SurfaceKindBitVector surfaceKind_;

	PlacementMode placementMode_;

	CircleManagerParam radiusCircle_;

	ChildSources childSources_;

	void setScanEnvironment(bool needScan) { scanEnvironment_ = needScan; }
	virtual bool killRequest(){ return true; }
	virtual bool isUseKillTimer() const { return true; }
	const Vect3f* environmentPoints() const { return environmentPoints_; }

private:
	void enable();
	void incDeadCnt(){ ++deadQuant_; }

	Player* worldPlayer() const;
	Player* player_;

	UnitLink<UnitBase> owner_;

	bool checkTarget(AffectMode mode, const UnitBase* target) const;

	/// время, в течении которого источник виден после "подсветки"
	float  visibleTime_;
	/// таймер на пропадание источника
	LogicTimer showTimer_;

	/// эффект для визуализации источника в режиме ожидания
	EffectAttribute waitingEffectAttribute_;
	EffectController waitingEffectController_;
	void waitingEffectStart();
	void waitingEffectStop();
	
	bool scanEnvironment_;

	Vect3f* environmentPoints_;
	Vect3f velocity_;
	void recalcEnvironmentPoints();
	void environmentAnalysis();

	friend class SourceManager;
};

/// Атрибуты источников, которые создаются юнитами.
class SourceWeaponAttribute: public SourceAttribute
{
public:
	SourceWeaponAttribute();

	void serialize(Archive& ar);

	const Vect2f& positionDelta() const { return positionDelta_; }

	bool isAutonomous() const { return isAutonomous_; }

	const CircleManagerParam& showParam() const { return showColor_; }

	const string& key() const { return key_; }

private:
	/// сдвиг координат
	Vect2f positionDelta_;
	/// если true, то работает до окончания времени жизни
	bool isAutonomous_;
	/// цвет отрисовки источника из оружия
	CircleManagerParam showColor_;
};

typedef std::vector<SourceWeaponAttribute> SourceWeaponAttributes;

#endif //__BASE_SOURCE_H__

