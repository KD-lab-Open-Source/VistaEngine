#ifndef __UI_MARK_OBJECT_ATTRIBUTE_H__
#define __UI_MARK_OBJECT_ATTRIBUTE_H__

#include "EffectReference.h"

/// Параметры объекта-пометки.
class UI_MarkObjectAttribute
{
public:
	UI_MarkObjectAttribute();

	void serialize(Archive& ar);

	bool isEmpty() const { return (!hasModel() && !hasEffect()); }

	bool hasModel() const { return !modelName_.empty(); }
	const char* modelName() const { return modelName_.c_str(); }
	void setModelName(const char* name){ modelName_ = name; }
	float scale() const { return modelScale_; }

	bool hasAnimation() const { return !animationName_.value().empty(); }
	const char* animationName() const { return animationName_; }
	void setAnimationName(const char* name){ animationName_ = name; }

	float animationPeriod() const { return animationPeriod_; }
	void setAnimationPeriod(float period){ animationPeriod_ = period; }

	bool hasEffect() const { return !effect_.isEmpty(); }
	bool syncEffectWithModel()const {return synchronizeWithModel_;}
	const EffectAttribute& effect() const { return effect_; }

	float lifeTime() const { return lifeTime_; }

private:

	/// имя файла 3D модели
	std::string modelName_;
	/// масштаб модели
	float modelScale_;
	/// имя цепочки анимации
	ComboListString animationName_;
	/// период анимации в секундах
	float animationPeriod_;

	/// спецэффект
	EffectAttribute effect_;
	bool synchronizeWithModel_;

	/// время жизни пометки в секундах
	/// если нулевое, то живёт один кадр
	float lifeTime_;

	void setComboList(const char* model_name);
};

#endif // __UI_MARK_OBJECT_H__
