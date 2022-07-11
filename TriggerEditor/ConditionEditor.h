#ifndef __VISTA_EDITOR_CONDITION_EDITOR_H_INCLUDED__
#define __VISTA_EDITOR_CONDITION_EDITOR_H_INCLUDED__

#include "kdw/Viewport2D.h"
#include "kdw/Plug.h"
#include "kdw/HSplitter.h"
#include "EditableCondition.h"

class ClassTree;
struct Condition;
struct ConditionSwitcher;

struct ConditionSlot {
	static float SLOT_HEIGHT;
	static float TEXT_HEIGHT;

	ConditionSlot();

	void create(Condition* condition, int offset = 0, ConditionSlot* parent = 0);

    int height() const;
	int depth() const;
    void calculateRect(const Vect2f& point, int parent_depth = 0);

	void invert();
	bool inverted() const { return condition_ ? condition_->inverted() : false; }

	typedef list<ConditionSlot> Slots;

	ConditionSlot* parent() { return parent_; }
	Condition* condition() { return condition_; }
	const Condition* condition() const{ return condition_; }
	const Rectf& rect() const { return rect_; }
	const Rectf& text_rect() const { return text_rect_; }
	const Rectf& not_rect() const { return not_rect_; }
	const char* name() const { return name_.c_str(); }
	Slots& children() { return children_; }
	const Slots& children() const{ return children_; }

	ConditionSlot* findSlot(Condition* condition);
	Condition* findCondition(const Vect2f& point);

protected:
	string name_;
	Slots children_;
    Color4f color_;
    int offset_;

	Rectf rect_;
	Rectf text_rect_;
	Rectf not_rect_;
	struct Condition* condition_;
	ConditionSlot* parent_;

private:
	void operator=(const ConditionSlot& rhs) {}
};

class ConditionEditor;

class ConditionEditorPlug : public kdw::Plug<EditableCondition, ConditionEditorPlug>{
public:
	ConditionEditorPlug();
    void onSet();    
	kdw::Widget* asWidget();
protected:
	ShareHandle<ConditionEditor> editor_;
};

class ConditionViewer : public kdw::Viewport2D
{
public:
	ConditionViewer(ConditionEditorPlug* owner, kdw::PropertyTree* propertyTree);

	void onMouseMove(const Vect2i& delta);
	void onMouseButtonDown(kdw::MouseButton button);
	void onMouseButtonUp(kdw::MouseButton button);

	void onKeyDown(const sKey& key);

	void onRedraw(HDC dc);
	void trackMouse(const Vect2f& point, bool dragOver = false);

	void onDrop(Condition* condition);

	void onMenuEdit();
	void onMenuCopy();
	void onMenuPaste();
	void onMenuDelete();
	void onSet();
	void onTreeChanged();

private:
	ConditionEditorPlug* plug_;
	kdw::PropertyTree* propertyTree_;

	ConditionSlot rootSlot_;
	Condition* selectedCondition_;
	ShareHandle<Condition> dragedCondition_;
	ShareHandle<Condition> editedCondition_;

	bool moving_;
	bool draging_;
	bool dragged_;

	bool canBeDropped(Condition *dest, Condition* source);
	Condition* conditionUnderMouse();

	void drawSlot(HDC dc, const ConditionSlot& slot);

	ConditionPtr createCondition(int typeIndex);
	ConditionSwitcher* findParent(Condition* condition, ConditionSwitcher* parent = 0);
	void removeCondition(Condition* condition);

	EditableCondition& value() { return plug_->value(); }

	static ShareHandle<Condition> conditionCopy_;
};

class ConditionEditor : public kdw::HSplitter
{
public:
	ConditionEditor(ConditionEditorPlug* owner);

	void onSet();

private:
	ConditionEditorPlug* plug_;
	ConditionViewer* viewer_;
	ClassTree* tree_;

	EditableCondition& value() { return plug_->value(); }
};



#endif
