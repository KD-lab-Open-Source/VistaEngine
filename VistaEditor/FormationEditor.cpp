#include "stdafx.h"
#include "FormationEditor.h"
#include "Serialization\StringTable.h"
#include "kdw\Win32\Handle.h"
#include "util\kdw\PopupMenu.h"
#include "kdw\ClassMenu.h"
#include "kdw\Win32\Window.h"

#include "..\units\AttributeSquad.h"

DECLARE_SEGMENT(FormationEditor)
REGISTER_PLUG(FormationPattern, FormationEditorPlug)

#pragma warning(push)
#pragma warning(disable: 4355)

FormationEditorPlug::FormationEditorPlug()
: editor_(new FormationEditor(this))
{
}

#pragma warning(pop)

void FormationEditorPlug::onSet()
{
	editor_->onSet();
}

kdw::Widget* FormationEditorPlug::asWidget()
{
	return editor_;
}

FormationEditor::FormationEditor(FormationEditorPlug* plug)
: kdw::VBox(8)
, plug_(plug)
{
	viewport_ = new FormationEditorViewPort(plug->value());
	add(viewport_, true, true, true);

	kdw::VBox* vbox = new kdw::VBox(4);
	add(vbox);
	{
			showTypeNames_ = new kdw::CheckBox("Show type names");
			vbox->add(showTypeNames_, true, true, true);
			showTypeNames_->setButtonLike(false);
			showTypeNames_->setStatus(false);
			showTypeNames_->signalChanged().connect(this, &FormationEditor::onShowTypeNames);
	}
}

void FormationEditor::onSet()
{
	viewport_->redraw();
}

void FormationEditor::onShowTypeNames()
{
	viewport_->setShowTypeNames(showTypeNames_->status());
	viewport_->redraw();
}

FormationPattern& FormationEditor::value()
{ 
	return plug_->value(); 
}

FormationEditorViewPort::FormationEditorViewPort(FormationPattern& pattern)
: kdw::Viewport2D(0, 12)
, selected_cell_(-1)
, pattern_(pattern)
, moving_(false)
, showTypeNames_(false)
{
}

void FormationEditorViewPort::onRedraw(HDC dc)
{
	__super::onRedraw(dc);

	Rectf rt = Rectf(-200, -200, 400, 400);
	fillRectangle(dc, rt, Color4c::WHITE);

	Color4c color1(216, 216, 216);
	Color4c color2(128, 128, 128);
	for(float x = rt.left(); x <= rt.right(); x += 10.0f) {
		drawLine(dc, Vect2f(x, rt.top()), Vect2f(x, rt.bottom()), round(x) % 100 == 0 ? color2 : color1);
		drawLine(dc, Vect2f(rt.left(), x), Vect2f(rt.right(), x), round(x) % 100 == 0 ? color2 : color1);
	}

	Win32::StockSelector old_font(dc, positionFont_);
	for(int i = 0; i < pattern_.cells().size(); i++){
		const FormationPattern::Cell& cell = pattern_.cells()[i];
		Vect2f pos = cell;
		drawCircle(dc, pos, cell.type->radius(), cell.type->color(), i == selected_cell_ ? 3 : 1);

		XBuffer str;
		if(showTypeNames_)
			str < " " <= i + 1 < " (" < cell.type->c_str() <")";
		else
			str < " " <= i + 1 < " ";

		drawText(dc, pos, str, Color4c::BLACK, Color4c::ZERO);
	}
}

void FormationEditorViewPort::onMouseButtonDown(kdw::MouseButton button)
{
	__super::onMouseButtonDown(button);

	if(button == kdw::MOUSE_BUTTON_RIGHT){
		selectCellUnderPoint(clickPoint());
		redraw();
	}
	else if(button == kdw::MOUSE_BUTTON_LEFT){
		selectCellUnderPoint(clickPoint());
		if(selected_cell_ != -1) 
			moving_ = true;
		redraw();
	}
}

class FormationTypeClassItemAdder: public kdw::ClassMenuItemAdder{
public:
	FormationTypeClassItemAdder(FormationEditorViewPort* formationEditor)
		: formationEditor_(formationEditor)
	{
	}
	void operator()(kdw::PopupMenuItem& root, int index, const char* text){
		root.add(text, index)
			.connect(formationEditor_, &FormationEditorViewPort::onMenuAdd)
			.check(index == formationEditor_->selectedCellTypeIndex());
	}
protected:
	FormationEditorViewPort* formationEditor_;
};

void FormationEditorViewPort::onMouseButtonUp(kdw::MouseButton button)
{
	__super::onMouseButtonUp(button);

	if(button == kdw::MOUSE_BUTTON_RIGHT){
		if(!scrolling()) {
			const char* comboList = UnitFormationTypes::instance().comboList();
			if(comboList[0] == '|')
				++comboList;
			
			selectCellUnderPoint(clickPoint());
			if(selected_cell_ == -1){
				kdw::PopupMenu menu(300);
				kdw::PopupMenuItem& addItem = menu.root().add(TRANSLATE("Добавить"));
				ComboStrings strings;
				splitComboList(strings, comboList, '|');
				FormationTypeClassItemAdder(this).generateMenu(addItem, strings);
				menu.spawn(this);
			} 
			else{
				kdw::PopupMenu menu(300);
				kdw::PopupMenuItem& addItem = menu.root().add(TRANSLATE("Тип"));
				ComboStrings strings;
				splitComboList(strings, comboList, '|');
				FormationTypeClassItemAdder(this).generateMenu(addItem, strings);
				menu.root().add(TRANSLATE("Удалить"))
					.connect(this, &FormationEditorViewPort::onFormationCellDelete)
					.setHotkey(sKey(VK_DELETE));
				menu.spawn(this);
			}
		}
	}
	if(button == kdw::MOUSE_BUTTON_LEFT){
		if(moving_) {
			if(selected_cell_ >= 0) {
				FormationPattern::Cell& cell = pattern_.cells_ [selected_cell_];
				float grid_size = 10.0f;
				if(Win32::isKeyPressed(VK_CONTROL)) {
					cell.set (round (cell.x / grid_size) * grid_size, round (cell.y / grid_size) * grid_size);
				}
			}
			moving_ = false;
		}
	}
}

void FormationEditorViewPort::onMouseMove(const Vect2i& delta)
{
	__super::onMouseMove(delta);

	if(moving_ && selected_cell_ >= 0) {
		pattern_.cells_[selected_cell_] += Vect2f(delta)/viewScale();
		redraw();
	}
}

void FormationEditorViewPort::selectCellUnderPoint(const Vect2f& v) 
{
	selected_cell_ = -1;

	FormationPattern::Cells::const_iterator it;
	int index = 0;
	FOR_EACH (pattern_.cells(), it) {
		const FormationPattern::Cell& cell = *it;
		float dist = cell.distance (v);
		if (dist <= cell.type->radius()) {
			selected_cell_ = index;
		}
		++index;
	}
}

int FormationEditorViewPort::selectedCellTypeIndex() 
{
	if(selected_cell_ != -1)
	{
		const char* comboList = UnitFormationTypes::instance().comboList();
		if(comboList[0] == '|')
			++comboList;
		ComboStrings strings;
		splitComboList(strings, comboList, '|');
		int ind = -1;
		for(ind = 0; ind < strings.size(); ind++)
			if(UnitFormationTypeReference(strings[ind].c_str()) == pattern_.cells_ [selected_cell_].type)
				break;
		return ind;
	}

	return -1;
}

void FormationEditorViewPort::onMenuAdd(int index)
{
	const char* comboList = UnitFormationTypes::instance().comboList();
	if(comboList[0] == '|')
		++comboList;
	ComboStrings strings;
	splitComboList(strings, comboList, '|');
	xassert(index >= 0 && index < strings.size());
	if (selected_cell_ != -1)  {
		FormationPattern::Cell& cell = pattern_.cells_ [selected_cell_];
		cell.type = UnitFormationTypeReference(strings[index].c_str());
	} else {
		FormationPattern::Cell cell;
		cell.set(clickPoint().x, clickPoint().y);
		cell.type = UnitFormationTypeReference(strings[index].c_str());
		pattern_.cells_.push_back (cell);
		selected_cell_ = pattern_.cells_.size () - 1;
	}
	redraw();
}

void FormationEditorViewPort::onFormationCellDelete()
{
	xassert (selected_cell_ >= 0);
	if (selected_cell_ >= 0) {
		pattern_.cells_.erase (pattern_.cells_.begin () + selected_cell_);
		selected_cell_ = -1;
		redraw();
	}
}

void FormationEditorViewPort::setPatternName(const char* name)
{
	pattern_.setName(name); 
}

