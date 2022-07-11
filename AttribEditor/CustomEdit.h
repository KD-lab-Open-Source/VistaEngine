#ifndef __CUSTOM_EDIT_H_INCLUDED__
#define __CUSTOM_EDIT_H_INCLUDED__

class CTreeListCtrl;

class CCustomEdit : public CEdit
{
public:
	CCustomEdit();

public:
	virtual ~CCustomEdit();

	void SetWindowText(LPCTSTR lpszString);
protected:
	
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChange();


	DECLARE_MESSAGE_MAP();
	void adjustControlHeight(LPCTSTR lpszStr);
	int getCtrlHeight(LPCTSTR lpszStr, int charHeight, int cellHeight);
private:
	CTreeListCtrl* m_ptrTreeCtrl;
	int textHeight_;
	int minHeight_;
};

#endif
