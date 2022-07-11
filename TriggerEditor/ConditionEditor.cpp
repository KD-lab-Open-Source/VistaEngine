#include "StdAfx.h"
#include "kdw/PopupMenu.h"
#include "kdw/ClassMenu.h"
#include "kdw/HSplitter.h"
#include "kdw/VSplitter.h"
#include "kdw/PropertyEditor.h"
#include "kdw/DragManager.h"
#include "ConditionEditor.h"
#include "ClassTree.h"
#include "Serialization\EnumDescriptor.h"

string conditionName(Condition* condition, bool withDigest = true){
	string result = TRANSLATE(FactorySelector<Condition>::Factory::instance().nameAlt(typeid(*condition).name()));
	string::size_type pos = result.find_last_of("\\");
	if(pos != string::npos)
		result = string(result.begin() + pos + 1, result.end())
  			 + " (" + string(result.begin(), result.begin() + pos) + ")";
	if(withDigest){
		string digest = kdw::generateDigest(Serializer(*condition));
		if(!digest.empty()){
			result += " [ ";
			result += digest;
			result += " ]";
		}
	}
	return result;
}

// ---------------------------------------------------------------------------

DECLARE_SEGMENT(ConditionEditor)
REGISTER_PLUG(EditableCondition, ConditionEditorPlug)

ShareHandle<Condition> ConditionViewer::conditionCopy_;

ConditionEditor::ConditionEditor(ConditionEditorPlug* plug)
: plug_(plug), kdw::HSplitter(0, 0)
{
	typedef FactorySelector<Condition>::Factory Factory;
	Factory& factory = Factory::instance();
	ComboStrings ignoredConditions;
	ignoredConditions.push_back(typeid(Condition).name());
	ignoredConditions.push_back(typeid(ConditionSwitcher).name());
	tree_ = new ClassTree(factory.comboStrings(), factory.comboStringsAlt(), ignoredConditions);

	kdw::VSplitter* vsplitter = new kdw::VSplitter;
	add(vsplitter, 0.4f);
	vsplitter->add(tree_, 0.6f);

	kdw::PropertyTree* propertyTree = new kdw::PropertyTree;
	propertyTree->setSelectFocused(true);
	propertyTree->setHideUntranslated(true);
	propertyTree->setCompact(true);
	propertyTree->setImmediateUpdate(true);
	vsplitter->add(propertyTree, 0.25);

	viewer_ = new ConditionViewer(plug, propertyTree);
	add(viewer_);
	showAll();
}

void ConditionEditor::onSet() 
{ 
	viewer_->onSet(); 
}

ConditionViewer::ConditionViewer(ConditionEditorPlug* plug, kdw::PropertyTree* propertyTree)
: kdw::Viewport2D(0, ConditionSlot::TEXT_HEIGHT)
, propertyTree_(propertyTree)
, moving_(false)
, draging_(false)
, selectedCondition_ (0)
, dragedCondition_ (0)
, plug_(plug)
{
	propertyTree->signalChanged().connect(this, &ConditionViewer::onTreeChanged);
}

void ConditionViewer::onMouseMove(const Vect2i& delta)
{
	__super::onMouseMove(delta);

	dragged_ = true;
	if(draging_){
		trackMouse(clickPoint(), true);
		//if(canBeDropped(point))
		//	SetCursor(::LoadCursor(0, MAKEINTRESOURCE(IDC_HAND)));
		//else
		//	SetCursor(::LoadCursor(0, MAKEINTRESOURCE(IDC_NO)));
		captureMouse();
	}
	else{
		if(lmbPressed() && delta.norm() >= 0.05){
			dragedCondition_ = selectedCondition_;
			trackMouse(clickPoint(), dragedCondition_ != 0);
			captureMouse();
		}
		else
			trackMouse(clickPoint());
	}
}

void ConditionViewer::onMouseButtonDown(kdw::MouseButton button)
{
	__super::onMouseButtonDown(button);

	if(button == kdw::MOUSE_BUTTON_RIGHT){
		selectedCondition_ = rootSlot_.findCondition(clickPoint());
		dragged_ = false;
	}
	else if(button == kdw::MOUSE_BUTTON_LEFT){
		selectedCondition_ = rootSlot_.findCondition(clickPoint());
		if(selectedCondition_){
			ConditionSlot* slot = rootSlot_.findSlot(selectedCondition_);
			if(slot->not_rect().point_inside(clickPoint())){
				slot->invert();		
				propertyTree_->revert();
			}
			else{
				propertyTree_->detach();
				propertyTree_->attach(Serializer(*selectedCondition_));
				editedCondition_ = selectedCondition_;
			}
		}
		else{
			propertyTree_->detach();
			editedCondition_ = 0;
		}
	}
	else if(button == kdw::MOUSE_BUTTON_LEFT_DOUBLE)
		onMenuEdit();
}

void ConditionViewer::onMenuEdit()
{
	if(selectedCondition_){
		if(kdw::edit(Serializer(*selectedCondition_), "Scripts\\TreeControlSetups\\EditConditionPropsState", kdw::ONLY_TRANSLATED, this, conditionName(selectedCondition_, false).c_str())){
			rootSlot_.create(value().condition);
			redraw();
		}
	}
}

void ConditionViewer::onTreeChanged() 
{ 
	if(editedCondition_){
		rootSlot_.create(value().condition);
		redraw(); 
	}
}

ConditionSwitcher* ConditionViewer::findParent(Condition* condition, ConditionSwitcher* parent)
{
	if(!parent)
		parent = dynamic_cast<ConditionSwitcher*>(&*value().condition);
	if(!parent || parent == condition)
		return 0;

	ConditionSwitcher::Conditions::iterator ci;
	FOR_EACH(parent->conditions, ci){
		if(*ci == condition)
			return parent;
		ConditionSwitcher* switcher = dynamic_cast<ConditionSwitcher*>(&**ci);
		ConditionSwitcher* result;
		if(switcher && (result = findParent(condition, switcher)) != 0)
			return result;
	}

	xassert(0);
	return 0;
}

void ConditionViewer::removeCondition(Condition* condition)
{
	if(ShareHandle<ConditionSwitcher> switcher = findParent(condition)){
		switcher->remove(condition);
		if(switcher->conditions.size() == 1){
			ConditionSwitcher* switcherParent = findParent(switcher);
			if(switcherParent)
				switcherParent->replace(switcher, switcher->conditions.front());
			else
				value().condition = switcher->conditions.front();
		}
	}
	else 
		value().condition = 0;
}

void ConditionViewer::onMenuDelete()
{
	if(!selectedCondition_)
		return;

	removeCondition(selectedCondition_);

	selectedCondition_ = 0;

	rootSlot_.create(value().condition);
	redraw();
}

void ConditionViewer::onMouseButtonUp(kdw::MouseButton button)
{
	__super::onMouseButtonUp(button);

	if(button == kdw::MOUSE_BUTTON_RIGHT){
		if(!dragged_ && (selectedCondition_ || !value().condition())){
			kdw::PopupMenu menu(300);
			menu.root().add(TRANSLATE("Удалить"))
				.connect(this, &ConditionViewer::onMenuDelete)
				.setHotkey(sKey(VK_DELETE));
			menu.root().addSeparator();
			menu.root().add(TRANSLATE("Редактировать..."))
				.connect(this, &ConditionViewer::onMenuEdit);
			menu.root().add(TRANSLATE("Копировать"))
				.connect(this, &ConditionViewer::onMenuCopy)
				.setHotkey(sKey('C' | sKey::CONTROL));
			menu.root().add(TRANSLATE("Вставить"))
				.connect(this, &ConditionViewer::onMenuPaste)
				.setHotkey(sKey('V' | sKey::CONTROL));
			
			menu.spawn(this);
		}
	}
	if(button == kdw::MOUSE_BUTTON_LEFT){
		trackMouse(clickPoint(), true);

		if(kdw::dragManager.dragging()){ 
			int index = kdw::dragManager.data();
			if(index >= 0)
				onDrop(createCondition(index));
			kdw::dragManager.drop();
		}
		else if(draging_ && dragedCondition_ && canBeDropped(selectedCondition_, dragedCondition_)){
			ConditionSwitcher* parent = findParent(selectedCondition_);
			if(parent && parent == findParent(dragedCondition_)){
				ConditionSwitcher::Conditions& conditions = parent->conditions;
				bool down = find(conditions.begin(), conditions.end(), dragedCondition_) < find(conditions.begin(), conditions.end(), selectedCondition_);
				parent->remove(dragedCondition_);
				ConditionSwitcher::Conditions::iterator it = find(conditions.begin(), conditions.end(), selectedCondition_);
				conditions.insert(down ? it + 1 : it, dragedCondition_);
				rootSlot_.create(value().condition);
				redraw();
			}
			else{
				removeCondition(dragedCondition_);
				rootSlot_.create(value().condition);
				trackMouse(clickPoint());
				onDrop(dragedCondition_);
			}
		}

		selectedCondition_ = 0;
		dragedCondition_ = 0;
		draging_ = false;

		releaseMouse();
	}
}

void ConditionViewer::onMenuCopy()
{
	if(selectedCondition_){
		BinaryOArchive oa;
		oa.serialize(*selectedCondition_, "condition", 0);
		typedef FactorySelector<Condition>::Factory Factory;
		Factory& factory = Factory::instance();
		int index = indexInComboListString(factory.comboListAlt(), factory.find(selectedCondition_).nameAlt());
		xassert(index >= 0);
		conditionCopy_ = createCondition(index);
		xassert(conditionCopy_);
		BinaryIArchive ia(oa);
		ia.serialize(*conditionCopy_, "condition", 0);
	}
}

void ConditionViewer::onMenuPaste()
{
	if(!selectedCondition_ || !conditionCopy_)
		return;
	
	ConditionSwitcher* parent = findParent(selectedCondition_);
	if(parent)
		parent->replace(selectedCondition_, conditionCopy_);
	else
		value().condition = conditionCopy_;

	rootSlot_.create(value().condition);
	redraw();
}

void ConditionViewer::onKeyDown(const sKey& key)
{
	switch(key.key){
	case 'C' | sKey::CONTROL:
		onMenuCopy();
		break;
	case 'V' | sKey::CONTROL:
		onMenuPaste();
		break;
	case VK_DELETE:
		onMenuDelete();
		break;
	};
}

ConditionPtr ConditionViewer::createCondition(int typeIndex) 
{
	return FactorySelector<Condition>::Factory::instance().findByIndex(typeIndex).create();
}

void ConditionViewer::drawSlot(HDC dc, const ConditionSlot& slot) 
{
	Color4c border_color(0, 0, 100, 255);
	Color4c fill_color(240, 240, 240, 255);
	
	Rectf rect(slot.rect());
	Rectf text_rect(slot.text_rect());
	Rectf not_rect (slot.not_rect());

	if(slot.condition() == selectedCondition_) 
		if(draging_) 
			fill_color = Color4c(255, 200, 200, 255);
		else 
			fill_color = Color4c(200, 255, 200, 255);

	if(slot.condition() == editedCondition_) 
		fill_color = Color4c(100, 255, 100, 255);

	fillRectangle(dc, rect, fill_color);
	drawRectangle(dc, rect, border_color);

	float textHeight_ = ConditionSlot::TEXT_HEIGHT;
	int size = slot.height();
	float border = (rect.height() / float(size) - 2.0f * pixelWidth() - textHeight_)*0.5f;
	if(size > 1)
		drawText (dc, text_rect, slot.name(), ALIGN_CENTER);
	else
		drawText (dc, text_rect, slot.name(), ALIGN_LEFT);

	drawCircle(dc, not_rect.center(), not_rect.width() * 0.5f, slot.inverted() ? Color4c(255, 0, 0) : Color4c(255, 255, 255), 1);
	if(slot.inverted()){
		SetTextColor(dc, RGB(255, 255, 255));		
		drawText(dc, not_rect, "!", ALIGN_CENTER, true);
		SetTextColor(dc, RGB(0, 0, 0));		
	} 

	ConditionSlot::Slots::const_iterator it;
	int index = 0;
	FOR_EACH(slot.children(), it){
		drawSlot (dc, *it);
		index += it->height();
	}
}

void ConditionViewer::onRedraw(HDC dc)
{
	__super::onRedraw(dc);
	
	RECT rt = { 0, 0, size().x, size().y };

	FillRect(dc, &rt, GetSysColorBrush(COLOR_APPWORKSPACE));

	Win32::StockSelector font(dc, positionFont_);
	SetBkMode(dc, TRANSPARENT);
	rootSlot_.calculateRect(Vect2f::ZERO);
	drawSlot(dc, rootSlot_);

	{
		Win32::StockSelector brush(dc, (HBRUSH)GetStockObject(HOLLOW_BRUSH));
		Win32::StockSelector pen(dc, (HBRUSH)GetStockObject(BLACK_PEN));
		Rectangle(dc, rt.left, rt.top, rt.right, rt.bottom);
	}
}

bool ConditionViewer::canBeDropped(Condition* dest, Condition* source)
{
	if(!dest || dest == source)
		return false;
	ConditionSwitcher* parent = findParent(dest);
	while(parent){
		if(parent == source)
			return false;
		parent = findParent(parent);
	}
	return true;
}

void ConditionViewer::onSet()
{
	rootSlot_.create(value().condition);
	rootSlot_.calculateRect(Vect2f::ZERO);
	centerOn(rootSlot_.rect().center());
}

void ConditionViewer::onDrop(Condition* condition)
{
	xassert(condition);

	if(!selectedCondition_){
		if(value().condition)
			return;
		value().condition = condition;
	}
	else if(ConditionSwitcher* switcher = dynamic_cast<ConditionSwitcher*>(selectedCondition_))
		switcher->add(condition);
	else{
		ShareHandle<ConditionSwitcher> newParent = new ConditionSwitcher;
		newParent->add(condition);
		newParent->add(selectedCondition_);
		ConditionSwitcher* parent = findParent(selectedCondition_);
		if(parent)
			parent->replace(selectedCondition_, newParent);
		else
			value().condition = newParent;
	}
	rootSlot_.create(value().condition);
	redraw();
}

void ConditionViewer::trackMouse(const Vect2f& point, bool dragOver)
{
	selectedCondition_ = rootSlot_.findCondition(point);
	draging_ = dragOver;
	redraw();
}

Condition* ConditionViewer::conditionUnderMouse()
{
	return rootSlot_.findCondition(mousePosition());
}


// ---------------------------------------------------------------------------

ConditionSlot::ConditionSlot()
: color_(1.0f, 0.0f, 1.0f, 1.0f)
, condition_(0)
{
}

ConditionSlot* ConditionSlot::findSlot(Condition* condition)
{
	if(condition_ == condition)
		return this;

	Slots::iterator it;
	FOR_EACH(children_, it){
		ConditionSlot* result = it->findSlot(condition);
		if(result)
			return result;
	}
	return 0;
}

Condition* ConditionSlot::findCondition(const Vect2f& point)
{
	Slots::iterator it;
	FOR_EACH(children_, it){
		Condition* result = it->findCondition(point);
		if(result)
			return result;
	}
	
	if(rect().point_inside(point))
		return condition_;
	
	return 0;
}

void ConditionSlot::create(Condition* condition, int offset, ConditionSlot* parent)
{
    condition_ = condition;
    offset_ = offset;
	parent_ = parent;
	children_.clear();

	if(condition){
		if(ConditionSwitcher* switcher = dynamic_cast<ConditionSwitcher*>(condition)){
			ConditionSwitcher::Conditions::iterator it;
			FOR_EACH(switcher->conditions, it){
				if(Condition* cond = *it){
					children_.push_back(ConditionSlot());
					children_.back().create(cond, offset + 1, this);
				}
			}
			name_ = TRANSLATE(getEnumNameAlt(switcher->type));
		}
		else{
			name_ = conditionName(condition).c_str();
		}
	}
	else{
		name_ = TRANSLATE("Выполняется всегда");
	}	
} 


float ConditionSlot::SLOT_HEIGHT = 25.0f;
float ConditionSlot::TEXT_HEIGHT = 12.0f;

int ConditionSlot::height() const
{
    int height = 0;
    Slots::const_iterator it;
    FOR_EACH (children_, it) {
        height += it->height();
    }
    return height ? height : 1;
}

int ConditionSlot::depth() const
{
	int depth = 0;
	ConditionSlot::Slots::const_iterator it;
	FOR_EACH (children_, it) {
        depth = max(depth, it->depth() + 1);
	}
	return depth;
}

void ConditionSlot::calculateRect(const Vect2f& point, int parent_depth)
{
	int size = height();
	float height = float(size) * SLOT_HEIGHT;
	float border = (SLOT_HEIGHT - TEXT_HEIGHT) * 0.5f;

	if(parent_depth == 0)
		parent_depth = depth();

	Color4c border_color(0, 0, 100, 255);
	rect_.set(point.x, point.y, 300.0f + (parent_depth) * SLOT_HEIGHT, height);

	ConditionSlot::Slots::iterator it;
	int index = 0;
	FOR_EACH (children_, it) {
		Vect2f offset (SLOT_HEIGHT, float(index) * SLOT_HEIGHT);
		it->calculateRect(point + offset, parent_depth - 1);
		index += it->height();
	}

	float text_offset = border * 2.0f + TEXT_HEIGHT;
	if (index > 1) { // switcher
		text_rect_.set (rect_.left(), rect_.top() + text_offset, 
			            SLOT_HEIGHT, rect_.height() - text_offset - border);
	} else {
		text_rect_.set (rect_.left() + text_offset, rect_.top(),
			            rect_.width()  - text_offset, rect_.height());
	}
	not_rect_.set(rect_.left() + border, rect_.top() + border, TEXT_HEIGHT, TEXT_HEIGHT);
}

void ConditionSlot::invert()
{
	if(condition_) {
		condition_->setInverted(!condition_->inverted());
	}
}

#pragma warning(push)
#pragma warning(disable: 4355)

ConditionEditorPlug::ConditionEditorPlug()
: editor_(new ConditionEditor(this))
{
}

#pragma warning(pop)

void ConditionEditorPlug::onSet()
{
	editor_->onSet();
}

kdw::Widget* ConditionEditorPlug::asWidget()
{
	return editor_;
}
