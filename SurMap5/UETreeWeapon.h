#pragma once

#include "UETreeBase.h"

class CAttribEditorCtrl;

class CUETreeWeapon : public CUETreeBase
{
	DECLARE_DYNAMIC(CUETreeWeapon)
public:
	CUETreeWeapon(CAttribEditorCtrl& attribEditor, const CRect& rt, CWnd* parent);
	~CUETreeWeapon();
    void rebuildTree ();
	void onItemSelected(ItemType item);
	void onItemRClick(ItemType item);

	void BuildTypeNode (ItemType _handle);
private:

    ItemType clickedItem_;
    bool pasteByDefault_;
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMenuAdd();
	afx_msg void OnMenuDelete();

	DECLARE_MESSAGE_MAP()
};


