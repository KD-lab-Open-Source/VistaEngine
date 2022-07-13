#ifndef __MAIN_FRM_H_INCLUDED__
#define __MAIN_FRM_H_INCLUDED__

#include "ObjectsManagerWindow.h"
#include "CameraDlg.h"
#include "WaveDlg.h"

#include "GeneralView.h"
#include "ToolsTreeWindow.h"
#include "SToolBar.h"
#include "SynchroTimer.h"

#include "ToolPreviewWindow.h"
#include "MiniMapWindow.h"

#include "..\render\inc\fps.h"

struct StatisticsAttr
{
	bool totalObjects;
	bool uniqueObjects;
	bool totalTextures;
	bool texturesSize;
	bool verticesSize;
	bool vertices;
	bool polygons;
	bool nodes;
	bool materials;
	bool textures;
	bool allObjects;
};

class CExtStatusBarProgressCtrl : public CProgressCtrl
{
	virtual LRESULT WindowProc(    
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam
		)
	{
		if( uMsg == WM_ERASEBKGND )
			return (!0);
		if( uMsg == WM_PAINT )
		{
			CRect rcClient;
			GetClientRect( &rcClient );
			CPaintDC dcPaint( this );
			CExtMemoryDC dc( &dcPaint, &rcClient );
			if( g_PaintManager->GetCb2DbTransparentMode(this) )
			{
				CExtPaintManager::stat_ExcludeChildAreas(
					dc,
					GetSafeHwnd(),
					CExtPaintManager::stat_DefExcludeChildAreaCallback
					);
				g_PaintManager->PaintDockerBkgnd(dc, this );
			} // if( g_PaintManager->GetCb2DbTransparentMode(this) )
			else
				dc.FillSolidRect( &rcClient, g_PaintManager->GetColor( CExtPaintManager::CLR_3DFACE_OUT ) );
			DefWindowProc( WM_PAINT, WPARAM(dc.GetSafeHdc()), 0L );
			g_PaintManager->OnPaintSessionComplete( this );
			return 0;
		}
		if( uMsg == WM_TIMER && IsWindowEnabled()){
			StepIt();
		}
		if( uMsg == WM_DESTROY ){
			KillTimer(0);
		}
		LRESULT lResult = CProgressCtrl::WindowProc(uMsg, wParam, lParam);
		return lResult;
	}
};

class cObjStatistic;
class CMainFrame : public CFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	FPS fps;
	FPS logicFps;
	CExtControlBar* getResizableBarDlg() { return &propertiesBar_; }

	CToolsTreeWindow& getToolWindow() { return toolsWindow_; }
	CSToolBar& toolBar() { return toolBar_; }
	CExtStatusControlBar& statusBar() { return statusBar_; }
	CExtStatusBarProgressCtrl& progressBar() { return progressBar_; }
	CCameraDlg& cameraDialog() { return m_dlgCamera; }
	CWaveDlg& waveDialog() { return m_dlgWave; }
	CMiniMapWindow& miniMapWindow() { return miniMap_; }

	void resetWorkspace();

	bool universeQuant();
	void put2TitleNameDirWorld(void);
	void save(const char* worldName);

	CObjectsManagerWindow& objectsManager() { return objectsManager_; }

	CGeneralView      m_wndView;

protected:
	bool saveDlgBarState();
	bool loadDlgBarState();

	// control bar embedded members
	WINDOWPLACEMENT m_dataFrameWP; // window placement persistence

	CCameraDlg m_dlgCamera;
	CWaveDlg m_dlgWave;

	CMiniMapWindow   miniMap_;
	CToolsTreeWindow toolsWindow_;
	CObjectsManagerWindow objectsManager_;

	CExtMenuControlBar        menuBar_;
	CSToolBar                 toolBar_;
	CExtStatusControlBar      statusBar_;
	CExtStatusBarProgressCtrl progressBar_;
	
	CExtToolControlBar librariesBar_;
	CExtToolControlBar editorsBar_;
	CExtToolControlBar filtersBar_;
	CExtToolControlBar workspaceBar_;

	CExtControlBar   toolsWindowBar_;
	CExtControlBar   objectsManagerBar_;
	CExtControlBar   propertiesBar_;
	CExtControlBar   miniMapBar_;

	CComboBox m_wndCombo;

	SyncroTimer syncroTimer;
	time_type lastTime;
	StatisticsAttr statisticsAttr;

	cObjStatistic* pObjStatistic;
// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL DestroyWindow();
	virtual void ActivateFrame(int nCmdShow = -1);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnUpdateViewToolbar(CCmdUI *pCmdUI);
	afx_msg void OnViewToolbar();

	afx_msg void OnFileOpen();
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
	afx_msg void OnFileSaveas();
	afx_msg void OnEditPlaypmo();
	afx_msg void OnDebugFillIn();
	afx_msg void OnEditMapScenario();
	afx_msg void OnEditTriggers();
	afx_msg void OnEditUnits();
	afx_msg void OnViewExtendedmodetreelbar();
	afx_msg void OnUpdateViewExtendedmodetreelbar(CCmdUI *pCmdUI);
	afx_msg void OnDebugSaveconfig();
	afx_msg void OnFileNew();
	afx_msg void OnFileSave();
	afx_msg void OnEditUnitTree();
	afx_msg void OnEditUserInterface();
	afx_msg void OnEditObjects();
	afx_msg void OnViewTreeBar();
	afx_msg void OnUpdateViewTreeBar(CCmdUI *pCmdUI);
	afx_msg void OnViewProperties();
	afx_msg void OnUpdateViewProperties(CCmdUI *pCmdUI);
	afx_msg void OnViewMinimap();
	afx_msg void OnUpdateViewMinimap(CCmdUI *pCmdUI);
	afx_msg void OnViewObjectsManager();
	afx_msg void OnUpdateViewObjectsManager(CCmdUI *pCmdUI);

	afx_msg BOOL OnBarCheck(UINT nID);
	afx_msg void OnUpdateControlBarMenu(CCmdUI* pCmdUI);
	afx_msg void OnViewResettoolbar2default();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewWaterSource();
	afx_msg void OnUpdateViewWaterSource(CCmdUI *pCmdUI);
	afx_msg void OnViewBubbleSource();
	afx_msg void OnUpdateViewBubbleSource(CCmdUI *pCmdUI);
	afx_msg void OnViewWindSource();
	afx_msg void OnUpdateViewWindSource(CCmdUI *pCmdUI);
	afx_msg void OnViewLightSource();
	afx_msg void OnUpdateViewLightSource(CCmdUI *pCmdUI);
	afx_msg void OnFileSaveminimaptoworld();
	afx_msg void OnFileRunWorld();
	afx_msg void OnEditGameScenario();

	afx_msg void OnViewBreakSource();
	afx_msg void OnUpdateViewBreakSource(CCmdUI *pCmdUI);
	afx_msg void OnEditRebuildworld();
	afx_msg void OnEditEffects();
	afx_msg void OnEditPreferences();
	afx_msg void OnViewSettings();
	afx_msg void OnFileRunMenu();
	afx_msg void OnDebugSaveDictionary();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnEditToolzers();
	afx_msg void OnFileProperties();
	afx_msg void OnEditSounds();
	afx_msg void OnEditVoices();
	afx_msg void OnFileExportVistaEngine();
	afx_msg void OnUpdateEditUndo(CCmdUI *pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditRedo(CCmdUI *pCmdUI);
	afx_msg void OnEditRedo();
	afx_msg void OnDebugShowpalettetexture();
	afx_msg void OnUpdateDebugShowpalettetexture(CCmdUI *pCmdUI);
	afx_msg void OnEditHeads();
	afx_msg void OnEditSoundTracks();
	afx_msg void OnEditReels();
	afx_msg void OnEditGlobalTrigger();
	afx_msg void OnUpdateViewEffects(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewCameras(CCmdUI *pCmdUI);
	afx_msg void OnViewEffects();
	afx_msg void OnViewCameras();
	afx_msg void OnEditEffectsEditor();
	afx_msg void OnEditTertools();
	afx_msg void OnEditCursors();
	afx_msg void OnEditDefaultAnimationChains();
	afx_msg void OnEditFormations();

	afx_msg void OnDebugEditDebugPrm();
	afx_msg void OnDebugEditAuxAttribute();
	afx_msg void OnDebugEditRigidBodyPrm();
	afx_msg void OnDebugEditToolzer();
	afx_msg void OnDebugEditExplodeTable();
	afx_msg void OnDebugEditSourcesLibrary();
	afx_msg void OnDebugShowmipmap();
	afx_msg void OnUpdateDebugShowmipmap(CCmdUI *pCmdUI);
	afx_msg void OnEditChangetotalworldheight();
	afx_msg void OnViewGeosurface();
	afx_msg void OnUpdateViewGeosurface(CCmdUI *pCmdUI);
	afx_msg void OnEditGameOptions();
	afx_msg void OnFileStatistics();
	afx_msg void OnViewShowGrid();
	afx_msg void OnUpdateViewShowGrid(CCmdUI *pCmdUI);
	afx_msg void OnViewEnableTimeFlow();
	afx_msg void OnUpdateViewEnableTimeFlow(CCmdUI *pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI *pCmdUI);
	afx_msg void OnFileImportTextFromExcel();
	afx_msg void OnFileExportTextToExcel();
	afx_msg void OnDebugEditstrategy();
	afx_msg void OnDebugUISpriteLib();

	LRESULT OnConstructPopupMenu(WPARAM wParam, LPARAM lParam);
protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif
