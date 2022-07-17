#ifndef __MAIN_FRM_H_INCLUDED__
#define __MAIN_FRM_H_INCLUDED__

#include "SynchroTimer.h"

#include "..\render\inc\fps.h"
#include "EventListeners.h"
#include "ExtStatusBarProgressCtrl.h"

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


class CTimeSliderDlg;
class CObjectsManagerWindow;
class CWaveDlg;
class CToolsTreeWindow;
class CGeneralView;
class CMiniMapWindow;

class CMainFrame : public CFrameWnd, public EventMaster, SelectionChangedListener
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

	CToolsTreeWindow& getToolWindow() { return *toolsWindow_; }
	CExtToolControlBar& toolBar() { return toolBar_; }
	CExtStatusControlBar& statusBar() { return statusBar_; }
	CExtStatusBarProgressCtrl& progressBar() { return progressBar_; }
	CWaveDlg& waveDialog() { return *waveDialog_; }
	CMiniMapWindow& miniMapWindow() { return *miniMap_; }
	CObjectsManagerWindow& objectsManager() { return *objectsManager_; }
	CTimeSliderDlg& timeSliderDialog() { return *timeSliderDialog_; }

	CGeneralView& view() { return *view_; }

	bool reloadMenu();

	void resetWorkspace();

	bool universeQuant();
	void put2TitleNameDirWorld(void);
	void editLibrary(const char* libraryName = "");
	void save(const char* worldName, bool flag_useTerToolColor=true);
	void onSelectionChanged();

	static UINT toolBarID() {
#ifdef _VISTA_ENGINE_EXTERNAL_
		return IDR_MAINFRAME_EXTERNAL;
#else
		return IDR_MAINFRAME;
#endif
	}
protected:
	bool saveDlgBarState();
	bool loadDlgBarState();

	// control bar embedded members
	WINDOWPLACEMENT m_dataFrameWP; // window placement persistence

	CWaveDlg* waveDialog_;
	CMiniMapWindow*   miniMap_;
	CToolsTreeWindow* toolsWindow_;
	CObjectsManagerWindow* objectsManager_;
    CTimeSliderDlg* timeSliderDialog_;

	CExtMenuControlBar        menuBar_;
	CExtToolControlBar        toolBar_;
	CExtStatusControlBar      statusBar_;
	CExtStatusBarProgressCtrl progressBar_;
	
	CExtToolControlBar librariesBar_;
	CExtToolControlBar editorsBar_;
	CExtToolControlBar filtersBar_;

	//CExtPanelControlBar timeSliderBar_;

	CExtControlBar   toolsWindowBar_;
	CExtControlBar   objectsManagerBar_;
	CExtControlBar   propertiesBar_;
	CExtControlBar   miniMapBar_;

	CComboBox m_wndCombo;

	SyncroTimer syncroTimer;
	time_type lastTime;
	StatisticsAttr statisticsAttr;

	CGeneralView*      view_;

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL DestroyWindow();
	virtual void ActivateFrame(int nCmdShow = -1);
	virtual BOOL PreTranslateMessage(MSG* pMsg);


	afx_msg void OnUpdateIsWorldLoaded(CCmdUI *pCmdUI);

	// File menu
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveas();

    afx_msg void OnFileMerge();

	afx_msg void OnFileSaveminimaptoworld();

	afx_msg void OnFileRunWorld();
	afx_msg void OnFileRunMenu();
	afx_msg void OnFileProperties();
	afx_msg void OnFileExportVistaEngine();

	afx_msg void OnFileSavewithouttertoolcolor();
	afx_msg void OnFileSaveVoiceFileDurations();
	afx_msg void OnFileResaveWorlds();
	afx_msg void OnFileResaveTriggers();

	// Edit menu
	afx_msg void OnEditMapScenario();
	afx_msg void OnEditRebuildworld();
	afx_msg void OnEditUpdateSurface();
	afx_msg void OnEditPreferences();
	afx_msg void OnEditSaveCameraAsDefault();

	afx_msg void OnEditTriggers();
	afx_msg void OnEditUnits();

	afx_msg void OnUpdateViewToolbar(CCmdUI *pCmdUI);
	afx_msg void OnViewToolbar();

	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
	afx_msg void OnEditPlaypmo();
	afx_msg void OnViewExtendedmodetreelbar();
	afx_msg void OnUpdateViewExtendedmodetreelbar(CCmdUI *pCmdUI);
	afx_msg void OnDebugSaveconfig();
	afx_msg void OnDebugEditZipConfig();
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

    afx_msg void OnUpdateTimeSlider(CCmdUI* pCmdUI);

	afx_msg BOOL OnBarCheck(UINT nID);
	afx_msg void OnUpdateControlBarMenu(CCmdUI* pCmdUI);
	afx_msg void OnViewResettoolbar2default();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateViewSources(CCmdUI *pCmdUI);
	

	afx_msg void OnEditGameScenario();

	afx_msg void OnEditEffects();
	afx_msg void OnEditTertools();
	afx_msg void OnViewSettings();
	afx_msg void OnDebugSaveDictionary();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnEditToolzers();
	afx_msg void OnEditSounds();
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
	afx_msg void OnUpdateViewCameras(CCmdUI *pCmdUI);

	afx_msg void OnViewSources();
	afx_msg void OnViewCameras();
	afx_msg void OnViewGeosurface();
	afx_msg void OnViewHideModels();
	afx_msg void OnViewPathFinding();
	afx_msg void OnUpdateViewPathFinding(CCmdUI* cmdUI);
	afx_msg void OnViewPathFindingReferenceUnit();

	afx_msg void OnUpdateViewHideModels(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewGeosurface(CCmdUI *pCmdUI);
	afx_msg void OnViewShowGrid();
	afx_msg void OnUpdateViewShowGrid(CCmdUI *pCmdUI);

	afx_msg void OnViewEnableTimeFlow();
	afx_msg void OnUpdateViewEnableTimeFlow(CCmdUI *pCmdUI);

	afx_msg void OnEditEffectsEditor();
	afx_msg void OnEditCursors();
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
	afx_msg void OnFileStatistics();
			void reloadLocStrings();
	afx_msg void OnDebugUISpriteLib();
	afx_msg void OnEditUITextSprites();

	LRESULT OnConstructPopupMenu(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg void OnLibrariesUIMessageTypes();
	afx_msg void OnLibrariesUIMessages();

	afx_msg void OnImportParameters();
	afx_msg void OnExportParameters();

	afx_msg void OnHelpKeyInfo();

	afx_msg void OnFileImportTextFromExcel();
	afx_msg void OnFileExportTextToExcel();

	afx_msg void OnFileParametersImportByGroups();
	afx_msg void OnFileParametersImportUnits();
	afx_msg void OnFileParametersImportFull();

	afx_msg void OnFileParametersExportByGroups();
	afx_msg void OnFileParametersExportUnits();
	afx_msg void OnFileParametersExportFull();
	afx_msg void OnFileParametersExportStatistics();
	afx_msg void OnFileUpdateQuickStartWorldsList();
};

CMainFrame& mainFrame();

void exportParametersByGroups();
void importParametersByGroups();

#endif
