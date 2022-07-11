#include "StdAfx.h"
#include "EnvironmentColors.h"
#include "Serialization.h"
#include "RangedWrapper.h"

EnvironmentAttributes::EnvironmentAttributes()
{
	fogOfWarColor_.set(0, 0, 0, 200);
	scout_area_alpha_ = 220;
	fogMinimapAlpha_ = 255;
	fog_start_ = 900.f;
	fog_end_ = 1100.f;
	game_frustrum_z_min_ = 5.f;
	game_frustrum_z_max_vertical_ = 4000.f;
	game_frustrum_z_max_horizontal_ = 4000.f;
	hideByDistanceFactor_ = 40.f; 
	hideByDistanceRange_ = 100.f;
	hideSmoothly_ = false;
	effectHideByDistance_ = false;
	effectNearDistance_ = 0.f;
	effectFarDistance_ = 100000.f;
	dayTimeScale_ = 500.f;
	nightTimeScale_ = 1000.f;
	rainConstant_ = 1.f;
	water_dampf_k_ = 7;
	height_fog_circle_ = 500;
	miniDetailTexResolution_ = 4;
}

void EnvironmentAttributes::serialize(Archive& ar)
{
	ar.openBlock("Fog of war", "Туман войны");
		ar.serialize(fogOfWarColor_, "fogOfWarColor", "Цвет тумана войны");
		fogOfWarColor_.a = clamp(fogOfWarColor_.a , 100, 255);
		ar.serialize(scout_area_alpha_, "scout_area_alpha", "Прозрачность разведанного");
		scout_area_alpha_ = clamp(scout_area_alpha_, 100, 255);
		ar.serialize(RangedWrapperi(fogMinimapAlpha_, 100, 255), "fogMinimapAlpha", "Прозрачность тумана войны на миникарте");
	ar.closeBlock();

	ar.openBlock("Environment fog", "Туман на мире");
		ar.serialize(fog_start_, "fog_start", "Ближняя граница тумана");
		ar.serialize(fog_end_, "fog_end", "Дальняя граница тумана");
		ar.serialize(RangedWrapperi(height_fog_circle_, 0, 2000), "height_fog_circle", "Высота перехода к туману");
	ar.closeBlock();
	
	ar.openBlock("Efects", "Эффекты");
		ar.serialize(effectHideByDistance_, "effectHideByDistance", "Скрывать эффекты при удалении");
		ar.serialize(effectNearDistance_, "effectNearDistance", "Ближняя граница эффектов");
		ar.serialize(effectFarDistance_, "effectFarDistance", "Дальняя граница эффектов");
	ar.closeBlock();

	ar.openBlock("Camera frustrum", "Обрезка кадра");
		ar.serialize(RangedWrapperf(game_frustrum_z_min_, 1.0f, 100.0f), "game_frustrum_z_min", "Ближняя граница камеры");
		ar.serialize(RangedWrapperf(game_frustrum_z_max_vertical_, 100.0f, 13000.0f), "game_frustrum_z_max", "Дальняя граница камеры (в вертикальном положении)");
		if(ar.isInput())
			game_frustrum_z_max_horizontal_ = game_frustrum_z_max_vertical_;
		ar.serialize(RangedWrapperf(game_frustrum_z_max_horizontal_, 100.0f, 13000.0f), "game_frustrum_z_max_horizontal", "Дальняя граница камеры (в горизонтальном положении)");
		//ar.serialize(hideByDistanceFactor_, "hideByDistanceFactor", "Исчезает при удалении (фактор)");
		//ar.serialize(hideByDistanceRange_, "hideByDistanceRange", "Исчезает при удалении (диапазон)");
		ar.serialize(hideSmoothly_, "hideSmoothly", "Исчезать плавно");
	ar.closeBlock();

	ar.openBlock("Time scale", "Масштаб времени суток");
		ar.serialize(dayTimeScale_, "dayTimeScale", "Масштаб времени днем");
		ar.serialize(nightTimeScale_, "nightTimeScale", "Масштаб времени ночью");
	ar.closeBlock();

	ar.openBlock("Water", "Вода");
		ar.serialize(rainConstant_, "rainConstant", "Параметр высыхания");
		ar.serialize(RangedWrapperi(water_dampf_k_, 1, 15), "water_dampf_k", "Скорость течения");
	ar.closeBlock();

	ar.serialize(RangedWrapperi(miniDetailTexResolution_, 1, 5), "miniDetailTexResolution", "Разрешение мелкодетальной текстуры");

	ar.serialize(timeColors_,"timeColors_","Цвет времени");
	ar.serialize(coastSprites_,"coastSprites","Прибрежные спрайты");
	ar.serialize(fallout_,"fallout","Осадки");
	ar.serialize(waterPlume_,"waterPlume","Следы на воде");
	ar.serialize(windMap_,"windMap","Ветер");
}
