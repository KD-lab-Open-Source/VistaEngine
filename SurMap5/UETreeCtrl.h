#ifndef __UE_TREE_CTRL_H_INCLUDED__
#define __UE_TREE_CTRL_H_INCLUDED__

#include "../Util/MFC/ObjectsTreeCtrl.h"
#include "UnitEditorLogicBase.h"
#include "BookmarkBase.h"

class CAttribEditorCtrl;
class CUETreeCtrl;
class CUnitEditorDlg;
class PopupMenu;
class CUETreeCtrl : public CObjectsTreeCtrl
{
	DECLARE_DYNAMIC(CUETreeCtrl)
public:
	typedef std::list< ShareHandle<BookmarkBase> > Bookmarks;

	CUETreeCtrl(CAttribEditorCtrl& attribEditor);
    virtual ~CUETreeCtrl();
	virtual void rebuildTree() {};

	void beforeElementChanged();
	void afterElementChanged();
	void beforeJumping(CAttribEditorCtrl::ItemType);

	void invokeSearch(const TreeNode& node);

	CAttribEditorCtrl& attribEditor() { return attribEditor_; };

	void setLogic(int logicClassIndex);
	int logicIndex(){ return logicClassIndex_; }
	UnitEditorLogicBase* logic() { return logic_; }
	void setLogicPosition(BookmarkBase* bookmark);

	void goBack();
	void addToHistory(BookmarkBase* bookmark);
	PopupMenu& popupMenu() { return *popupMenu_; };
	Bookmarks& history() { return history_; }
protected:
	afx_msg void OnSelChanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSelChanging(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDoubleClick(NMHDR *pNMHDR, LRESULT *pResult);

    afx_msg void OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDragEnter (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDragLeave (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDragOver (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDrop (NMHDR* pNMHDR, LRESULT* pResult);

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	void setLogic(UnitEditorLogicBase* _logic);
	CUnitEditorDlg& unitEditorDialog();

	Bookmarks history_;
	PopupMenu* popupMenu_;
	ShareHandle<UnitEditorLogicBase> logic_;
	int logicClassIndex_;

	ItemType draggedItem_;
	//ItemType lastSelectedItem_;

	ShareHandle<BookmarkBase> currentPosition_;

    CAttribEditorCtrl& attribEditor_;
};

class BookmarkGeneric: public BookmarkBase{
public:
	BookmarkGeneric(int logicClassIndex, CUETreeCtrl& treeControl, CUETreeCtrl::ItemType item);
	~BookmarkGeneric();
	int logicClassIndex() const;
	void visit(CUETreeCtrl& treeControl);
	void serialize(Archive& ar);
private:
	ComboStrings treePath_;
	ComboStrings attribEditorPath_;
	int logicClassIndex_;
};

#endif
