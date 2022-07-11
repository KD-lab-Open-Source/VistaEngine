#include "StdAfx.h"

#include "Anchor.h"
#include "ExternalShow.h"
#include "CameraManager.h"
#include "Serialization.h"
#include "..\UserInterface\UI_Types.h"
#include "EditorVisual.h"

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(Anchor, Type, "Type")
REGISTER_ENUM_ENCLOSED(Anchor, GENERAL, "Общий");
REGISTER_ENUM_ENCLOSED(Anchor, START_LOCATION, "Стартовая точка");
REGISTER_ENUM_ENCLOSED(Anchor, MINIMAP_MARK, "Отметка на миникарте");
END_ENUM_DESCRIPTOR_ENCLOSED(Anchor, Type)

Anchor::Anchor()
: type_(GENERAL)
, symbol_(0)
{
	setRadius(10);
}

void Anchor::showEditor() const
{
	if(editorVisual().isVisible(objectClass())){
		editorVisual().drawRadius(position(), radius(), EditorVisual::RADIUS_OBJECT, selected());
		std::string text = typeText(type());
		if(label_.c_str()[0] != '\0'){
			text += " - ";
			text += label_;
		}
		editorVisual().drawText(position(), text.c_str(), EditorVisual::TEXT_LABEL);
		editorVisual().drawOrientationArrow(pose(), selected());
	}
}

void Anchor::showDebug() const
{
	const char* text = typeText(type());
	
	sColor4f color = selected() ? sColor4f(RED) : sColor4f(WHITE); 
	
	show_text(position(), text, color);
	show_vector(position(), radius(), color);
}


void Anchor::serialize(Archive& ar)
{
	ar.serialize(type_, "type", 0);
	if(!ar.isEdit()){
		ar.serialize(pose_, "pose", "Позиция");
	}
	ar.serialize(label_, "label", type_ == START_LOCATION ? 0 : "Имя метки");
	ar.serialize(radius_, "radius", "Радиус");
	if(type_ == MINIMAP_MARK){
		if(!ar.isEdit() && !isUnderEditor())
			ar.serialize(selected_, "active", 0);
		if(!symbol_)
			symbol_ = new UI_MinimapSymbol;
		ar.serialize(*symbol_, "symbol", "Символ на миникарте");
	}
	else if(symbol_){
		delete symbol_;
		symbol_ = 0;
	}
}
