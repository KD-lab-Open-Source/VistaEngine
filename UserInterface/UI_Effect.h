#ifndef __UI_EFFECT_
#define __UI_EFFECT_
#include "Util\EffectContainer.h"
#include "XTL\Handle.h"

class cEffect;
class cObject3dx;
class UI_ControlBase;
class Camera;

/// параметры спецэффекта
class UI_EffectAttribute : public PolymorphicBase
{
public:
	UI_EffectAttribute();
	bool eq(const UI_EffectAttribute& rsh) const {
		return effectReference_ == rsh.effectReference_ && fabsf(scale_ - rsh.scale_) < 0.1f && legionColor_ == rsh.legionColor_
			&& stopImmediately_ == rsh.stopImmediately_ && isCycled_ == rsh.isCycled_; }

	bool isEmpty() const { return effectReference_.get() == 0; }

	bool legionColor() const { return legionColor_; }

	void serialize(Archive& ar);

	bool isCycled() const { return isCycled_; }
	bool stopImmediately() const { return stopImmediately_; }

	float scale() const { return scale_; }

	const EffectReference& effectReference() const { return effectReference_; }
	EffectKey* effect(float scale = -1.f, Color4c skin_color = Color4c(255,255,255,255)) const { return isEmpty() ? 0 : effectReference_->getEffect(scale > 0.f ? scale : scale_, skin_color); }

protected:

	/// зацикливать эффект или нет
	bool isCycled_;
	/// обрывать
	bool stopImmediately_;

	/// окрашивать в цвет легиона
	bool legionColor_;

	/// масштаб эффекта
	float scale_;

	/// ссылка на эффект из библиотеки эффектов
	EffectReference effectReference_;
};

// --------------------------------------------------------------------------

class UI_EffectAttributeAttachable : public UI_EffectAttribute
{
public:
	void serialize(Archive& ar);

	const char* node() const { return nodeLink_.c_str(); }
private:
	string nodeLink_;
};

// --------------------------------------------------------------------------

class UI_EffectController
{
public:
	UI_EffectController();

	bool operator == (const UI_EffectAttribute* attr) const { return (effectAttribute_ == attr); }
	const UI_EffectAttribute* attr() const { return effectAttribute_; }

	bool isEnabled() const { return effectAttribute_ && effect_; }
	
	bool effectStart(const UI_EffectAttribute* attribute, const Vect3f& pose);
	void effectStop(bool immediately = false);

	void drawDebugInfo(Camera* camera) const;

protected:
	void initEffect(cEffect* eff);

	cEffect* effect_;	
	const UI_EffectAttribute* effectAttribute_;
};

// --------------------------------------------------------------------------

class UI_EffectController3D : public UI_EffectController
{
public:
	UI_EffectController3D() : position_(Vect3f::ZERO) {}

	bool effectStart(const UI_EffectAttribute* attribute, const Vect3f& pose);

	void setPosition(const Vect3f& pos);
	const Vect3f& position() const { return position_; }

private:
	Vect3f position_;
};

// --------------------------------------------------------------------------

class UI_EffectControllerAttachable : public UI_EffectController
{
public:
	const Vect3f& position() const;
};

// --------------------------------------------------------------------------

class UI_EffectControllerAttachable3D : public UI_EffectControllerAttachable
{
public:
	const UI_EffectAttributeAttachable* attr() const { return static_cast<const UI_EffectAttributeAttachable*>(effectAttribute_); }

	bool effectStart(const UI_EffectAttributeAttachable* attribute, cObject3dx* owner);
};

// --------------------------------------------------------------------------

class UI_EffectControllerAttachable2D : public UI_EffectControllerAttachable
{
public:
	UI_EffectControllerAttachable2D() : owner_(0) {}
	bool operator == (const UI_ControlBase* ownr) const { return owner_ == ownr; }

	bool effectStart(const UI_EffectAttribute* attribute, const UI_ControlBase* owner);
	void updatePosition();

	void getDebugInfo(Camera* camera, XBuffer& buf) const;

private:
	Vect3f calcPos(const UI_ControlBase* ctrl, Camera* camera);
	const UI_ControlBase* owner_;
};

#endif//__UI_EFFECT_