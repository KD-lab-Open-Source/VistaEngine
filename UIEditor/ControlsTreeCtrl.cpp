#include "StdAfx.h"
#include "ControlUtils.h"
#include ".\ControlsTreeCtrl.h"

#include "UIEditor.h"
#include "EditorView.h"
#include "MainFrame.h"

#include "SelectionManager.h"
#include "UserInterface\UserInterface.h"
#include "UserInterface\UI_Controls.h"

#include "UITreeObjects.h"

#include "Serialization\Dictionary.h"
#include "Serialization\SerializationFactory.h"
#include "mfc\PopupMenu.h"
#include "UITreeObjects.h"
#include "kdw/ImageStore.h"
#include "kdw/TreeView.h"


int getControlImage(const std::type_info& info, bool visibleInEditor)
{
	const static std::type_info* types[] = {
		&typeid(UI_Screen),
		&typeid(UI_ControlButton),
		&typeid(UI_ControlCustom),
		&typeid(UI_ControlSlider),
		&typeid(UI_ControlProgressBar),
		&typeid(UI_ControlTab),
		&typeid(UI_ControlStringList),
		&typeid(UI_ControlEdit),
		&typeid(UI_ControlState),
		&typeid(UI_ControlUnitList),
		&typeid(UI_ControlTextList),
		&typeid(UI_ControlVideo),
		&typeid(UI_ControlStringCheckedList),
		&typeid(UI_ControlComboList),
		&typeid(UI_ControlHotKeyInput),
		&typeid(UI_ControlInventory)
	};

	const int count = sizeof(types) / sizeof(types[0]);

    for(int i = 0; i < count; ++i){
        if(*types[i] == info){
			return i + 1 + (!visibleInEditor ? count + 1: 0);
        }
    }
    return 0;
}

ControlsTree::ControlsTree()
{
	UITreeRoot* root = new UITreeRoot();
	model()->setRoot(root);

	imageStore_ = new kdw::ImageStore(16, 16, "CONTROLS", RGB(255, 0, 255));
	setImageStore(imageStore_);
}

bool ControlsTree::onRowLMBDown(kdw::TreeRow* row, const Recti& rowRect, Vect2i point)
{
	kdw::StringTreeColumnDrawer* drawer = safe_cast<kdw::StringTreeColumnDrawer*>(this->columns()[0].drawer());
	Recti imageRect;
	drawer->getSubRectImage(row, rowRect, imageRect);
	if(imageRect.point_inside(point)){
		UITreeObjectControl* object = dynamic_cast<UITreeObjectControl*>(row);
		if(object){
			object->control()->setVisibleInEditor(!object->control()->visibleInEditor());
			object->updateText();
			update();
		}        
		return false;
	}
	else
		return __super::onRowLMBDown(row, rowRect, point);    
}

UITreeRoot* ControlsTree::root()
{
	
	return safe_cast<UITreeRoot*>(__super::root());
}

const UITreeRoot* ControlsTree::root() const
{
	return safe_cast<const UITreeRoot*>(__super::root());
}

// ---------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CControlsTreeCtrl, CObjectsTreeCtrl)
END_MESSAGE_MAP()

CControlsTreeCtrl::CControlsTreeCtrl()
: CObjectsTreeCtrl(new ControlsTree)
, drop_target_(0)
, dragged_control_(0)
{
}

CControlsTreeCtrl::~CControlsTreeCtrl()
{

}

void CControlsTreeCtrl::buildTree () 
{
	tree()->root()->rebuild();
	tree()->update();
}

void CControlsTreeCtrl::buildContainerSubtree(UI_ControlContainer& container)
{
	UITreeObject* object = tree()->root()->findByControlContainer(&container);
	if(object){
		object->rebuild();
		tree()->update();
	}
}


void CControlsTreeCtrl::updateSelected()
{
	kdw::TreeSelection selection = tree()->model()->selection();
	kdw::TreeSelection::iterator it;
	FOR_EACH(selection, it){
		UITreeObject* object = safe_cast<UITreeObject*>(tree()->model()->rowFromPath(*it));
		object->updateText();
	}
}

void CControlsTreeCtrl::selectState(UI_ControlBase* control, int _stateIndex)
{
	UITreeObject* object = tree()->root()->findByControlContainer(control);
	if(UITreeObjectControlState* state = tree()->root()->stateByIndex(_stateIndex))
		tree()->selectObject(state);
}

void CControlsTreeCtrl::selectScreen(UI_Screen* screen)
{
	if(UITreeObject* object = tree()->root()->findByControlContainer(screen)){
		tree()->selectObject(object);
	}
	else
		xassert(0);
}

bool CControlsTreeCtrl::selectControl(UI_ControlBase* control)
{
	if(UITreeObject* object = tree()->root()->findByControlContainer(control)){
		tree()->selectObject(object);
		return true;
	}
	xassert(0);
	return false;
}

void CControlsTreeCtrl::selectControlInScreen (UI_ControlBase* control, UI_Screen* screen)
{
	selectControl(control);
}


// --------------------------------------------------------------------------------------------
#include "EditorAction.h"

ControlsTree* EditorAction::controlsTree()
{
	xassert(uiEditorFrame() && uiEditorFrame()->GetControlsTree());
	ControlsTree* tree = uiEditorFrame()->GetControlsTree()->tree();
	xassert(tree);
	return tree;
}
