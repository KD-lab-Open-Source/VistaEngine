#include "stdafx.h"
#include "ActionsEnvironmental.h"

#include "..\Render\src\postEffects.h"
#include "Environment.h"
#include "Anchor.h"

#include "..\Water\Fallout.h"
#include "RenderObjects.h"
#include "..\Water\SkyObject.h"
#include "..\Water\CoastSprites.h"
#include "vMap.h"
#include "GameOptions.h"
#include "..\Game\SoundApp.h"
#include "..\Render\src\Gradients.h"

STARFORCE_API void initActionsEnvironmental()
{
SECUROM_MARKER_HIGH_SECURITY_ON(12);

REGISTER_CLASS(Action, ActionActivateSources, "Глобальные действия\\Активировать источники")
REGISTER_CLASS(Action, ActionDeactivateSources, "Глобальные действия\\Деактивировать источники")
REGISTER_CLASS(Action, ActionActivateMinimapMark, "Глобальные действия\\Активировать пометку на миникарте")
REGISTER_CLASS(Action, ActionDeactivateMinimapMarks, "Глобальные действия\\Деактивировать пометку на миникарте")
REGISTER_CLASS(Action, ActionSetFogOfWar, "Глобальные действия\\Туман войны(обязательно восстанавливать)")
REGISTER_CLASS(Action, ActionSetSilhouette, "Глобальные действия\\Силуэты(не использовать!!!)") 

REGISTER_CLASS(Action, ActionSetFallout, "Погода\\Установить параметры осадков")
REGISTER_CLASS(Action, ActionSetFalloutType, "Погода\\Параметры осадков (Только визуальные)")
REGISTER_CLASS(Action, ActionSetFalloutFlood, "Погода\\Параметры осадков (Заполнение водой)")


REGISTER_CLASS(Action, ActionSetWaterOpacity, "Погода\\Установить прозрачность воды")
REGISTER_CLASS(Action, ActionSetWind, "Погода\\Установить направление ветра")
REGISTER_CLASS(Action, ActionSetFog, "Погода\\Установить туман")
REGISTER_CLASS(Action, ActionSetEnvironmentTime, "Погода\\Установить время суток")
REGISTER_CLASS(Action, ActionSetWaterColor, "Погода\\Установить цвет воды")
REGISTER_CLASS(Action, ActionSetReflectSkyColor, "Погода\\Установить цвет отражённого неба")
REGISTER_CLASS(Action, ActionSetTimeScale, "Погода\\Скорость течения времени суток на мире")
REGISTER_CLASS(Action, ActionSetCoastSprites, "Погода\\Параметры прибрежных спрайтов");
REGISTER_CLASS(Action, ActionSetWaterLevel, "Погода\\Установить уровень воды")

REGISTER_CLASS(Action, ActionSetEffect, "Глобальные действия\\Включить/выключить эффект");

SECUROM_MARKER_HIGH_SECURITY_OFF(12);
}

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ActionSetFallout, TypeFallout, "TypeFallout")
REGISTER_ENUM_ENCLOSED(ActionSetFallout, NONE, "Нет осадков")
REGISTER_ENUM_ENCLOSED(ActionSetFallout, RAIN, "Дождь")
REGISTER_ENUM_ENCLOSED(ActionSetFallout, SNOW, "Снег")
END_ENUM_DESCRIPTOR_ENCLOSED(ActionSetFallout, TypeFallout)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ActionSetEffect, Effects, "Effects")
REGISTER_ENUM_ENCLOSED(ActionSetEffect, EFFECT_MONOCHROME, "Монохромный")
REGISTER_ENUM_ENCLOSED(ActionSetEffect, EFFECT_BLOOM, "Свечение")
REGISTER_ENUM_ENCLOSED(ActionSetEffect, EFFECT_UNDER_WATER, "Подводный")
REGISTER_ENUM_ENCLOSED(ActionSetEffect, EFFECT_DOF, "Глубина резкости(Floating Z-Buffer)")
REGISTER_ENUM_ENCLOSED(ActionSetEffect, EFFECT_COLORDODGE, "Наблюдатель")
END_ENUM_DESCRIPTOR_ENCLOSED(ActionSetEffect, Effects)

string ActionActivateSources::comboList="";
ComboListStringStatic::ComboListStringStatic() : ComboListString(ActionActivateSources::comboList.c_str()) {}
ComboListStringStatic::ComboListStringStatic(const char* nameIn) : ComboListString(ActionActivateSources::comboList.c_str(), nameIn) {}

ActionSetTimeScale::ActionSetTimeScale()
{
	dayTimeScale_ = 500.f;
	nightTimeScale_ = 1000.f;
}

void ActionSetTimeScale::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.openBlock("Time scale", "Масштаб времени суток");
		ar.serialize(dayTimeScale_, "dayTimeScale", "Масштаб времени днем");
		ar.serialize(nightTimeScale_, "nightTimeScale", "Масштаб времени ночью");
	ar.closeBlock();
}

void ActionSetTimeScale::activate()
{
	if(environment)
		environment->setTimeScale(dayTimeScale_, nightTimeScale_);
}

void ActionSetWaterLevel::activate()
{ 
	(*environment).water()->SetEnvironmentWater(waterHeight); 
	(*environment).water()->SetDampfK(water_dampf_k); 
}


bool ComboListStringStatic::serialize(Archive& ar, const char* name, const char* nameAlt)
{
    return ar.serialize(static_cast<ComboListString&>(*this), name, nameAlt);
}


ActionSetWind::ActionSetWind()
{
	angleDirection = 0;
	windPower = 1;
	windType = WindMap::DIRECTION_TO_CENTER_MAP;
}

void ActionSetWind::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(windType, "windType", "Тип ветра");
	ar.serialize(angleDirection, "angleDirection", "Угол направления ветра");
	ar.serialize(windPower, "windPower", "Сила ветра");
}

void ActionSetWind::activate()
{	float angle = M_PI * (1.5f + angleDirection / 180.0f);
	Vect2f dir = Vect2f(cos(angle) * windPower, sin(angle) * windPower);
	windMap->setDirection(dir);
	windMap->setWindType(windType);
	windMap->updateRect(0, 0, vMap.H_SIZE - 1, vMap.V_SIZE - 1);
}

ActionSetEffect::ActionSetEffect()
{
	switchMode = ON;
	effects = EFFECT_BLOOM;
}

void ActionSetEffect::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(switchMode, "switchMode", "Действие");
	ar.serialize(effects, "effects", "Эффект");
}

bool ActionSetEffect::automaticCondition() const
{
	if(!__super::automaticCondition())
		return false;

	bool flag = switchMode == ON ? true : false;
	switch(effects) {
		case EFFECT_BLOOM:
			if(flag)
				return !environment->PEManager()->IsActive(PE_BLOOM);
			else 
				return environment->PEManager()->IsActive(PE_BLOOM);
			
		case EFFECT_MONOCHROME:
			if(flag)
				return !environment->PEManager()->IsActive(PE_MONOCHROME);
			else 
				return environment->PEManager()->IsActive(PE_MONOCHROME);
			
		case EFFECT_UNDER_WATER:
			if(flag)
				return !environment->PEManager()->IsActive(PE_UNDER_WATER);
			else 
				return environment->PEManager()->IsActive(PE_UNDER_WATER);

		case EFFECT_DOF:
			if(flag)
				return !gb_VisGeneric->GetEnableDOF();
			else
				return gb_VisGeneric->GetEnableDOF();
		case EFFECT_COLORDODGE:
			if(flag)
				return !environment->PEManager()->IsActive(PE_COLOR_DODGE);
			else 
				return environment->PEManager()->IsActive(PE_COLOR_DODGE);

	}
	return false;
}

void ActionSetEffect::activate()
{
	bool flag = switchMode == ON ? true : false;
	switch(effects) {
		case EFFECT_BLOOM:
			environment->PEManager()->SetActive(PE_BLOOM,flag);
			break;
		case EFFECT_MONOCHROME:
			environment->PEManager()->SetActive(PE_MONOCHROME,flag);
			break;
		case EFFECT_UNDER_WATER:
			environment->PEManager()->SetActive(PE_UNDER_WATER,flag);
			break;
		case EFFECT_DOF:
			gb_VisGeneric->SetEnableDOF(flag);
			break;
		case EFFECT_COLORDODGE:
			environment->PEManager()->SetActive(PE_COLOR_DODGE,flag);
	}
}

ActionActivateSources::ActionActivateSources() 
{
	active_ = true;
}

void ActionActivateSources::activate() 
{
	activateSources(sources_, SOURCE_MAX);
}

void ActionActivateSources::serialize(Archive& ar) 
{
	__super::serialize(ar);
	if(ar.isOutput() && ar.isEdit() && environment)
		comboList = environment->sourceNamesComboList(SOURCE_MAX);

	ar.serialize(sources_, "sources_", "sources_");
}	

void ActionActivateSources::activateSources(Sources& sources_, SourceType type) 
{
	Sources::iterator i;
	FOR_EACH(sources_, i){
		SourceBase* source = environment->getSource(type, *i);
		if(!source){
			xassert_s(0 && "Источник по метке не найден: ", *i);
		}
		else{
			source->setActivity(active_);
		}
	}
}

ActionDeactivateSources::ActionDeactivateSources()
{
	active_ = false;
}

/////////////////////////////////////////////////////
void ActionActivateMinimapMark::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(anchorName_, "anchorName", "Имя пометки");
}

void ActionActivateMinimapMark::activate()
{
	if(Anchor* anchor = environment->findAnchor(anchorName_.c_str()))
		anchor->setSelected(active_);
}
/////////////////////////////////////////////////////

ActionSetFallout::ActionSetFallout()
{
	type = NONE;
	intencity = 0.5;
	time = 5;
	rainConstant = 0;
}

void ActionSetFallout::activate() 
{
	cFallout& fallout = *environment->fallout();
	cWater& water = *environment->water();
	switch(type){
	case NONE:
		fallout.Set(FALLOUT_CURRENT,0, time);
		//fallout.End(time);
		water.SetRainConstant(-rainConstant*rain_multiplicator);
		break;
	case RAIN:
		fallout.Set(FALLOUT_RAIN,intencity / 100.f, time);
		water.SetRainConstant(-rainConstant*rain_multiplicator);
		break;
	case SNOW:
		fallout.Set(FALLOUT_SNOW,intencity / 100.f, time);
		break;
	}
}

void ActionSetFallout::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(type, "type", "Тип осадков");
	ar.serialize(RangedWrapperf(intencity, 0, 100, 0.01f), "intensity", "Интенсивность (0-100)");
	ar.serialize(time, "time", "Время перехода");
	ar.serialize(rainConstant, "rainConstant", "ВОДА: Параметр высыхания");
}

ActionSetFalloutType::ActionSetFalloutType()
{
	type = ActionSetFallout::NONE;
	intencity = 50;
	time = 5;
}

void ActionSetFalloutType::activate() 
{
	cFallout& fallout = *environment->fallout();
	switch(type){
	case ActionSetFallout::NONE:
		fallout.Set(FALLOUT_CURRENT,0, time);
		break;
	case ActionSetFallout::RAIN:
		fallout.Set(FALLOUT_RAIN,intencity / 100.f, time);
		break;
	case ActionSetFallout::SNOW:
		fallout.Set(FALLOUT_SNOW,intencity / 100.f, time);
		break;
	}
}

void ActionSetFalloutType::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(type, "type", "Тип осадков");
	ar.serialize(RangedWrapperf(intencity, 0, 100, 0.01f), "intensity", "Интенсивность (0-100)");
	ar.serialize(time, "time", "Время перехода");
}

ActionSetFalloutFlood::ActionSetFalloutFlood()
{
	rainConstant = 0;
}

void ActionSetFalloutFlood::activate() 
{
	cWater& water = *environment->water();
	water.SetRainConstant(-rainConstant*rain_multiplicator);
}

void ActionSetFalloutFlood::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(rainConstant, "rainConstant", "ВОДА: Параметр высыхания");
}
/////////////////////////////////////////////////////

ActionSetEnvironmentTime::ActionSetEnvironmentTime()
{
	time = 12;
}

void ActionSetEnvironmentTime::activate() 
{
	environment->environmentTime()->SetTime(time);
}

void ActionSetEnvironmentTime::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(time, "time", "Время суток");
}

////////////////////////////////////////////////////

ActionSetFog::ActionSetFog()
{
	fog_enable = false;
	fog_start = 300;
	fog_end = 3000;
}

void ActionSetFog::activate()
{
	environment->setFogEnabled(fog_enable);
	environment->setFogStart(fog_start);
	environment->setFogEnd(fog_end);
}

void ActionSetFog::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(fog_enable, "fog_enable", "Включить туман");
	ar.serialize(fog_start, "fog_start", "Ближняя граница тумана");
	ar.serialize(fog_end, "fog_end", "Дальняя граница тумана");
}

////////////////////////////////////////////////////
ActionSetFogOfWar::ActionSetFogOfWar()
{
	mode_ = MODE_OFF;
}

void ActionSetFogOfWar::activate()
{
	switch(mode_){
		case MODE_OFF:
			environment->setShowFogOfWar(false);
			break;
		case MODE_ON:
			environment->setShowFogOfWar(true);	
			break;
		case MODE_RESTORE:
			environment->setShowFogOfWar(!debugDisableFogOfWar);	
	}
}

ActionSetWaterOpacity::ActionSetWaterOpacity()
{
	if(environment){
		cWater* water = environment->water(); 
		if(water){
			opacityGradient_ = water->GetOpacity();
			water->ClampOpacity(opacityGradient_);
		}
	}
}

ActionSetCoastSprites::ActionSetCoastSprites()
{
	if(environment)
		coastSprites_ = environment->coastSpritesAttributtes();
}

void ActionSetCoastSprites::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(coastSprites_, "coastSprites", "Прибрежные спрайты");
}

void ActionSetCoastSprites::activate()
{
	if(environment){
		environment->setCoastSpritesAttributtes(coastSprites_);
		cCoastSprites* pCoastSprites = environment->GetCoastSprites();
		if(pCoastSprites)
			pCoastSprites->Init(coastSprites_);
	}
}

void ActionSetWaterOpacity::serialize(Archive& ar)
{
	__super::serialize(ar);
	if(environment){
		cWater* water = environment->water(); 
		if(water)
			water->ClampOpacity(opacityGradient_);
	}
	ar.serialize(static_cast<WaterGradient&>(opacityGradient_), "zLevelOpacityGradient", "Прозрачность воды на разной глубине");
}

void ActionSetWaterOpacity::activate()
{
	if(environment){
		cWater* water = environment->water(); 
		if(water)
			water->SetOpacity(opacityGradient_);
	}
}

ActionSetReflectSkyColor::ActionSetReflectSkyColor()
{
	reflectSkyColor_.set(255,255,255,0);
	switchMode_ = ON;
}

void ActionSetReflectSkyColor::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(reflectSkyColor_, "reflectSkyColor", "Цвет");
	ar.serialize(switchMode_, "switchMode", "Действие");
}

void ActionSetReflectSkyColor::activate()
{
	if(environment){
		cEnvironmentTime* envTime = environment->environmentTime();
		if(envTime){
			envTime->setEnableChangeReflectSkyColor( switchMode_ == ON ? false : true);
			if(switchMode_ == ON)
				envTime->SetCurReflectSkyColor(reflectSkyColor_);
		}			
	}
}

void ActionSetFogOfWar::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(mode_, "mode", "Операция");
}

ActionSetWaterColor::ActionSetWaterColor()
: riverColor_(0, 0, 0, 1), seaColor_(0, 0, 0, 1)
{
	transitionTime_ = 5;
}

void ActionSetWaterColor::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(riverColor_, "riverColor", "Цвет неглубокой воды");
	ar.serialize(seaColor_, "seaColor", "Цвет глубокой воды");
	ar.serialize(transitionTime_, "transitionTime", "Время перехода, секунды");
}

void ActionSetWaterColor::activate()
{
//	environment->water()->GetColor(riverColorPrev_, seaColorPrev_);
	transitionTimer_.start(round(transitionTime_*1000));
}

bool ActionSetWaterColor::workedOut()
{
	float t = transitionTimer_();
	sColor4c riverColor, seaColor;
	riverColor.interpolate(riverColorPrev_, riverColor_, t);
	seaColor.interpolate(seaColorPrev_, seaColor_, t);
//	environment->water()->SetColor(riverColor, seaColor);
	return t > 1.f - FLT_EPS;
}
