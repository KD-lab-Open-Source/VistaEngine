#ifndef __ATTRIB_COMBO_BOX_H_INCLUDED__
#define __ATTRIB_COMBO_BOX_H_INCLUDED__

class CTreeListCtrl;
class CAttribComboBox : public CComboBox
{
	DECLARE_MESSAGE_MAP()
public:
	~CAttribComboBox();

	afx_msg void OnCloseUp();
	afx_msg void OnSelEndOk();
	afx_msg void OnSelEndCancel();
	afx_msg void OnSetFocus();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
protected:
	CTreeListCtrl& treeListCtrl();

	virtual void OnSelChange ();
};

#endif
