#ifndef __TREE_COMBO_BOX_H_INCLUDED__
#define __TREE_COMBO_BOX_H_INCLUDED__

class CTreeComboBox;

class CPopupTreeCtrl : public CTreeCtrl {
    DECLARE_DYNAMIC(CPopupTreeCtrl)
public:
    CPopupTreeCtrl();

	static const char* className() { return "VistaEngineParameterSelectorPopup"; }
	void setParent(CTreeComboBox* menu);
private:

	CTreeComboBox* parent_;
//	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSelChanged(NMHDR* nm, LRESULT* result);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};

class CTreeComboBox : public CButton {
	friend CPopupTreeCtrl;
    DECLARE_DYNAMIC(CTreeComboBox)
public:
    CTreeComboBox();

	static const char* className() { return "VistaEngineParameterSelectorMenu"; }

	void showDropDown();
	virtual void hideDropDown(bool byLostFocus = false);
	BOOL Create(DWORD style, const CRect& rect, CWnd* parent, UINT id);
	void registerWindowClass();

	CTreeCtrl& getTree() { return tree_; };
	void setText(const char* text) {
		text_ = text;
	}
    
public:
	virtual void onItemSelected(HTREEITEM item);

	std::string text_;
	CPopupTreeCtrl tree_;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

    void interceptParentWndProc();
    void unInterceptParentWndProc();

	static BOOL IsMsgOK(HWND hWnd, UINT nMsg, /*WPARAM wParam,*/ LPARAM lParam);
	static WNDPROC m_parentWndProc;
	static CTreeComboBox* m_pActiveComboBox;
	static LRESULT CALLBACK ParentWindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
protected:
	virtual void PreSubclassWindow();
public:
	afx_msg void OnDestroy();
};


#endif
