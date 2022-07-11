#ifndef __EFFECT_CONTROLLER_H__
#define __EFFECT_CONTROLLER_H__

#include "XTL\SafeCast.h"
#include "Interpolation.h"
#include "SoundAttribute.h"
#include "UnitLink.h"
#include "Timers.h"

class BaseUniverseObject;
class UnitBase;
class cEffect;
class SNDSound;

class EffectAttribute;
class EffectAttributeAttachable;

const float minDistance = 5.f;

/// Спецэффект, привязанный к базовому объекту
class EffectController
{
public:
	EffectController(const BaseUniverseObject* owner, const Vect2f delta = Vect2f::ZERO);
	virtual ~EffectController();

	enum PlacementMode {
		PLACEMENT_NONE,
		PLACEMENT_BY_ATTRIBUTE,
		PLACEMENT_WATER
	};

	bool operator == (const EffectAttribute* attr) const { return (effectAttribute_ == attr); }
	const EffectAttribute* attr() const { return effectAttribute_; }

	bool isSwitchedOff() const { return lastSwitchOffState_; }

	bool isEnabled() const { return (effectAttribute_ && effect_); }

	Vect3f position() const;

	void setOwner(const BaseUniverseObject* p){ owner_ = p; }
	
	const Vect2f& positionDelta() const { return positionDelta_; }
	void setPositionDelta(const Vect2f& delta){ positionDelta_ = delta; }
	
	void setPlacementMode(PlacementMode mode){ placementMode_ = mode; }

	bool effectStart(const EffectAttribute* attribute, float scale = 1.f, Color4c skin_color = Color4c(255, 255, 255, 255));

	void moveToTime(int time);

	int getTime() const;
	
	void release();

	virtual bool logicQuant(float radius = 0.0f);

	void showDebugInfo() const;

protected:
	virtual void effectStop();
	/// проверяет, надо ли выключать эффект
	/// \a radius - радиус, в котором выполняется проверка
	/// при выключении эффекта возвращает false и выставляет \a switchedOff_ в true
	bool checkEnvironment(float radius = 0.0f);

	const BaseUniverseObject* owner() const { return owner_; }
	
	virtual bool createEffect(EffectKey* key);
	/// начальная установка параметров эффекта, вызывается из \a createEffect()
	void initEffect(cEffect* eff);

	cEffect* effect_;	
	const EffectAttribute* effectAttribute_;

	/// меняется после непосредственного выключения эффекта
	bool lastSwitchOffState_;

private:
	/// владелец, к которому осуществляется привязка
	const BaseUniverseObject* owner_;

	Se3f effectPose_;

	/// смещение эффекта по отношению к владельцу
	Vect2f positionDelta_; // TODO: избавится, владельцы пусть хранят список BaseUniverseObject для привязки эффекта 

	/// Режим установки на поверхность - не ставить/как задано в атрибуте/ставить на воду.
	PlacementMode placementMode_;
};

//-------------------------------------------------------------------

class UnitEffectController : public EffectController
{
public:
	UnitEffectController(const BaseUniverseObject* owner) 
		: EffectController(owner)
		, paused_(false)
		, effectNode_(-1) {}

	const EffectAttributeAttachable* attr() const { return reinterpret_cast<const EffectAttributeAttachable*>(effectAttribute_); }

	/// установка узла привязки эффекта, надо вызывать до старта
	void setEffectNode(int node){ effectNode_ = node; }
		
	/// управлением включением/выключением в соответствии с окружающей средой
	bool logicQuant(float radius = 0.0f);

	bool killTimer() const { return killTimer_.finished(); }
	void setKillTimer(int time){ killTimer_.start(time); }

	void setPause(bool flag) { paused_ = flag; }
	bool pause() const { return paused_; }

private:
	const UnitBase* owner() const;
	bool createEffect(EffectKey* key);

	/// место привязки эффекта
	int effectNode_;
	LogicTimer killTimer_;
	bool paused_;
};

//-------------------------------------------------------------------

class GraphEffectController : public EffectController
{
public:
	GraphEffectController(const BaseUniverseObject* owner) :  EffectController(owner) {}

	// управлением включением/выключением в соответствии с окружающей средой
	bool logicQuant(float radius = 0.0f);
	
	/// Напрямую обновляет позицию эффекта.
	/// Вызывать только из графического потока, объект-владелец должен быть графическим.
	void updatePosition();

protected:
	void effectStop();
};

struct SoundControllerToken
{
	SoundControllerToken(const SoundAttribute* sound = 0, const BaseUniverseObject* owner = 0) : owner_(owner)
	{
		soundAttribute_ = static_cast<const Sound3DAttribute*>(sound);
	}
	const Sound3DAttribute* soundAttribute_;
	UnitLink<const BaseUniverseObject> owner_;
};

/// Управление трёхмерным звуком.
class SoundController
{
public:
	SoundController();
	~SoundController();

	void release();

	bool operator == (const SoundControllerToken& token) const { return (owner_ == token.owner_ && soundAttribute_ == token.soundAttribute_); }

	bool init(const SoundAttribute* Sound, const BaseUniverseObject* owner = NULL);
	bool isInited() const { return sound_ && soundAttribute_; }

	/// задать координаты звука, вызывать перед стартом
	void setPosition(const Vect3f& pos);
	/// получить координаты звука
	Vect3f position() const { return pose_; }

	/// установка громкости, параметр [0, 1]
	bool setVolume(float volume);

	/// задать владельца
	void setOwner(BaseUniverseObject* owner) {owner_ = owner; }
	const BaseUniverseObject* owner() const {return owner_; }

	/// запустить(перезапустить) звук на проигрывание
	void start();
	
	/// остановить проигрывание
	void stop(bool immediately = false);

	/// проверить играет ли еще звук
	bool isPlaying() const;

	/// проапдейтить координаты звука и при необходимости запустить/остановить
	bool quant();

private:
	Vect3f pose_;
	SNDSound* sound_;
	const Sound3DAttribute* soundAttribute_;
	UnitLink<const BaseUniverseObject> owner_;
};

#endif /* __EFFECT_CONTROLLER_H__ */
