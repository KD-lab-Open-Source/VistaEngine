#ifndef __SURMAP5_UI_EDITOR_SELECTION_MANAGER__
#define __SURMAP5_UI_EDITOR_SELECTION_MANAGER__

#include "Handle.h"
#include "Selection.h"
#include "RectEditor.h"
class UI_ControlBase;
class PositionChangeAction;

class SelectionManager : public RectEditor
{
public:
    static SelectionManager& the();
    ~SelectionManager();

    void deselectAll();
    void select(UI_ControlBase* _control);
    void select(const Selection& _selection);
    void toggleSelection(UI_ControlBase* _control);
    bool isSelected(UI_ControlBase* _control) const;

    Selection& selection (){ return selection_; }
protected:
    SelectionManager();

	virtual void onClick(const Vect2f& point) {}

	virtual Rectf getRect(int index) const;
	virtual int getRectCount() const;
	virtual void setRect(int index, const Rectf& rect);
    virtual bool onStartMoving(const Vect2f& point);
    virtual void onMove(const Vect2f& delta);
    virtual bool onStartScaling(const Vect2f& point, const Vect2i& corner);
	virtual void onScale(const Vect2f& delta);
protected:
    ShareHandle<PositionChangeAction> move_action_;
    ShareHandle<PositionChangeAction> scale_action_;
    Selection selection_;
};

#endif
