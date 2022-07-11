#ifndef __SHOWCHANGE_CONTROLLER_H__
#define __SHOWCHANGE_CONTROLLER_H__

#include "Render\inc\RenderMT.h"
#include "Timers.h"
#include "XTL\SwapVector.h"
#include "XMath\Colors.h"

#include "UserInterface\UI_Font.h"

class UnitInterface;
class Archive;

struct ShowUpAttribute
{
	ShowUpAttribute();

	UI_FontReference font_;
	float scale_;
	Color4f color;
	time_type time;
	int height;
	float fadeSpeed;

	static float minDrawSize;
	static bool distanceFade;
	static void serializeStatic(Archive& ar);

	void serialize(Archive& ar);
};

struct ShowChangeSettings
{
	ShowChangeSettings();

	bool showFlyParameters;
	ShowUpAttribute showIncAttribute;
	ShowUpAttribute showDecAttribute;
	float spawnFreq;

	void serialize(Archive& ar);

	const static ShowChangeSettings EMPTY;
};


//! поднимающийся вверх текст
class ShowUpController
{
	const ShowUpAttribute* attr_;
	float timer_;
	string msg_;

public:
	ShowUpController(const char* str, const ShowUpAttribute* attr);
	bool enabled() const;
	bool draw(Vect3f position);
};

//! Отображение изменения значения у объекта
class ShowChangeController : public ShareHandleBase
{
	typedef SwapVector<ShowUpController> ShowUpControllers;
	ShowUpControllers controllers_;

	//UnitLink<const UnitInterface> owner_;
	const UnitInterface* owner_;
	Vect3f lastPosition_;
	int initHeight_;
	const ShowChangeSettings* attr_;
	
	float oldValue_;
	float curentValue_;
	LogicTimer timer_;

public:

	ShowChangeController(const UnitInterface* owner, const ShowChangeSettings* attr, float initVal, int initHeight = 0);
	~ShowChangeController() {}

	bool alive() const { MTG(); return owner_ || !controllers_.empty() || fabsf(curentValue_ - oldValue_) >= 1.f; }
	void setOwner(const UnitInterface* owner) { owner_ = owner; }

	float value() const { return curentValue_; }
	void update(float newVal) { curentValue_ = newVal; }

	bool quant();

private:

	bool isVisible() const;
};

typedef ShareHandle<ShowChangeController> SharedShowChangeController;

#endif //__SHOWCHANGE_CONTROLLER_H__
