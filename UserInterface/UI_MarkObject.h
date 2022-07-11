#ifndef __UI_MARK_OBJECT_H__
#define __UI_MARK_OBJECT_H__

#include "UI_Enums.h"
#include "UI_MarkObjectAttribute.h"
#include "BaseUniverseObject.h"
#include "UnitLink.h"
#include "EffectReference.h"
#include "EffectController.h"
#include "..\util\Timers.h"

class cObject3dx;

class UnitInterface;
typedef UnitLink<const UnitInterface> UnitInterfaceConstLink;

/// Данные для создания/обновления пометки.
class UI_MarkObjectInfo
{
public:
	UI_MarkObjectInfo(UI_MarkType type = UI_MARK_TYPE_DEFAULT, const Se3f pose = Se3f::ID,
		const UI_MarkObjectAttribute* attr = 0, const UnitInterface* owner = 0,	const UnitInterface* target = 0);

	bool isEmpty() const { return (!attribute_ || attribute_->isEmpty()); }

	UI_MarkType type() const { return type_; }
	const Se3f& pose() const { return pose_; }

	const UI_MarkObjectAttribute* attribute() const { return attribute_; }
	const UnitInterface* owner() const { return owner_; }
	const UnitInterface* target() const { return target_; }

private:

	UI_MarkType type_;
	Se3f pose_;

	const UI_MarkObjectAttribute* attribute_;
	
	UnitInterfaceConstLink owner_;
	UnitInterfaceConstLink target_;
};

/// Объект-пометка.
/**
Ставится на поверхность для визуализации области 
поражения оружия, места сборки юнитов с завода и т.п.
*/
class UI_MarkObject : public BaseUniverseObject
{
public:
	UI_MarkObject();
	UI_MarkObject(const UI_MarkObject& obj);
	~UI_MarkObject();

	UI_MarkObject& operator = (const  UI_MarkObject& obj);

	bool operator == (const UI_MarkObject& obj) const;
	bool operator == (const UI_MarkObjectInfo& inf) const;

	void setPose(const Se3f& pose, bool init);

	bool update(const UI_MarkObjectInfo& inf);
	void restartTimer(float time = 0.f);

	bool quant(float dt);

	void clear();
	bool isEmpty() const { return attribute_ ? !(attribute_->hasEffect() || attribute_->hasModel()) : true; }

	void showDebugInfo() const;
private:

	UI_MarkType type_;

	bool updated_;

	cObject3dx* model_;
	float animationPhase_;

	GraphEffectController effect_;

	float lifeTime_;

	/// параметры метки
	const UI_MarkObjectAttribute* attribute_;

	/// объект - владелец метки
	UnitInterfaceConstLink owner_;
	UnitInterfaceConstLink target_;

	void release();
};

#endif /* __UI_MARK_OBJECT_H__ */
