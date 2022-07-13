#include "stdafx.h"
#include "TreeInterface.h"
#include "LibraryAttribEditor.h"
#include "..\AttribEditor\TreeListCtrl\xTreeCtrlLib.h"
#include "Dictionary.h"
#include "mfc\PopupMenu.h"

void CLibraryAttribEditor::onMenuConstruction(CLibraryAttribEditor::ItemType item, PopupMenuItem& menuRoot)
{
    TreeNode* node = getNode(item);
	LibraryBookmark bookmark;
    if(node && node->editor() && node->editor()->getLibraryBookmark(&bookmark)){
        menuRoot.add(TRANSLATE("Следовать по ссылке..."))
                .connect(bindArgument(referenceCall(signalFollowReference_), bookmark));
    }
    menuRoot.add(TRANSLATE("Искать в библиотеках..."))
            .connect(bindArgument(referenceCall(signalSearchInvoked_), node));
}

void CLibraryAttribEditor::onElementChanged(ItemType item)
{
	__super::onElementChanged(item);
	const TreeNode* node = getNode(item);
	if(signalElementChanged_)
		signalElementChanged_(node);
}

void CLibraryAttribEditor::onElementSelected()
{
	if(signalElementSelected_){
		ItemType selectedItem = treeControl().GetSelectedItem();
		if(selectedItem){
			this->getItemPath(selectedPath_, selectedItem);
		}
		else{
			selectedPath_.clear();
		}
		signalElementSelected_(selectedPath_);	
	}
}

bool CLibraryAttribEditor::beforeElementEdit(CLibraryAttribEditor::ItemType item, bool middleButton)
{
    bool shift = bool(GetAsyncKeyState(VK_SHIFT) >> 15);
	if(shift || middleButton){
		TreeNode* node = getNode(item);
		xassert(node);
		LibraryBookmark bookmark;
		if(node && node->editor() && node->editor()->getLibraryBookmark(&bookmark))
			signalFollowReference_(bookmark);
		return false;
	}
	return true;
}
