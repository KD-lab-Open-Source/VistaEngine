#ifndef __ANCOR_H_INCLUDED__
#define __ANCOR_H_INCLUDED__

#include "BaseUniverseObject.h"
#include "Handle.h"

class Archive;
class UI_MinimapSymbol;

class Anchor : public BaseUniverseObject, public ShareHandleBase {
public:
	enum Type {
		GENERAL,
		START_LOCATION,
		MINIMAP_MARK
	};

	static const char* typeText(Type type){
		switch(type){
		case START_LOCATION:
			return "Start Point";
		case MINIMAP_MARK:
			return "Mark";
		case GENERAL:
		default:
			return "anchor";
		}
	}
	Anchor();

	const char* c_str() const	{ return label_.c_str(); }
	void setLabel(const char* label) { label_ = label; }
	bool operator == (const char* label) const		{ return label_ == label; }

	void showEditor() const;
	void showDebug() const;
	
	void serialize(Archive& ar);

	Type type() const { return type_; }
	void setType(Type type) { type_ = type; }

	const UI_MinimapSymbol* symbol() const { return symbol_; }

	UniverseObjectClass objectClass() const{ return UNIVERSE_OBJECT_ANCHOR; }
protected:
	string label_;
	Type type_;
	UI_MinimapSymbol* symbol_;
};	

#endif // #ifndef __ANCOR_H_INCLUDED__

