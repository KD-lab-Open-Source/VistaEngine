#ifndef __LIBRARY_ATTRIB_EDITOR_H_INCLUDED__
#define __LIBRARY_ATTRIB_EDITOR_H_INCLUDED__

#include "AttribEditor\AttribEditorCtrl.h"
#include "XTL\Functor.h"

#include "Serialization/LibraryBookmark.h"

class TreeNode;
class PopupMenuItem;

class CLibraryAttribEditor : public CAttribEditorCtrl{
public:
	CLibraryAttribEditor();

	
    Functor1<void, const ItemType>& signalSearchTreeNode() { return signalSearchInvoked_; }

    Functor1<void, LibraryBookmark>& signalFollowReference() { return signalFollowReference_; }
	Functor1<void, const ComboStrings&>& signalElementSelected() { return signalElementSelected_; }
	Functor0<void>& signalElementChanged() { return signalElementChanged_; }
protected:
	ComboStrings selectedPath_;
	int initControl();

	void onSelected();
	void onChanged();
	//void onFollowReference(ItemType item);
	void onFollowReference(LibraryBookmark bookmark);
	void onSearch(ItemType item);

    Functor1<void, const ItemType> signalSearchInvoked_;
    Functor1<void, LibraryBookmark> signalFollowReference_;
    Functor1<void, const ComboStrings&> signalElementSelected_;
    Functor0<void> signalElementChanged_;
};

#endif