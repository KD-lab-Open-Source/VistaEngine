#ifndef __UI_MARK_OBJECT_H__
#define __UI_MARK_OBJECT_H__

#include "UI_Enums.h"
#include "UI_MarkObjectAttribute.h"
#include "BaseUniverseObject.h"
#include "UnitLink.h"
#include "EffectReference.h"
#include "EffectController.h"
#include "Timers.h"

class cObject3dx;

class UnitInterface;
typedef UnitLink<const UnitInterface> UnitInterfaceConstLink;
typedef UnitLink<const BaseUniverseObject> BaseUniverseObjectConstLink;

/// Данные для создания/обновления пометки.
class UI_MarkObjectInfo
{
public:
	UI_MarkObjectInfo(UI_MarkType type = UI_MARK_TYPE_DEFAULT, const Se3f pose = Se3f::ID,
		const UI_MarkObjectAttribute* attr = 0, const BaseUniverseObject* owner = 0, const UnitInterface* target = 0);

	bool isEmpty() const { return (!attribute_ || attribute_->isEmpty()); }

	UI_MarkType type() const { return type_; }
	const Se3f& pose() const { return pose_; }

	const UI_MarkObjectAttribute* attribute() const { return attribute_; }
	const BaseUniverseObject* owner() const { return owner_; }
	const UnitInterface* target() const { return target_; }

private:

	UI_MarkType type_;
	Se3f pose_;

	const UI_MarkObjectAttribute* attribute_;
	
	BaseUniverseObjectConstLink owner_;
	UnitInterfaceConstLink target_;
};

/// Объект-пометка.
/**
Ставится на поверхность для визуализации области 
поражения оружия, места сборки юнитов с завода и т.п.
*/
class UI_MarkObject : public BaseUniverseObject
{
	friend class UI_LinkToMarkObject;
public:
	UI_MarkObject();
	UI_MarkObject(const UI_MarkObject& obj);
	~UI_MarkObject();

	UI_MarkObject& operator = (const  UI_MarkObject& obj);

	bool operator == (const UI_MarkObject& obj) const {
		return type_ == obj.type_ && owner_ == obj.owner_ && attribute_ == obj.attribute_;
	}
	bool operator == (const UI_MarkObjectInfo& inf) const {
		return type_ == inf.type() && attribute_ == inf.attribute() && pose().trans().eq(inf.pose().trans(), 4.f);
	}

	void setPose(const Se3f& pose, bool init);

	bool update(const UI_MarkObjectInfo& inf);

	bool quant(float dt);

	void clear();
	bool isEmpty() const { return attribute_ ? !(attribute_->hasEffect() || attribute_->hasModel()) : true; }

	void showDebugInfo() const;
private:

	bool calcTargetMarkPosition(Vect3f& pos);

	UI_MarkType type_;

	unsigned long updated_;

	cObject3dx* model_;
	float animationPhase_;

	GraphEffectController effect_;

	float lifeTime_;

	/// параметры метки
	const UI_MarkObjectAttribute* attribute_;

	/// объект - владелец метки
	BaseUniverseObjectConstLink owner_;
	UnitInterfaceConstLink target_;

	void release();
};

//-------------------------------------------------------------------

class UI_LinkToMarkObject : public GraphEffectController
{
public:
	UI_LinkToMarkObject(const UI_MarkObject* owner = 0) : GraphEffectController(owner) {}

	void updateLink(const cObject3dx* obj, int nodeIndex);

private:
	const UI_MarkObject* owner() const { return safe_cast<const UI_MarkObject*>(EffectController::owner()); }

};


#endif /* __UI_MARK_OBJECT_H__ */
