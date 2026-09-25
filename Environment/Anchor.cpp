#include "stdafx.h"

#include "Anchor.h"
#include "CameraManager.h"
#include "Serialization/Serialization.h"
#include "Serialization/SerializationFactory.h"
#include "Serialization/EnumDescriptor.h"
#include "Serialization/Decorators.h"
#include "UserInterface/UI_MinimapSymbol.h"
#include "EditorVisual.h"

UNIT_LINK_GET(Anchor);

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(Anchor, Type, "Type")
REGISTER_ENUM_ENCLOSED(Anchor, GENERAL, "Общий");
REGISTER_ENUM_ENCLOSED(Anchor, START_LOCATION, "Стартовая точка");
REGISTER_ENUM_ENCLOSED(Anchor, MINIMAP_MARK, "Отметка на миникарте");
END_ENUM_DESCRIPTOR_ENCLOSED(Anchor, Type)

struct UI_MinimapSymbolPolymorphic : public UI_MinimapSymbol, public PolymorphicBase {
	void serialize(Archive& ar){
		UI_MinimapSymbol::serialize(ar);
	}
};

REGISTER_CLASS(UI_MinimapSymbolPolymorphic, UI_MinimapSymbolPolymorphic, "Символ на миникарте");

Anchor::Anchor(bool doNotRegister)
: type_(GENERAL)
, symbol_(0)
, doNotRegister_(doNotRegister)
{
	if(!doNotRegister_)
		unitID_.registerUnit(this);
	setRadius(10);
}

const UI_MinimapSymbol* Anchor::symbol() const
{
	return symbol_.get();
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
	
	Color4f color = selected() ? Color4f::RED : Color4f::WHITE; 
	
	show_text(position(), text, color);
	show_vector(position(), radius(), color);
}

void Anchor::serialize(Archive& ar)
{
	__super::serialize(ar);

	if(ar.isInput() && dead() && !doNotRegister_)
		unitID_.registerUnit(this);

	ar.serialize(type_, "type", 0);
	if(!ar.isEdit())
		ar.serialize(pose_, "pose", "Позиция");
	ar.serialize(selected_, "active", 0);
	ar.serialize(label_, "label", type_ == START_LOCATION ? 0 : "Имя метки");
	ar.serialize(radius_, "radius", "Радиус");
#ifdef MAELSTROM_DATA
	// The symbol was a bare UI_MinimapSymbol the anchor owned outright, written as a plain
	// nested "symbol" node and only by a MINIMAP_MARK; 2008 made it a polymorphic handle
	// under "uisymbol". Asked for at the 2008 name nothing is found, symbol() stays null and
	// UI_Minimap::addAnchor drops the anchor on the floor -- which is every campaign mission's
	// objective marker: 47 of them across 16 worlds, all SYMBOL_SPRITE, 44 on the animated
	// point.avi and 3 on escape.avi, the ring the minimap pulses over where you are being sent.
	if(type_ == MINIMAP_MARK){
		if(!symbol_)
			symbol_.set(new UI_MinimapSymbolPolymorphic);
		if(!ar.serialize(static_cast<UI_MinimapSymbol&>(*symbol_), "symbol", "Символ на миникарте") && ar.isInput())
			symbol_ = 0;
	}
	else if(ar.isInput())
		symbol_ = 0;
#else
	if(ar.isEdit()){
		typedef OptionalPtr<UI_MinimapSymbolPolymorphic,
			                PolymorphicHandle<UI_MinimapSymbolPolymorphic> > OptionalPtrType;
		ar.serialize(static_cast<OptionalPtrType&>(symbol_),
		             "uisymbol", "Символ на миникарте");
	}
	else
		ar.serialize(symbol_, "uisymbol", "Символ на миникарте");
#endif
}