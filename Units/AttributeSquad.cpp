#include "StdAfx.h"
#include "AttributeSquad.h"
#include "IronLegion.h"
#include "Serialization\Serialization.h"
#include "Serialization\SerializationFactory.h"
#include "Serialization\StringTableImpl.h"

WRAP_LIBRARY(FormationPatterns, "FormationPattern", "Паттерны формаций", "Scripts\\Content\\FormationPattern", 0, LIBRARY_EDITABLE);

WRAP_LIBRARY(AttributeSquadTable, "AttributeSquad", "Типы сквадов", "Scripts\\Content\\AttributeSquad", 0, LIBRARY_EDITABLE | LIBRARY_IN_PLACE);

BEGIN_ENUM_DESCRIPTOR(VelocityCorrection, "VelocityCorrection");
REGISTER_ENUM(VELOCITY_DEFAULT, "не корректировать");
REGISTER_ENUM(VELOCITY_MIN, "по минимальной");
REGISTER_ENUM(VELOCITY_AVERAGE, "по средней");
REGISTER_ENUM(VELOCITY_MAX, "по максимальной");
END_ENUM_DESCRIPTOR(VelocityCorrection);

///////////////////////////////////////////////////

void FormationPattern::Cell::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(type, "type", "^");
}

void FormationPattern::serialize(Archive& ar)
{
	StringTableBase::serialize(ar); 
	ar.serialize(cells_, "cells", "Ячейки");
}

///////////////////////////////////////////////////

AttributeSquad::Formation::Formation() 
: rotateFront(true), attackByRadius(true), attackRadius(100), uniformFormation(true), velocityCorrection(VELOCITY_DEFAULT)
{
}

void AttributeSquad::Formation::serialize(Archive& ar)
{
 	ar.serialize(formationPattern, "formationPattern", "^");
 	ar.serialize(rotateFront, "rotateFront", "Поворачивать фронт");
	ar.serialize(uniformFormation, "uniformFormation", "Однородная формация");
 	ar.serialize(attackByRadius, "attackByRadius", "Атаковать на указанном радиусе");
	if(!ar.serialize(velocityCorrection, "velocityCorrection", "Корректировать скорость юнитов")){
		bool correctSpeed = false;
		ar.serialize(correctSpeed, "correctSpeed", "Корректировать скорость юнитов");
		if(correctSpeed)
			velocityCorrection = VELOCITY_AVERAGE;
	}
	if(!ar.isEdit() || attackByRadius)
 		ar.serialize(attackRadius, "attackRadius", "Радиус атаки");
}

AttributeSquad::AttributeSquad(const char* name) 
: StringTableBase(name)
{
	homePositionOffsetFactor.set(1.2f, 0);
	formationRadiusBase = 10;
	followDistanceFactor = 1; // followDistanceFactor*(radius1 + radius2)
	unitClass_ = UNIT_CLASS_SQUAD;
	internal = true;
	enableJoin = true;
	automaticJoin = false;
	automaticJoinRadius = 100;
	automaticJoinRadiusEffect.color.set(255, 255, 255, 0);
	joinRadius = 100;
	joinRadiusEffect.color.set(255, 255, 255, 0);
	disableMainUnitAutoAttack = false;
	forceUnitsAutoAttack = false;
	followMainUnitInAutoMode = false;
	showSightSector = true;
	showSelectRadius = false;
	accountingNumber = 0;
	showWayPoint = true;
	showAllWayPoints = false;
	showAllWayPointDist = 15.f;
}

void AttributeSquad::serialize(Archive& ar) 
{
	if(ar.isOutput() && !ar.isEdit()){	// Сложные расчеты - только перед записью
		allowedUnitsAttributes.clear();
		AttributeLibrary::Map::const_iterator mi;
		FOR_EACH(AttributeLibrary::instance().map(), mi){
			const AttributeBase* attribute = mi->get();
			if(attribute && attribute->isLegionary()){
				const AttributeLegionary* legionary = safe_cast<const AttributeLegionary*>(attribute);
				if(legionary->squad == this)
					allowedUnitsAttributes.add(legionary);
			}
		}
	}

	StringTableBase::serialize(ar);
	ar.serialize(parametersInitial, "parametersInitial", "Личные (начальные) параметры сквада");
	ar.serialize(formations, "formations", "Формации");
	ar.serialize(enableJoin, "enableJoin", "Разрешить объединение сквадов");
	ar.serialize(automaticJoin, "automaticJoin", "Автоматически присоединять юнитов");
	if(automaticJoin){
		ar.serialize(automaticJoinRadius, "automaticJoinRadius", "Радиус автоматического присоединения");
		ar.serialize(automaticJoinRadiusEffect, "automaticJoinRadiusEffect", "Визуализация радиуса автоматического присоединения");
	}
	ar.serialize(joinRadius, "joinRadius", "Максимальный радиус объеденения сквадов");
	ar.serialize(joinRadiusEffect, "joinRadiusEffect", "Визуализация максимального радиуса объеденения сквадов");

	if(ar.isInput()){
		internal = true;
		if(formations.empty())
			formations.push_back(Formation());
	}

	ar.serialize(accountingNumber, "accountingNumber", "Число, учитываемое в максимальном количестве юнитов");
	ar.serialize(unitNumberMaxType, "unitNumberMaxType", "Тип максимального количества юнитов");
	ar.serialize(allowedUnits, "allowedUnits", "Список допустимых типов юнитов");

	ar.serialize(disableMainUnitAutoAttack, "disableMainUnitAutoAttack", "Запретить автоматическую атаку у главного юнита");
	ar.serialize(forceUnitsAutoAttack, "forceUnitsAutoAttack", "Не главные юниты атакуют только автоматически");

	ar.serialize(followMainUnitInAutoMode, "followMainUnitInAutoMode", "Следовать за главным в автоматическом режиме");

	ar.serialize(permanentEffects, "permanentEffects", "постоянные эффекты");
	ar.serialize(mainUnitEffect, "mainUnitEffect", "Эффект для главного юнита в скваде");
	ar.serialize(waitingUnitEffect, "waitingUnitEffect", "Эффект для ждущего юнита в скваде");
	ar.serialize(showSightSector, "showSightSector", "Показывать сектор обзора");

	if(ar.openBlock("Interface", "Интерфейс")){
		ar.serialize(selectSprites_, "Miniatures", "Миниатюры");

		ar.serialize(selectionCursor_, "selection_cursor", "Курсор выбора");
		selectionCursorProxy_ = selectionCursor_;

		ar.serialize(initialHeightUIParam, "initialHeightUIParam", "высота юнита для вывода значений");

		if(ar.openBlock("squadSign", "Знак сквада")){
			ar.serialize(showSpriteForUnvisible, "showSpriteForUnvisible", "Выводить когда всего сквада не видно");
			ar.serialize(selectBySprite, "selectBySprite", "Селектить по знаку");
			ar.serialize(selectSprites, "selectSprites", "Выводимые спрайты");
			ar.closeBlock();
		}

		if(ar.openBlock("minimap", "Обозначение на миникарте")){
			ar.serialize(minimapSymbolType_, "symbolType", "тип пометки");
			if(minimapSymbolType_ == UI_MINIMAP_SYMBOLTYPE_SELF){
				ar.serialize(minimapSymbol_, "minimapSymbol", "Не выделенный");
				minimapSymbol_.scaleByEvent = false;
				ar.serialize(minimapPermanentSymbol_, "minimapSymbolSelected", "Выделенный");
				minimapPermanentSymbol_.scaleByEvent = false;
			}
			ar.closeBlock();
		}

		ar.serialize(showWayPoint, "showWayPoint", "Показывать точку назначения");
		ar.serialize(showTriggerWayPoint, "showTriggerWayPoint", "Отображать при команде идти из триггера");
		ar.serialize(targetPoint, "targetPoint", "Собственная отметка точки назначения");

		ar.serialize(showAllWayPoints, "showAllWayPoints", "Показывать все точки пути");
		if(showAllWayPoints)
			ar.serialize(showAllWayPointDist, "showAllWayPointDist", "Расстояние между пометками");

		if(ar.openBlock("selection", "При селекте")){
			ar.serialize(showSelectRadius, "showSelectRadius", "Показывать селект");
			ar.serialize(selectRadius, "selectRadius", "Радиус сквада для селекта, 0 - по реальному радиусу");
			ar.closeBlock();
		}

		ar.closeBlock();
	}

	ar.serialize(allowedUnitsAttributes, "allowedUnitsAttributes", 0);

//	ar.serialize(homePositionOffsetFactor, "homePositionOffsetFactor", "homePositionOffsetFactor");
//	ar.serialize(formationRadiusBase, "formationRadiusBase", "formationRadiusBase");
//	ar.serialize(followDistanceFactor, "followDistanceFactor", "followDistanceFactor");
}

