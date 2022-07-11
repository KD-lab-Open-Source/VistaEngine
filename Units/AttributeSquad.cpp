#include "StdAfx.h"
#include "AttributeSquad.h"
#include "Serialization.h"
#include "TypeLibraryImpl.h"

WRAP_LIBRARY(FormationPatterns, "FormationPattern", "Паттерны формаций", "Scripts\\Content\\FormationPattern", 0, true);

WRAP_LIBRARY(AttributeSquadTable, "AttributeSquad", "Типы сквадов", "Scripts\\Content\\AttributeSquad", 0, false);

///////////////////////////////////////////////////

void FormationPattern::Cell::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(type, "type", "Тип");
}

void FormationPattern::serialize(Archive& ar)
{
	StringTableBase::serialize(ar); 
	ar.serialize(cells_, "cells", "Ячейки");
}

///////////////////////////////////////////////////

AttributeSquad::Formation::Formation() 
: rotateFront(true), attackByRadius(true), attackRadius(100), hardFormation(false), correctSpeed(false)
{
}

void AttributeSquad::Formation::serialize(Archive& ar)
{
 	ar.serialize(formationPattern, "formationPattern", "Паттерн");
 	ar.serialize(rotateFront, "rotateFront", "Поворачивать фронт");
	ar.serialize(hardFormation, "hardFormation", "Жесткая формация");
 	ar.serialize(attackByRadius, "attackByRadius", "Атаковать на указанном радиусе");
	ar.serialize(correctSpeed, "correctSpeed", "Корректировать скорость юнитов");
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
}

void AttributeSquad::serialize(Archive& ar) 
{
	StringTableBase::serialize(ar);
	ar.serialize(numbers, "numbers", "Количество юнитов");
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
		if(numbers.empty())
			numbers.push_back(UnitNumber());
	}

//	ar.serialize(homePositionOffsetFactor, "homePositionOffsetFactor", "homePositionOffsetFactor");
//	ar.serialize(formationRadiusBase, "formationRadiusBase", "formationRadiusBase");
//	ar.serialize(followDistanceFactor, "followDistanceFactor", "followDistanceFactor");
}

