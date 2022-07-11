#include "StdAfx.h"
#include "ShowChangeController.h"

#include "UnitInterface.h"
#include "CameraManager.h"
#include "Render\Src\cCamera.h"
#include "Serialization\Serialization.h"
#include "Serialization\RangedWrapper.h"
#include "RenderObjects.h"
#include "Render\src\Scene.h"

#include "Environment\Environment.h"

#include "UserInterface\UI_Render.h"
#include "UserInterface\UserInterface.h"
#include "UserInterface\UI_Logic.h"

float ShowUpAttribute::minDrawSize = 10.f;
bool ShowUpAttribute::distanceFade = true;

ShowUpAttribute::ShowUpAttribute() :
	color(1.f, 1.f, 1.f)
{
	time = 1000;
	height = 25;
	fadeSpeed = 2.f;
	scale_ = 1.f;
}

ShowChangeSettings::ShowChangeSettings()
{
	showFlyParameters = false;
	spawnFreq = 1.f;
}

void ShowUpAttribute::serializeStatic(Archive& ar)
{
	ar.serialize(RangedWrapperf(minDrawSize, 8, 20), "minDrawSize", "Минимальный размер шрифта");
	ar.serialize(distanceFade, "distanceFade", "Исчезать при удалении");
}

void ShowUpAttribute::serialize(Archive& ar)
{
	ar.serialize(font_, "font", "Шрифт");
	ar.serialize(scale_, "scale", "Масштаб");
	ar.serialize(color, "color", "цвет текста");
	float seconds = 0.001f * time;
	if(!ar.inPlace())
		ar.serialize(RangedWrapperf(seconds, 0.2f, 10.f), "time", "время показа");
	time = seconds * 1000;
	ar.serialize(height, "height", "высота поднятия");
	ar.serialize(fadeSpeed, "fadeSpeed", "скорость затухания");
	if(fadeSpeed < 1.f)
		fadeSpeed = 1.f;
}

void ShowChangeSettings::serialize(Archive& ar)
{
	ar.serialize(showFlyParameters, "showFlyParameters", "&Показывать взлетающий текст");
	if(showFlyParameters){
		ar.serialize(showIncAttribute, "showIncAttribute", "Увеличение значения");
		ar.serialize(showDecAttribute, "showDecAttribute", "Уменьшение значения");
		ar.serialize(spawnFreq, "spawnMinFreq", "Минимальльная периодичность появления");
	}
}

const ShowChangeSettings ShowChangeSettings::EMPTY;

ShowUpController::ShowUpController(const char* str, const ShowUpAttribute* attr)
{
	msg_ = str;
	attr_ = attr;
	timer_ = 0.f;
}

inline bool ShowUpController::enabled() const
{
	return attr_->color.a > FLT_EPS && UI_Dispatcher::instance().isEnabled() && UI_LogicDispatcher::instance().tipsEnabled();
}

bool ShowUpController::draw(Vect3f position)
{
	MTG();

	if(!enabled())
		return false;

	if(timer_ >= 0)
		timer_ += terScene->GetDeltaTime();
	else
		return false;

	if(timer_ <= attr_->time){
		float ph = timer_ / attr_->time;

		Vect3f e, w;
		position.z += attr_->height * ph;
		cameraManager->GetCamera()->ConvertorWorldToViewPort(&position, &w, &e);
		if(w.z > 0){
			Color4f color(attr_->color);
			color.a *= ph > (1.f - 1.f/attr_->fadeSpeed) ? attr_->fadeSpeed * (1.f - ph) : 1.f;
			float scale = attr_->scale_ * clamp(0.4f * cameraManager->GetCamera()->GetFocusViewPort().x / w.z, .0f, 4.f);

			FT::Font* font = (attr_->font_.get() ? attr_->font_->font() : 0);
			if(!font)
				font = UI_Render::instance().defaultFont()->font();
				
			float h = float(font->size()) * scale;
			if(h < ShowUpAttribute::minDrawSize){
				scale = ShowUpAttribute::minDrawSize / float(font->size());
				if(ShowUpAttribute::distanceFade){
					const float vMinSize = 0.6f * ShowUpAttribute::minDrawSize;
					color.a *= h >= vMinSize ? (h - vMinSize) / vMinSize : 0.f;
				}
			}

			if(color.a > 0.01f){
				gb_RenderDevice->SetFont(font);
				gb_RenderDevice->OutText(e.xi(), e.yi(), msg_.c_str(), color, ALIGN_TEXT_CENTER, ALPHA_BLEND, Vect2f(scale, scale));
				gb_RenderDevice->SetFont(0);
			}
		}
		return true;
	}
	else
		timer_ = -1.f;

	return false;
}

ShowChangeController::ShowChangeController(const UnitInterface* owner, const ShowChangeSettings* attr, float initVal, int initHeight)
{
	xassert(owner && attr);
	owner_ = owner;
	lastPosition_ = owner->position();
	initHeight_ = initHeight;
	lastPosition_.z += initHeight_;
	attr_ = attr ? attr : &ShowChangeSettings::EMPTY;
	curentValue_ = oldValue_ = initVal;
}

bool ShowChangeController::quant()
{
	MTG();
	
	if(!attr_->showFlyParameters)
		return false;

	if(const UnitInterface* owner = owner_){
		lastPosition_ = owner->interpolatedPose().trans();
		lastPosition_.z += initHeight_;

		if(!owner->alive())
			owner_ = 0;
	}

	if(!timer_.busy()){
		float diff = curentValue_ - oldValue_;
		if(fabsf(diff) >= 1.f){
			oldValue_ += diff;

			char buf[64];
			sprintf(buf, "%+d", round(diff));

			if(isVisible())
				controllers_.push_back(ShowUpController(buf, diff > 0 ? &attr_->showIncAttribute :  &attr_->showDecAttribute));
			
			timer_.start(attr_->spawnFreq * 1000);
		}
	}

	ShowUpControllers::iterator it = controllers_.begin();
	while(it != controllers_.end())
		if(it->draw(lastPosition_))
			++it;
		else
			it = controllers_.erase(it);

	return !controllers_.empty();
}

bool ShowChangeController::isVisible() const
{
	if(const UnitInterface* owner = owner_)
		return owner->isVisibleUnderForOfWar();
	else
		return environment->isVisibleUnderForOfWar(Vect2i(lastPosition_));

	return true;
}
