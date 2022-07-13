#ifndef __LISTVIEWCTRLEX_H__
#define __LISTVIEWCTRLEX_H__
/******************************************************************************

$Author$
  
$Modtime$
$Revision$

Description: Interfaces of the classes "CListCtrlEx" and "CListViewEx"
             (list control and list view with sort icons and
              colored sort column)

$Log$

******************************************************************************/

/*** Declaration of "workhorse" class "CListBase" ****************************/
class CListBase
{
  public:
  CListBase(): m_pListCtrl       (0),
               m_bSortIconEnabled(false),
               m_bColorSortColumn(false),
               m_nSortColumn     (1),
               m_bKeepLabelLeft  (false),
               m_bLocked         (false),
               m_bControl        (false)
  {
    CreateSortIcons();
  }

  void         ColorSortColumn(bool bEnable = true);
  void         DrawItem       (LPDRAWITEMSTRUCT lpDrawItemStruct);
  virtual void DrawSmallIcon  (CDC* pDC, LVITEM* pItem, LPRECT pRect);
  virtual void DrawStateIcon  (CDC* pDC, LVITEM* pItem, LPRECT pRect);
  virtual void DrawSubItemText(CDC* pDC, LVITEM* pItem, LVCOLUMN* pColumn,
                               LPRECT pRect);
  void         EnableSortIcon (bool bEnable = true, int nSortColumn = 1);
  int          IndexToOrder   (int nIndex);
  bool         KeepLabelLeft  (bool bKeepLeft = true);
  void         SetSortIcon    ();

	private:
  friend class CListCtrlEx;
  friend class CListViewEx;

  void        CreateSortIcons      ();
  int         GetLabelWidth        (CDC* pDC, LVITEM* pItem, int nMaxWidth)
    const;
  LVITEM*     GetLVITEM            (int nItem, int nSubItem = 0) const;
  bool        GetRealSubItemRect   (int iItem, int iSubItem, int nArea,
                                    CRect& ref);
  bool        GetStateIconRect     (int nItem, LPRECT pRect);
  int         HitTest              (CPoint pt, UINT* pFlags = 0);
  int         InsertColumn         (int nCol, const LVCOLUMN* pColumn);
  int         InsertColumn         (int nCol, LPCTSTR lpszColumnHeading,
                                    int nFormat, int nWidth, int nSubItem);
  void        InvalidateNonItemArea();
  void        JustifyFirstColumn   (int nFormat);
  void        MakeShortString      (CDC* pDC, CString& strText, int nColumnLen)
    const;
	BOOL        OnColumnclick        (NMHDR* pNMHDR, LRESULT* pResult);
	void        OnCustomDraw         (NMHDR* pNMHDR, LRESULT* pResult);
	bool        OnEraseBkgnd         (CDC* pDC);
	bool        OnKeyDown            (UINT nChar);
	void        OnKeyUp              (UINT nChar);
	void        OnKillFocus          ();
	bool        OnLButtonDblClk      (CPoint point);
	bool        OnLButtonDown        (CPoint point);
	bool        OnNotify             (LPARAM lParam);
	void        OnSetFocus           ();
	void        OnSysColorChange     ();
  int         OrderToIndex         (int nOrder);
  CImageList* SetImageList         (CImageList* pImageList, int nImageList);

  static const int m_nFirstColXOff;
  static const int m_nNextColXOff;
  static const int m_nIconXOff;
  CListCtrlEx*     m_pListCtrl;
  bool             m_bSortIconEnabled;
  bool             m_bColorSortColumn;
  CImageList       m_imglstSortIcons;
  CBitmap          m_bmpUpArrow;
  CBitmap          m_bmpDownArrow;
  int              m_nUpArrow;
  int              m_nDownArrow;
  DWORD            m_dwColSortColor;
  int              m_nSortColumn;
  int              m_nFormatOfSubItem0;
  bool             m_bKeepLabelLeft;
  bool             m_bLocked;
  bool             m_bControl;
};


/*** Declaration of class "CListCtrlEx" **************************************/
class CListCtrlEx: public CListCtrl, public CListBase
{
  DECLARE_DYNCREATE(CListCtrlEx);

  public:
  CListCtrlEx() {m_pListCtrl = this;}

  virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
  {
    CListBase::DrawItem(lpDrawItemStruct);
  }

  int HitTest(CPoint pt, UINT* pFlags = 0)
  {
    return CListBase::HitTest(pt, pFlags);
  }

  int HitTest(LVHITTESTINFO* pHitTestInfo)
  {
    return CListBase::HitTest(pHitTestInfo->pt, &pHitTestInfo->flags);
  }

  int InsertColumn(int nCol, const LVCOLUMN* pColumn)
  {
    return CListBase::InsertColumn(nCol, pColumn);
  }

  int InsertColumn(int nCol, LPCTSTR lpszColumnHeading,
                   int nFormat = LVCFMT_LEFT, int nWidth = -1,
                   int nSubItem = -1)
  {
    return CListBase::InsertColumn(nCol, lpszColumnHeading, nFormat, nWidth,
                                   nSubItem);
  }

  CImageList* SetImageList(CImageList* pImageList, int nImageList)
  {
    return CListBase::SetImageList(pImageList, nImageList);
  }

  // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListViewCtrl)
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

	// Generated message map functions
  protected:
	//{{AFX_MSG(CListCtrlEx)
	afx_msg void OnSysColorChange();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg BOOL OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCustomDraw (NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};


/*** Declaration of class "CListViewEx" **************************************/
class CListViewEx: public CListView, public CListBase
{
  DECLARE_DYNCREATE(CListViewEx);

  public:
  CListViewEx() {m_pListCtrl = static_cast<CListCtrlEx*>(&GetListCtrl());}

  virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
  {
    CListBase::DrawItem(lpDrawItemStruct);
  }

  int HitTest(CPoint pt, UINT* pFlags = 0)
  {
    return CListBase::HitTest(pt, pFlags);
  }

  int HitTest(LVHITTESTINFO* pHitTestInfo)
  {
    return CListBase::HitTest(pHitTestInfo->pt, &pHitTestInfo->flags);
  }

  int InsertColumn(int nCol, const LVCOLUMN* pColumn)
  {
    return CListBase::InsertColumn(nCol, pColumn);
  }

  int InsertColumn(int nCol, LPCTSTR lpszColumnHeading,
                   int nFormat = LVCFMT_LEFT, int nWidth = -1,
                   int nSubItem = -1)
  {
    return CListBase::InsertColumn(nCol, lpszColumnHeading, nFormat, nWidth,
                                   nSubItem);
  }

  CImageList* SetImageList(CImageList* pImageList, int nImageList)
  {
    return CListBase::SetImageList(pImageList, nImageList);
  }

  // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListViewCtrl)
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

	// Generated message map functions
  protected:
	//{{AFX_MSG(CListCtrlEx)
	afx_msg void OnSysColorChange();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg BOOL OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCustomDraw (NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};

#endif //__LISTVIEWCTRLEX_H__
