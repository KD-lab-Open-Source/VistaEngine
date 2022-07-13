#ifndef __CONDITION_LIST_H_INCLUDED__
#define __CONDITION_LIST_H_INCLUDED__

#include "..\Util\MFC\TreeListCtrl.h"
#include "IConditionDroppable.h"

class CConditionEditor;

class CConditionList : public CTreeListCtrl {
	DECLARE_MESSAGE_MAP()
public:
	CConditionList (IConditionDroppable* condition_editor);

    void initControl ();
	CConditionEditor* getEditorByLocalPoint (const CPoint& point);
    IConditionDroppable* condition_editor_;
private:
    afx_msg void OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult);

	bool draging_;
public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};

#endif
