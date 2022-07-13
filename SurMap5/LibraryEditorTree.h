#ifndef __UE_TREE_CTRL_H_INCLUDED__
#define __UE_TREE_CTRL_H_INCLUDED__

#include "../Util/MFC/ObjectsTreeCtrl.h"
#include "Functor.h"

class CAttribEditorCtrl;
class CUnitEditorDlg;
class PopupMenu;
class EditorLibraryInterface;
class Serializeable;
class CLibraryEditorWindow;
class TreeNode;


class LibraryTabBase;
// --------------------------------------------------------------------------
class CLibraryEditorTree : public CObjectsTreeCtrl
{
	DECLARE_DYNAMIC(CLibraryEditorTree)
    friend class LibraryElementTreeObject;
    friend class LibraryBookmarkTreeObject;
public:
	CLibraryEditorTree(CAttribEditorCtrl* attribEditor);
    virtual ~CLibraryEditorTree();

	CAttribEditorCtrl* attribEditor() { return attribEditor_; };

	void spawnMenuAtObject(TreeObject* object);
	PopupMenu& popupMenu() { return *popupMenu_; };

	LibraryTabBase* tab();

    // signals:
    Functor2<void, const char*, Serializeable&>& signalElementSelected(){ return signalElementSelected_; }
protected:
	afx_msg int OnCreate(LPCREATESTRUCT createStruct);  
    afx_msg void OnSize(UINT nType, int cx, int cy);
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    // signals:
    Functor2<void, const char*, Serializeable&> signalElementSelected_;

	DECLARE_MESSAGE_MAP()
private:
	PtrHandle<PopupMenu> popupMenu_;

	ItemType draggedItem_;

    CAttribEditorCtrl* attribEditor_;
};

#endif
