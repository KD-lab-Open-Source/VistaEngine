#include "stdafx.h"
#include "Resource.h"

#include "UIEditor.h"
#include ".\MainFrame.h"
#include "UIEditorPanel.h"
#include "ControlsTreeCtrl.h"
#include "EditorView.h"
#include "Options.h"

#include "UserInterface\UI_Render.h"
#include "UserInterface\UserInterface.h"
#include "Game\Universe.h"
#include "Render\Src\VisGeneric.h"

#include "kdw/PropertyEditor.h"
#include "Serialization\XPrmArchive.h"
#include "Serialization\Dictionary.h"
#include "Serialization\SerializationFactory.h"
#include "Serialization\StringTableImpl.h"

#include "ActionManager.h"
#include "PositionChangeAction.h"
#include "SelectionManager.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CUIMainFrame
IMPLEMENT_DYNAMIC(CUIMainFrame, CFrameWnd)

CUIMainFrame::CUIMainFrame()
{

}

CUIMainFrame::~CUIMainFrame()
{
}


BEGIN_MESSAGE_MAP(CUIMainFrame, CFrameWnd)
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_WM_KEYDOWN()
	ON_WM_CLOSE()
	ON_WM_ACTIVATEAPP()

	ON_COMMAND(ID_UI_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_UI_FILE_QUICK_SAVE, OnFileQuickSave)
	ON_UPDATE_COMMAND_UI(ID_UI_FILE_SAVE, OnUpdateFileSave)
	ON_COMMAND(ID_UI_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_UI_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_UI_EDIT_DELETE, OnEditDelete)
	ON_UPDATE_COMMAND_UI(ID_UI_EDIT_DELETE, OnUpdateEditDelete)
	ON_REGISTERED_MESSAGE(WM_CHECK_ITS_ME, OnCheckItsMe)
	ON_COMMAND(ID_UI_EDIT_ADD_SCREEN, OnEditAddScreen)
	
	ON_COMMAND(ID_UI_EDIT_FONT_LIBRARY, OnEditFontLibrary)
	ON_COMMAND(ID_UI_EDIT_EQUIPMENT_SLOT_TYPES, OnEditEquipmentSlotTypes)
	ON_COMMAND(ID_UI_EDIT_INVENTORY_CELL_TYPES, OnEditInventoryCellTypes)

	ON_COMMAND(ID_UI_EDIT_PARAMETERS, OnEditParameters)
	ON_COMMAND(ID_UI_EDIT_SNAP_OPTIONS, OnEditSnapOptions)
	ON_COMMAND(ID_UI_VIEW_SNAP_TO_GRID, OnViewSnapToGrid)
	ON_UPDATE_COMMAND_UI(ID_UI_VIEW_SHOW_GRID, OnUpdateViewShowGrid)
	ON_UPDATE_COMMAND_UI(ID_UI_VIEW_SNAP_TO_GRID, OnUpdateViewSnapToGrid)
	ON_COMMAND(ID_UI_VIEW_SHOW_GRID, OnViewShowGrid)
	ON_COMMAND(ID_UI_VIEW_SNAP_TO_BORDER, OnViewSnapToBorder)
	ON_UPDATE_COMMAND_UI(ID_UI_VIEW_SNAP_TO_BORDER, OnUpdateViewSnapToBorder)
	ON_COMMAND(ID_UI_VIEW_SHOW_BORDER, OnViewShowBorder)
	ON_UPDATE_COMMAND_UI(ID_UI_VIEW_SHOW_BORDER, OnUpdateViewShowBorder)
	ON_COMMAND(ID_UI_EDIT_TO_TEXTURE_SIZE, OnEditToTextureSize)
	ON_COMMAND(ID_UI_VIEW_SHOWHOTKEYS, OnEditHotKeys)

	ON_COMMAND(ID_UI_VIEW_ZOOM_BY_WIDTH, OnViewZoomShowAll)
	ON_COMMAND(ID_UI_VIEW_ZOOM_ONE_TO_ONE, OnViewZoomOneToOne)
	ON_UPDATE_COMMAND_UI(ID_UI_VIEW_ZOOM_BY_WIDTH, OnUpdateViewZoomShowAll)
	ON_UPDATE_COMMAND_UI(ID_UI_VIEW_ZOOM_ONE_TO_ONE, OnUpdateViewZoomOneToOne)

	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


void CUIMainFrame::OnDestroy()
{
    // Экстрим
	CUIEditorPanel* pPanel = GetPanel ();
	if (pPanel->DestroyWindow ()) {
		delete pPanel;
	}

	CUIEditorView* viewWindow = &view();
	if (viewWindow && viewWindow->DestroyWindow ()) {
		delete viewWindow;
	}
	
	// dictionary.save ();
	Options::instance().saveLibrary();
    
	CFrameWnd::OnDestroy();
}

BOOL CUIMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	
    m_wndSplitter.CreateStatic (this, 1, 2, WS_CHILD | WS_VISIBLE);
	m_wndSplitter.CreateView (0, 0, RUNTIME_CLASS (CUIEditorPanel), CSize (200, 200), 0);
	m_wndSplitter.CreateView (0, 1, RUNTIME_CLASS (CUIEditorView), CSize (200, 200), 0);
	m_wndSplitter.ShowWindow (SW_SHOW);
	
	return CFrameWnd::OnCreateClient(lpcs, pContext);
}

CAttribEditorCtrl* CUIMainFrame::GetAttribEditor ()
{
	CUIEditorPanel* pPanel = static_cast<CUIEditorPanel*> (m_wndSplitter.GetPane (0, 0));
	ASSERT (pPanel);
	return &pPanel->attribEditor();
}

CUIEditorView& CUIMainFrame::view()
{
	CUIEditorView* view = static_cast<CUIEditorView*>(m_wndSplitter.GetPane (0, 1));
	xassert(view);
	return *view;
}
CUIEditorPanel* CUIMainFrame::GetPanel ()
{
	CUIEditorPanel* pPanel = static_cast<CUIEditorPanel*> (m_wndSplitter.GetPane (0, 0));
	return pPanel;
}

CControlsTreeCtrl* CUIMainFrame::GetControlsTree ()
{
	CUIEditorPanel* pPanel = static_cast<CUIEditorPanel*> (m_wndSplitter.GetPane (0, 0));
	ASSERT (pPanel);
	return &pPanel->controlsTree();
}

int CUIMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if(!m_wndToolBar.CreateEx (this) ||
	   !m_wndToolBar.LoadToolBar (IDR_UIEDITORFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	CWaitCursor wait_cursor;
	AttributeLibrary::instance();

	UI_Dispatcher::instance().init();

	GetControlsTree ()->buildTree ();

	UpdateWindow ();
	return 0;
}

void CUIMainFrame::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	view().OnKeyDown (nChar, nRepCnt, nFlags);
	CFrameWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CUIMainFrame::OnEditUndo()
{
	ActionManager::the().undo();
	view().updateProperties();
}

void CUIMainFrame::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	if(ActionManager::the().can_be_undone()){
		pCmdUI->Enable (TRUE);
		CString label;
		label.Format ("Undo %s\tCtrl+Z",
						ActionManager::the ().last_action_description ().c_str ());
		pCmdUI->SetText (label);
		return;
	}
	pCmdUI->Enable (FALSE);
	pCmdUI->SetText ("Can't Undo\tCtrl+Z");
}


void CUIMainFrame::OnEditDelete()
{
    view().eraseSelection();
}

void CUIMainFrame::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	if (!SelectionManager::the().selection ().empty ()) {
		pCmdUI->Enable (TRUE);
		return;
	}
	pCmdUI->Enable (FALSE);
}


LRESULT CUIMainFrame::OnCheckItsMe (WPARAM wParam, LPARAM lParam)
{
	ShowWindow(SW_RESTORE);
	SetForegroundWindow ();
	return 0;
}


void CUIMainFrame::OnFileSave()
{
	CWaitCursor waitCursor;
	ActionManager::the().setChanged(false);
    saveAllLibraries();
}

void CUIMainFrame::OnFileQuickSave()
{
	CWaitCursor waitCursor;
	ActionManager::the().setChanged(false);
	saveInterfaceLibraries();
}

void CUIMainFrame::OnEditFontLibrary()
{
	EditorLibraryInterface* library = LibrariesManager::instance().find("UI_FontLibrary");
	if(library)
		library->editLibrary(true);
	else
		xassert(0);
}

void CUIMainFrame::OnEditEquipmentSlotTypes()
{
	EditorLibraryInterface* library = LibrariesManager::instance().find("EquipmentSlotType");
	if(library)
		library->editLibrary(true);
	else
		xassert(0);
}

void CUIMainFrame::OnEditInventoryCellTypes()
{
	EditorLibraryInterface* library = LibrariesManager::instance().find("InventoryCellType");
	if(library)
		library->editLibrary(true);
	else
		xassert(0);
}

struct UIParametersSerialization{
	void serialize(Archive& ar)
	{
		UI_Dispatcher::instance().serializeUIEditorPrm(ar);
		UI_Render::instance().serialize(ar);
	}
};

void CUIMainFrame::OnEditParameters()
{
	if(kdw::edit(Serializer(UIParametersSerialization()), "Scripts\\TreeControlSetups\\UIParametersState", kdw::ONLY_TRANSLATED | kdw::IMMEDIATE_UPDATE, GetSafeHwnd())){
		UI_BackgroundScene::instance().init(gb_VisGeneric);
		ActionManager::the().setChanged();
	}
}

void CUIMainFrame::OnEditAddScreen()
{
	UI_Dispatcher::instance().addScreen (TRANSLATE("Новый экран"));
	GetControlsTree ()->buildTree ();
}

void CUIMainFrame::OnClose()
{
	if(ActionManager::the().haveChanges()){
		const char* message = TRANSLATE("Желаете сохранить изменения перед выходом?");
		int result = MessageBox(message, 0, MB_YESNOCANCEL | MB_ICONQUESTION);
		
		if(result == IDYES)
			OnFileSave ();
		else if(result == IDCANCEL)
			return;
	}

	CFrameWnd::OnClose();
}

void CUIMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
	CFrameWnd::OnActivateApp(bActive, dwThreadID);

	((CUIEditorApp*)AfxGetApp())->active_ = bool(bActive);
}

void CUIMainFrame::OnEditSnapOptions()
{
	if(kdw::edit(Serializer(Options::instance()), ".\\Scripts\\TreeControlSetups\\UIEditorOptionsState", kdw::ONLY_TRANSLATED | kdw::IMMEDIATE_UPDATE, GetSafeHwnd()))
		Options::instance().saveLibrary();

	view().updateWindowPosition();
}

void CUIMainFrame::OnUpdateViewShowGrid(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck (Options::instance().showGrid ());
}

void CUIMainFrame::OnUpdateViewSnapToGrid(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck (Options::instance().snapToGrid ());
}

void CUIMainFrame::OnViewSnapToGrid()
{
	Options::instance().setSnapToGrid (!Options::instance().snapToGrid());
}

void CUIMainFrame::OnViewShowGrid()
{
	Options::instance().setShowGrid (!Options::instance().showGrid());
}

void CUIMainFrame::OnViewSnapToBorder()
{
	Options::instance().setSnapToBorder(!Options::instance().snapToBorder());
}

void CUIMainFrame::OnUpdateViewSnapToBorder(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Options::instance().snapToBorder());
}

void CUIMainFrame::OnViewShowBorder()
{
	Options::instance().setShowBorder(!Options::instance().showBorder());
}

void CUIMainFrame::OnUpdateViewShowBorder(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Options::instance().showBorder());
}


void CUIMainFrame::OnEditToTextureSize()
{
	Selection selection = SelectionManager::the().selection();
	Selection::iterator it;
	FOR_EACH(selection, it){
		UI_ControlBase* control = *it;
		if(control && !control->states().empty()){
			UI_ControlState* state = control->state(0);
			if(!state)
				continue;
			const UI_ControlShowMode* mode = 0;
			mode = state->showMode(UI_SHOW_NORMAL);
			if(!mode)
				mode = state->showMode(UI_SHOW_DISABLED);
			if(!mode)
				mode = state->showMode(UI_SHOW_HIGHLITED);
			if(!mode)
				mode = state->showMode(UI_SHOW_ACTIVATED);
			if(!mode)
				continue;
			if(!mode->sprite().isEmpty()){
				Rectf position(control->position().left_top(), mode->sprite().size() / Vect2f(UI_Render::instance().renderSize()));
				Selection sel;
				sel.push_back(control);
				PositionChangeAction* action = new PositionChangeAction(sel);
				action->setNewRect(position);
				ActionManager::the().act(action);
			}

		}
	}
}

void CUIMainFrame::OnEditHotKeys()
{
	if(const UI_Screen* scr = UI_Dispatcher::instance().currentScreen())
		scr->saveHotKeys();
}

void CUIMainFrame::OnUpdateViewZoomShowAll(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Options::instance().zoomMode() == Options::ZOOM_SHOW_ALL);
}

void CUIMainFrame::OnUpdateViewZoomOneToOne(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Options::instance().zoomMode() == Options::ZOOM_ONE_TO_ONE);
}

void CUIMainFrame::OnViewZoomShowAll()
{
	Options::instance().setZoomMode(Options::ZOOM_SHOW_ALL);
	
	CRect rect;
	view().GetClientRect(&rect);
	view().OnSize(0, rect.Width(), rect.Height());
}

void CUIMainFrame::OnViewZoomOneToOne()
{
	Options::instance().setZoomMode(Options::ZOOM_ONE_TO_ONE);

	CRect rect;
	view().GetClientRect(&rect);
	view().OnSize(0, rect.Width(), rect.Height());
}

BOOL CUIMainFrame::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void CUIMainFrame::OnUpdateFileSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ActionManager::the().haveChanges());
}