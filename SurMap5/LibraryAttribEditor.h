#ifndef __LIBRARY_ATTRIB_EDITOR_H_INCLUDED__
#define __LIBRARY_ATTRIB_EDITOR_H_INCLUDED__

#include "..\AttribEditor\AttribEditorCtrl.h"
#include "Functor.h"

#include "LibraryBookmark.h"

class TreeNode;
class PopupMenuItem;

class CLibraryAttribEditor : public CAttribEditorCtrl{
public:
    Functor1<void, const TreeNode*>& signalSearchTreeNode() { return signalSearchInvoked_; }

    Functor1<void, LibraryBookmark>& signalFollowReference() { return signalFollowReference_; }
	Functor1<void, const ComboStrings&>& signalElementSelected() { return signalElementSelected_; }
	Functor1<void, const TreeNode*>& signalElementChanged() { return signalElementChanged_; }
protected:
	ComboStrings selectedPath_;

    // virtuals:
	bool beforeElementEdit(CLibraryAttribEditor::ItemType item, bool middleButton);
	void onMenuConstruction(CLibraryAttribEditor::ItemType item, PopupMenuItem& menuRoot);
	void onElementSelected();
	void onElementChanged(ItemType item);

    Functor1<void, const TreeNode*> signalSearchInvoked_;
    Functor1<void, LibraryBookmark> signalFollowReference_;
    Functor1<void, const ComboStrings&> signalElementSelected_;
    Functor1<void, const TreeNode*> signalElementChanged_;
};

#endif