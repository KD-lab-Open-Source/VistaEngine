#ifndef __UI_MAIN_FRAME_H_INCLUDED__
#define __UI_MAIN_FRAME_H_INCLUDED__

class CUIEditorPanel;
class CUIEditorView;
class CControlsTreeCtrl;
class CAttribEditorCtrl;

class CUIMainFrame : public CFrameWnd
{
	friend class CUIEditorApp;

	DECLARE_DYNAMIC(CUIMainFrame)

	CUIMainFrame();           // protected constructor used by dynamic creation
	virtual ~CUIMainFrame();
protected:
    CSplitterWnd m_wndSplitter;
	CToolBar m_wndToolBar;
protected:
	DECLARE_MESSAGE_MAP()
public:
	CUIEditorView& view();
	CUIEditorPanel* GetPanel ();
	CAttribEditorCtrl* GetAttribEditor ();
	CControlsTreeCtrl* GetControlsTree ();

	afx_msg void OnEditTree();
	afx_msg void OnDestroy();
protected:

	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnFileSave();
	afx_msg void OnFileQuickSave();
	afx_msg void OnUpdateFileSave(CCmdUI *pCmdUI);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI *pCmdUI);
	afx_msg void OnEditDelete();
	afx_msg void OnUpdateEditDelete(CCmdUI *pCmdUI);
	afx_msg LRESULT OnCheckItsMe (WPARAM wParam, LPARAM lParam);
	afx_msg void OnEditAddScreen();
	//afx_msg void OnEditDeleteScreen();
	afx_msg void OnEditFontLibrary();
    afx_msg void OnEditParameters();
	afx_msg void OnClose();
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
	afx_msg void OnEditSnapOptions();
	afx_msg void OnViewSnapToGrid();
	afx_msg void OnUpdateViewShowGrid(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewSnapToGrid(CCmdUI *pCmdUI);
	afx_msg void OnViewShowGrid();
	afx_msg void OnViewSnapToBorder();
	afx_msg void OnUpdateViewSnapToBorder(CCmdUI *pCmdUI);
	afx_msg void OnViewShowBorder();
	afx_msg void OnUpdateViewShowBorder(CCmdUI *pCmdUI);
	afx_msg void OnEditToTextureSize();
	afx_msg void OnEditHotKeys();
	afx_msg void OnUpdateViewZoomShowAll(CCmdUI *pCmdUI);
    afx_msg void OnUpdateViewZoomOneToOne(CCmdUI *pCmdUI);
    afx_msg void OnViewZoomShowAll();
    afx_msg void OnViewZoomOneToOne();

	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

#endif
