#ifndef __T_E_FRAME_H_INCLUDED__
#define __T_E_FRAME_H_INCLUDED__

#include <Prof-UIS.h>

/////////////////////////////////////////////////////////////////////////////
// TEFrame frame

class TETreeManager;
class TEMiniMapManager;
class ScaleMgr;
class TriggerEditorView;
class TriggerProfList;
class TriggerEditor;

#include <boost/scoped_ptr.hpp>

class TEFrame : public CFrameWnd
{
	DECLARE_DYNCREATE(TEFrame)
protected:

// Attributes
public:
	TEFrame();           
	TEFrame(TriggerEditor* triggerEditor);           
	virtual ~TEFrame();

// Operations
public:
	void setParent(HWND hparent);
	HWND getParent() const;

	void setTETreeManager(TETreeManager* mngr);
	TETreeManager* getTETreeManager() const;

	TriggerEditorView* addTriggerEditorView();

	TriggerProfList* getTriggerProfList() const;
	//! \brief Для установки размеров окна view. Вызывается один раз в начале, 
	//! чтобы сделать это когда нужно мне
	void updateViewSize();

	void resetWorkspace();
	bool loadWorkspace();
	bool saveWorkspace();
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TEFrame)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, 
							void* pExtra, 
							AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual void PostNcDestroy();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void ActivateFrame(int nCmdShow = -1);
	afx_msg LONG onFindReplace(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(TEFrame)
	afx_msg void OnClose();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnFileSave();
	afx_msg void onUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void onViewProfiler();
	afx_msg void onUpdateViewProfiler(CCmdUI* pCmdUI);
	afx_msg void onViewToolbar();
	afx_msg void onUpdateViewToolbar(CCmdUI* pCmdUI);
	afx_msg void onViewTree();
	afx_msg void onUpdateViewTree(CCmdUI* pCmdUI);
	afx_msg void onViewMiniMap();
	afx_msg void onUpdateViewMiniMap(CCmdUI* pCmdUI);
	afx_msg void onUpdateEditFind(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();

	//! После создания выполняет настройки необъодимые для корректной работы дерева
	bool initTETreeManager();
private:
	CExtToolControlBar mainToolBar_;

	//! Окно, поверх которого открывается окно редактора триггеров
	HWND hParentWnd_;
	//! Дерево редактора триггеров
	TETreeManager* treeManager_;
	//! Управление масштабом
	boost::scoped_ptr<ScaleMgr> scaleMgr_;
	//! Миникарта триггеров
    boost::scoped_ptr<TEMiniMapManager> miniMapManager_;
	//! Статус бар
	CStatusBar wndStatusBar_;
	//! Окно, в котором отображается поле редактора триггеров
	TriggerEditorView* triggerEditorView_;


	TriggerEditor* triggerEditor_;

	WINDOWPLACEMENT windowPlacement_;

	//! Отладчик, триггеров
	boost::scoped_ptr<TriggerProfList> triggerProfList_;
public:
	afx_msg void OnFileProperties();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif
