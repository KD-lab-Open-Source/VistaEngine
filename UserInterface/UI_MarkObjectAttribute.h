#ifndef __UI_MARK_OBJECT_ATTRIBUTE_H__
#define __UI_MARK_OBJECT_ATTRIBUTE_H__

#include "EffectReference.h"
#include "Units\Object3dxInterface.h"
#include "UI_Types.h"

/// Параметры объекта-пометки.
class UI_MarkObjectAttribute
{
public:
	UI_MarkObjectAttribute();

	void serialize(Archive& ar);

	bool isEmpty() const { return (!hasModel() && !hasEffect()); }

	bool hasModel() const { return !modelName_.empty(); }
	const char* modelName() const { return modelName_.c_str(); }
	int linkNodeIndex() const { return linkNode_; }
	void setModelName(const char* name){ modelName_ = name; }
	float scale() const { return modelScale_; }

	bool hasAnimation() const { return !animationName_.value().empty(); }
	const char* animationName() const { return animationName_; }
	void setAnimationName(const char* name){ animationName_ = name; }

	bool animateByAttack() const { return animateByAttack_; }
	bool finishAnimation() const { return finishAnimation_; }

	float animationPeriod() const { return animationPeriod_; }
	void setAnimationPeriod(float period){ animationPeriod_ = period; }

	bool hasEffect() const { return !effect_.isEmpty(); }
	bool syncEffectWithModel()const {return synchronizeWithModel_;}
	const EffectAttribute& effect() const { return effect_; }
	const UI_Cursor& cursor() const { return cursorProxy_; }

	bool rotateWithCamera() const { return rotateWithCamera_; }

	float lifeTime() const { return lifeTime_; }

private:

	/// имя файла 3D модели
	std::string modelName_;
	/// место привязки связи
	GenericObject3dxNode linkNode_;
	/// масштаб модели
	float modelScale_;
	/// имя цепочки анимации
	ComboListString animationName_;
	/// период анимации в секундах
	float animationPeriod_;

	/// спецэффект
	EffectAttribute effect_;
	bool synchronizeWithModel_;

	/// имя файла с курсором
	string cursorfileName_;
	UI_Cursor cursorProxy_;

	/// поворачивать вместе с камерой
	bool rotateWithCamera_;

	/// анимировать модель только при нажатой кнопке атаки
	bool animateByAttack_;

	/// доигрывать анимацию до конца перед исчезновением
	bool finishAnimation_;

	/// время жизни пометки в секундах
	/// если нулевое, то живёт один кадр
	float lifeTime_;

	void setComboList(const char* model_name);
};

#endif // __UI_MARK_OBJECT_H__
