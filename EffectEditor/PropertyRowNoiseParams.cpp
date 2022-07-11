#include "StdAfx.h"
#include "kdw/PropertyRow.h"
#include "kdw/PropertyTreeModel.h"
#include "kdw/PropertyTree.h"
#include "kdw/Win32/Drawing.h"
#include "kdw/Win32/Window.h"
#include "Render/src/NParticle.h"
#include "Render/src/NParticleKey.h"
#include "Serialization\SerializationFactory.h"

#include "NoiseParamsDialog.h"

namespace kdw{

class PropertyRowNoiseParams : public PropertyRowImpl<NoiseParams, PropertyRowNoiseParams>{
public:
	PropertyRowNoiseParams(void* object, int size, const char* name, const char* nameAlt, const char* typeName)
	: PropertyRowImpl<NoiseParams, PropertyRowNoiseParams>(object, size, name, nameAlt, typeName)
	{
	}
	PropertyRowNoiseParams()
	{
	}

	bool onActivate(PropertyTree* tree);
	WidgetPosition widgetPosition() const{ return WIDGET_POSITION_COLUMN; }
    
	void redraw(HDC dc, const RECT& iconRect, const RECT& widgetRect, const RECT& lineRect, PropertyRowState state);
};

// ---------------------------------------------------------------------------

bool PropertyRowNoiseParams::onActivate(PropertyTree* tree)
{
	NoiseParamsDialog dialog(tree->mainWindow());
	dialog.set(value());
	if(dialog.showModal() == RESPONSE_OK){
		value() = dialog.get();
		tree->model()->rowChanged(this);
	}
	return true;
}

void PropertyRowNoiseParams::redraw(HDC dc, const RECT& iconRect, const RECT& widgetRect, const RECT& lineRect, PropertyRowState state)
{
	RECT rt = widgetRect;
	//if(!disabled_)
	Win32::drawButton(dc, widgetRect, TRANSLATE("Изменить..."), Win32::defaultFont());
}

REGISTER_PROPERTY_ROW(NoiseParams, PropertyRowNoiseParams)

}
