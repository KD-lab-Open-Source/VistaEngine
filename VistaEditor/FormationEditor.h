#ifndef __FORMATION_EDITOR_H_INCLUDED__
#define __FORMATION_EDITOR_H_INCLUDED__

#include "kdw\ViewPort2D.h"
#include "kdw\Plug.h"
#include "kdw/CheckBox.h"
#include "kdw\Win32/Handle.h"
#include "..\units\AttributeSquad.h"

struct Color4c;

class FormationEditorPlug;

class FormationEditorViewPort : public kdw::Viewport2D
{
public:
	FormationEditorViewPort(FormationPattern& pattern);

	void selectCellUnderPoint(const Vect2f& point);

	void setPatternName(const char* name);
	const FormationPattern& pattern() const { return pattern_; }

	void onMouseMove(const Vect2i& delta);
	void onMouseButtonDown(kdw::MouseButton button);
	void onMouseButtonUp(kdw::MouseButton button);

	void onRedraw(HDC dc);
    
	void onFormationCellDelete();
	void onMenuAdd(int index);

	int selectedCellTypeIndex();

	void setShowTypeNames(bool showTypeNames) { showTypeNames_ = showTypeNames; };
	bool showTypeNames() const{ return showTypeNames_; }

private:
	int selected_cell_;
	FormationPattern& pattern_;
	bool moving_;
	bool showTypeNames_;
};

class FormationEditor : public kdw::VBox{
public:
	FormationEditor(FormationEditorPlug* plug);
	void onSet();
protected:
	void onShowTypeNames();
	FormationPattern& value();

	FormationEditorViewPort* viewport_;
	kdw::CheckBox* showTypeNames_;
	FormationEditorPlug* plug_;
};

class FormationEditorPlug: public kdw::Plug<FormationPattern, FormationEditorPlug>{
public:
	FormationEditorPlug();
	static std::string valueAsString(const FormationPattern& value){
		return value.c_str();
	}
	void onSet();
	kdw::Widget* asWidget();
protected:
	ShareHandle<FormationEditor> editor_;
};

#endif
