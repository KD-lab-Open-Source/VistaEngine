#include "StdAfx.h"
#include "Render/src/NParticleKey.h"
#include "kdw/_PropertyRowBuiltin.h"
#include "kdw/PropertyRow.h"
#include "kdw/PropertyTree.h"
#include "kdw/Win32/Drawing.h"
#include "kdw/Win32/Window.h"
#include "kdw/PropertyTreeModel.h"
#include "EffectDocument.h"
#include "kdw/ImageStore.h"
#include "Serialization\SerializationFactory.h"

namespace kdw{

void drawAnimationCheck(HDC dc, const RECT& rect, bool animated);

ImageStore globalCurveImage(16, 16, "CURVE", RGB(255, 0, 255));

void drawAnimationCheck(HDC dc, const RECT& rect, bool animated)
{
	int size = 16;
	int offsetY = ((rect.bottom - rect.top) - size) / 2;
	int offsetX = ((rect.right - rect.left) - size) / 2;
	globalCurveImage._draw(animated ? 1 : 0, dc, rect.left + offsetX, rect.top + offsetY);
}

template<class Type>
class PropertyRowCurveImpl : public PropertyRowImpl<Type, PropertyRowCurveImpl>, PropertyRowNumericInterface{
public:
	typedef PropertyRowImpl<Type, PropertyRowCurveImpl> Base;
	PropertyRowCurveImpl() {}
	PropertyRowCurveImpl(const char* name, const char* nameAlt, Type& value)
	: Base(name, nameAlt, value)
	, currentValueChanged_(false)
	, currentValue_(0.0f)
	{
	}
	PropertyRowCurveImpl(void* object, int size, const char* name, const char* nameAlt, const char* typeName)
	: Base(object, size, name, nameAlt, typeName)
	, showSpline_(false)
	, disabled_(false)
	, currentValueChanged_(false)
	, currentValue_(0.0f)
	{
		if(NodeCurve* node = getDocumentNode())
			showSpline_ = node->visible();
		else
			disabled_ = true;
	}

	int minimalWidth() const{ return 60; }
	void redraw(HDC dc, const RECT& iconRect, const RECT& widgetRect, const RECT& floorRect, PropertyRow* hostRow)
	{
		if(!disabled_)
			drawAnimationCheck(dc, iconRect, showSpline_);
		Win32::drawEdit(dc, widgetRect, valueAsString().c_str(), Win32::defaultFont());
	}
	NodeCurve* getDocumentNode() const{
		std::string name(value().name());
		if(NodeEmitter* nodeEmitter = globalDocument->activeEmitter()){
			if(NodeCurve* nodeCurve = nodeEmitter->curveByName(name.c_str()))
				return nodeCurve;
			else
				return 0;
		}
		return 0;
	}
	bool onSelect(PropertyTreeModel* model){
		if(NodeCurve* node = getDocumentNode()){
			globalDocument->setActiveCurve(node);
			globalDocument->signalCurveToggled().emit();
			return true;
		}
        return true;
	}
	bool onActivateIcon(PropertyTree* tree){
        showSpline_ = !showSpline_;
		if(NodeCurve* node = getDocumentNode()){
			node->setVisible(showSpline_);
			globalDocument->signalCurveToggled().emit();
			tree->update();
		} 
		return true;
	}
	bool onActivate(PropertyTree* tree){
		return onActivateIcon(tree);
	}
	bool hasIcon() const{ return true; }
	PropertyRow* clone(){
		return new PropertyRowCurveImpl<Type>(name_, nameAlt_, Type(value_));
	}
	PropertyRowWidget* createWidget(PropertyTreeModel* model){
		return new PropertyRowWidgetNumeric(this, model, this);
	}
	WidgetPosition widgetPosition() const{ return WIDGET_POSITION_COLUMN; }

	void setValueFromString(const char* str){
		currentValue_ = atof(str);
		currentValueChanged_ = true;
	}
	bool assignTo(void* object, int size){
		if(currentValueChanged_){
			if(NodeCurve* nodeCurve = getDocumentNode())
				nodeCurve->setCurrentValue(currentValue_);
		}
		return false;
	}
	std::string valueAsString() const{
		NodeCurve* nodeCurve = getDocumentNode();
		XBuffer buf;
		if(nodeCurve)
			buf <= nodeCurve->currentValue();
		return static_cast<const char*>(buf);
	}
protected:
	float currentValue_;
	bool currentValueChanged_;
	bool showSpline_;
	bool disabled_;
};

#define REGISTER_PROPERTY_ROW_CURVE(CurveType)                                                            \
typedef CurveWrapper<CurveType> CurveWrapper##CurveType;                                                  \
class PropertyRowCurve##CurveType : public PropertyRowCurveImpl<CurveWrapper##CurveType>{                 \
public:                                                                                                   \
	typedef PropertyRowCurveImpl<CurveWrapper##CurveType> Base;                                           \
	PropertyRowCurve##CurveType() {}                                                                       \
	PropertyRowCurve##CurveType(void* object, int size, const char* name, const char* nameAlt, const char* typeName) \
	: Base(object, size, name, nameAlt, typeName)                                                         \
	{                                                                                                     \
	}                                                                                                     \
};																										  \
REGISTER_PROPERTY_ROW(CurveWrapper##CurveType, PropertyRowCurve##CurveType)

REGISTER_PROPERTY_ROW_CURVE(KeysFloat)
REGISTER_PROPERTY_ROW_CURVE(EffectBeginSpeeds)

#undef REGISTER_PROPERTY_ROW_CURVE
}
