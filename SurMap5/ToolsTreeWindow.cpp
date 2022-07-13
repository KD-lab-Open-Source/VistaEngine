#include "stdafx.h"

#include "ToolsTreeWindow.h"

#include "MainFrame.h"
#include "GeneralView.h"

#include "Game\CameraManager.h"

#include "ToolsTreeCtrl.h"

#include "SurToolSelect.h"
#include "SurToolMove.h"
#include "SurToolRotate.h"
#include "SurToolScale.h"

#include "SurToolPathEditor.h"

IMPLEMENT_DYNAMIC(CToolsTreeWindow, CFrameWnd)

BEGIN_MESSAGE_MAP(CToolsTreeWindow, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	
	ON_COMMAND_RANGE(ID_TOOL_FIRST, ID_TOOL_LAST, OnTool)
	ON_UPDATE_COMMAND_UI_RANGE(ID_TOOL_FIRST, ID_TOOL_LAST, OnUpdateTool)
	ON_WM_DESTROY()

	ON_CBN_SELCHANGE(ID_BRUSH_COMBO_PLACE, OnBrushRadiusComboSelChanged)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_COMBO_PLACE, OnUpdateBrushRadius)
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

CToolsTreeWindow::CToolsTreeWindow()
: mainFrame_(0)
, tree_(*new CToolsTreeCtrl)
, transformToolsEnabled_(true)
{
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if( !::GetClassInfo (hInst, className(), &wndclass) )
	{
		wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
		wndclass.lpfnWndProc	= ::DefWindowProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hInst;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= AfxGetApp ()->LoadStandardCursor (IDC_ARROW);
		wndclass.hbrBackground	= reinterpret_cast<HBRUSH> (COLOR_WINDOW);
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= className();

		if (!AfxRegisterClass (&wndclass))
			AfxThrowResourceException ();
	}

	tools_.resize(TOOL_MAX);
	
	tools_[TOOL_SELECT] = new CSurToolSelect();
	tools_[TOOL_MOVE] = new CSurToolMove();
	tools_[TOOL_ROTATE] = new CSurToolRotate();
	tools_[TOOL_SCALE] = new CSurToolScale();

}

CToolsTreeWindow::~CToolsTreeWindow()
{
	//delete &tree_; 
}

int CToolsTreeWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	VERIFY(g_CmdManager->ProfileSetup("Tools", GetSafeHwnd()));
	VERIFY(g_CmdManager->UpdateFromToolBar("Tools", IDR_TOOLS_BAR, 0, 0, false, true, RGB(255, 0, 255)));

	VERIFY(toolsBar_.Create("Tools Toolbar", this, IDR_TOOLS_BAR));
	VERIFY(toolsBar_.LoadToolBar(IDR_TOOLS_BAR, RGB(255, 0, 255)));

	toolsBar_.EnableDocking(CBRS_ALIGN_TOP);
    //////////////////////////////////////////////////////////////////////////////
    DWORD comboStyle = WS_VISIBLE | WS_CHILD | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS;
    if (!brushRadiusCombo_.Create(comboStyle, CRect(0, 0, 80, 300), &toolsBar_, ID_BRUSH_COMBO_PLACE)) {
        TRACE0("Failed to create combo-box\n");
        return FALSE;
    }
	defaultFont_.CreateFont (16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS,
							 CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma");
	brushRadiusCombo_.SetFont (&defaultFont_);
	toolsBar_.SetButtonCtrl(toolsBar_.CommandToIndex(ID_BRUSH_COMBO_PLACE), &brushRadiusCombo_);
    brushRadiusCombo_.SetCurSel(0);
	//Массив допустимых радиусов
	const long ArrSize_Brush[]={1,3,5,7,10,15,20,30,50,75,100,150,200};
    // Добавляем элементы данных в контрол.
	for(int i=0; i<sizeof(ArrSize_Brush)/sizeof(ArrSize_Brush[0]); i++ ){
		brushRadiusCombo_.SetItemData(brushRadiusCombo_.AddString(""), ArrSize_Brush[i]);
	}
    //////////////////////////////////////////////////////////////////////////////

	tree_.init(mainFrame_);
	if(!tree_.CreateEx(WS_EX_CLIENTEDGE, 0,0, WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|WS_BORDER
		/*|TVS_HASBUTTONS*/|TVS_HASLINES|TVS_LINESATROOT|TVS_EDITLABELS
        |TVS_INFOTIP|TVS_DISABLEDRAGDROP|TVS_SINGLEEXPAND|TVS_SHOWSELALWAYS|TVS_TRACKSELECT,
        CRect(0,0,0,0), this, 0))
	{
            AfxMessageBox ("Failed to create m_wndDockedCtrlTree\n");
			return -1;
	}

	VERIFY(CExtControlBar::FrameEnableDocking(this));

	RecalcLayout();

	replaceEditorMode();
	return 0;
}

BOOL CToolsTreeWindow::Create(CMainFrame* parent, CExtControlBar* bar)
{
	mainFrame_ = parent;
	if(!CFrameWnd::Create(className(), "", WS_CHILD | WS_VISIBLE,
		CRect(0,0,0,0), bar /*, bar->GetDlgCtrlID()*/)){
        AfxMessageBox ("Failed to create CToolsTreeWindow\n");
		return FALSE;
	}

	SetFont(CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT)));
	return TRUE;
}

void CToolsTreeWindow::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	CRect rt;
	GetClientRect(&rt);
	toolsBar_.GetWindowRect(&rt);
	tree_.MoveWindow(0, rt.Height(), cx, cy - rt.Height());
}

BOOL CToolsTreeWindow::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

	return TRUE;
}

void CToolsTreeWindow::OnTool(UINT nID)
{
	int index = nID - ID_TOOL_FIRST;
    replaceEditorMode(tools_.at(index));	
}

void CToolsTreeWindow::OnUpdateTool(CCmdUI *pCmdUI)
{
	int index = pCmdUI->m_nID - ID_TOOL_FIRST;
	pCmdUI->SetCheck(currentTool() == tools_.at(index));
	if(index > 0){
		pCmdUI->Enable(transformToolsEnabled_);
	}	
}

void CToolsTreeWindow::destroyToolWindows()
{
	for(int i = 0; i < tools_.size(); ++i) {
		if(::IsWindow(tools_[i]->GetSafeHwnd())) {
			tools_[i]->DestroyWindow();
		}
	}
	for(int i = 0; i < editorModeStack_.size() - 1; ++i){
		editorModeStack_[i]->DestroyWindow();
	}
	tree_.destroyToolWindows();
}

void CToolsTreeWindow::setCurrentTool(CSurToolBase* tool)
{
	if(currentTool() == tool && ::IsWindow(tool->GetSafeHwnd()))
		return;
	
	destroyToolWindows();
	enableTransformTools(true);

	bool tool_in_tree = true;
	for(int i = 0; i < tools_.size(); ++i) {
		if(tool == tools_[i])
			tool_in_tree = false;
	}
	//if(!tool_in_tree)
	//	tree_.SelectItem(0);

	Vect3f cursorPosition = Vect3f::ZERO;
	if(vMap.isWorldLoaded()) {
		if(cameraManager){
			cursorPosition = cameraManager->coordinate().position();
			tool->setCursorPosition(cursorPosition);
		}
	}

	if(::IsWindow(mainFrame_->getResizableBarDlg()->GetSafeHwnd())){
		VERIFY(tool->CreateExt(mainFrame_->getResizableBarDlg()));
		tool->ShowSizeGrip(FALSE);
		CRect rt;
		mainFrame_->getResizableBarDlg()->GetClientRect(&rt);
		tool->MoveWindow(0, 0, rt.Width(), rt.Height());
		tool->ShowWindow(SW_SHOWNOACTIVATE);
		mainFrame_->view().SetFocus();
	}
}

const CSurToolBase* CToolsTreeWindow::currentTool() const
{
    if(!editorModeStack_.empty())
        return editorModeStack_.back();
    else
        return 0;
}

CSurToolBase* CToolsTreeWindow::currentTool()
{
    if(!editorModeStack_.empty())
        return editorModeStack_.back();
    else
        return 0;
}

void CToolsTreeWindow::OnDestroy()
{
	CFrameWnd::OnDestroy();

    editorModeStack_.clear();
}

bool CToolsTreeWindow::onKeyDown(UINT key, UINT flags)
{
    switch(key){
    case VK_ESCAPE:
        popEditorMode();
        return true;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	{
		int index = key - '0';
		xassert(index >= 0 && index <= 9);
		if(GetAsyncKeyState(VK_CONTROL)){
			if(currentTool())
				tree_.getToolPath(tree_.shortcuts_[index], currentTool());
		}
		else{
			if(!tree_.shortcuts_[index].empty()){
				CSurToolBase* tool = tree_.findTool(tree_.shortcuts_[index]);
				//replaceEditorMode(tool);
				tree_.GetTreeCtrl().SelectItem(tool->treeItem());
			}
		}
		return true;
	}
    case 'Q':
		replaceEditorMode(tools_[TOOL_SELECT]);
        return true;
    case 'W':
		if(transformToolsEnabled_){
			replaceEditorMode(tools_[TOOL_MOVE]);
			return true;
		}
		break;
    case 'E':
		if(transformToolsEnabled_){
			replaceEditorMode(tools_[TOOL_ROTATE]);
	        return true;
		}
		break;
    case 'R':
		if(transformToolsEnabled_){
			replaceEditorMode(tools_[TOOL_SCALE]);
			return true;
		}
		break;
    };
	return false;
}

void CToolsTreeWindow::OnBrushRadiusComboSelChanged ()
{
	if(CSurToolBase* currentToolzer = currentTool())
		currentToolzer->onBrushRadiusChanged();
}

void CToolsTreeWindow::OnUpdateBrushRadius(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(vMap.isWorldLoaded());
}

int CToolsTreeWindow::getBrushRadius(void)
{
	return brushRadiusCombo_.GetItemData(brushRadiusCombo_.GetCurSel()); 
}

void CToolsTreeWindow::setBrushRadius(int rad)
{
	int cboxSize=brushRadiusCombo_.GetCount();
	for(int i=0; i<cboxSize; i++){
		if(brushRadiusCombo_.GetItemData(i)==rad){
			brushRadiusCombo_.SetCurSel(i);
		}
	}
}

void CToolsTreeWindow::previousBrush()
{
	brushRadiusCombo_.SetCurSel(max(0, brushRadiusCombo_.GetCurSel()-1));
}

void CToolsTreeWindow::nextBrush()
{
	brushRadiusCombo_.SetCurSel(min(brushRadiusCombo_.GetCount() - 1, brushRadiusCombo_.GetCurSel() + 1));
}

void CToolsTreeWindow::replaceEditorMode(CSurToolBase* tool)
{
    editorModeStack_.clear();
	if(tool)
		pushEditorMode(tool);
	else
		pushEditorMode(tools_[TOOL_SELECT]);
}

void CToolsTreeWindow::pushEditorMode(CSurToolBase* tool)
{
	EditorModeStack::iterator it = std::find(editorModeStack_.begin(), editorModeStack_.end(), tool);
	if(it != editorModeStack_.end())
		xassert(0);
	editorModeStack_.push_back(tool);
	setCurrentTool(tool);
}

void CToolsTreeWindow::popEditorMode()
{
    xassert(!editorModeStack_.empty());

    editorModeStack_.pop_back();
    if(editorModeStack_.empty())
        editorModeStack_.push_back(tools_[TOOL_SELECT]);
    setCurrentTool(editorModeStack_.back());
}

void CToolsTreeWindow::rebuildTools()
{
	replaceEditorMode();
	tree_.BuildTree();
}

void CToolsTreeWindow::enableTransformTools(bool enable)
{
	transformToolsEnabled_ = enable;
	//toolsBar_.E
}
bool CToolsTreeWindow::isToolFromTreeSelected() const
{
	const CSurToolBase* tool = currentTool();
	if(std::find(tools_.begin(), tools_.end(), tool) == tools_.end())
		return true;
	else
		return false;
}

void CToolsTreeWindow::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	onKeyDown(nChar, nFlags);

	CFrameWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}
