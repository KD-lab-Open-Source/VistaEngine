#include "StdAfx.h"
#include "ActionManager.h"
#include "SelectionManager.h"
#include "SubRectEditor.h"

#include "UserInterface\UI_Types.h"

Rectf SubRectEditor::getRect(int index) const {
    Rectf subRect (SelectionManager::the().selection().front()->getSubRect (index));
    Rectf controlRect (SelectionManager::the().selection().front()->position());
	Rectf rect (controlRect.width()  * subRect.left() + controlRect.left(),
				controlRect.height() * subRect.top()  + controlRect.top(),
				controlRect.width()  * subRect.width(),
				controlRect.height() * subRect.height());
	return rect;
}

void SubRectEditor::setRect (int index, const Rectf& _rect) {
    Rectf controlRect (SelectionManager::the().selection().front()->position());
	Rectf rect ((_rect.left() - controlRect.left())  / controlRect.width()  ,
				(_rect.top() - controlRect.top()) / controlRect.height() ,
				 _rect.width()  / controlRect.width(),
				 _rect.height() / controlRect.height());
	
	if(SelectionManager::the().selection().front()->getSubRect(index) != rect)
		ActionManager::the().setChanged();

    SelectionManager::the().selection().front()->setSubRect (index, rect);
}

bool SubRectEditor::onStartScaling (const Vect2f& pos, const Vect2i& corner) {
    Rectf control_rect (SelectionManager::the().selection().front()->position());
	start_rect_ = control_rect;
	grabbed_corner_ = corner;
    return true;
}

bool SubRectEditor::onStartMoving (const Vect2f& pos) {
    return true;
}

void SubRectEditor::onMove (const Vect2f& delta) {
}

int SubRectEditor::getRectCount () const {
	return SelectionManager::the().selection().front()->getSubRectCount ();
}
void SubRectEditor::onClick (const Vect2f& point) {

}
