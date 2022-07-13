#include "StdAfx.h"
#include "SurMap5.h"

#include "SurToolUnit.h"
#include "SurToolSource.h"
#include "SurToolAnchor.h"
#include "SurTool3DM.h"
#include "SurToolCameraEditor.h"

#include "ObjectsManagerWindow.h"
#include "ObjectsManagerTree.h"

#include "ToolsTreeWindow.h"
#include "MainFrame.h"

#include "AttribEditor\AttribEditorCtrl.h"
#include "MFC\SizeLayoutManager.h"
#include "Environment\SourceManager.h"
#include "Environment\Anchor.h"

#include "Game\Universe.h"
#include "Units\Nature.h"
#include "Units\EnvironmentSimple.h"

#include "Game\CameraManager.h"
#include "NameComboDlg.h"

#include <algorithm>

#include "SelectionUtil.h"

#include "kdw/Clipboard.h"
#include "kdw/Win32Proxy.h"
#include "FileUtils\FileUtils.h"
#include "mfc\PopupMenu.h"

//////////////////////////////////////////////////////////////////////////////

BOOL CObjectsManagerWindow::Create(DWORD style, const CRect& rect, CWnd* parentWnd)
{
	return CWnd::Create(className(), 0, style, rect, parentWnd, 0);
}

CObjectsManagerWindow::CObjectsManagerWindow(CMainFrame* mainFrame)
: sortByName_(true)
, skipSelChanged_(false)
, popupMenu_(new PopupMenu(32))
, tree_(new CObjectsManagerTree())
{
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if(!::GetClassInfo (hInst, className(), &wndclass)){
        wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|WS_TABSTOP;
		wndclass.lpfnWndProc	= ::DefWindowProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hInst;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= AfxGetApp ()->LoadStandardCursor (IDC_ARROW);
		wndclass.hbrBackground	= reinterpret_cast<HBRUSH> (COLOR_WINDOW);
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= className();

		if(!AfxRegisterClass(&wndclass))
			AfxThrowResourceException();
	}
}

CObjectsManagerWindow::~CObjectsManagerWindow()
{
}

BEGIN_MESSAGE_MAP(CObjectsManagerWindow, CWnd)
	ON_NOTIFY(TCN_SELCHANGE,  0, OnTabChanged)

	ON_WM_SHOWWINDOW()
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


int CObjectsManagerWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

    CRect treeRect;
    GetClientRect(&treeRect);
	treeRect.top = 24;
	tree_->initControl(0, this);
	tree_->MoveWindow(&treeRect, FALSE);

	CRect rect;
	tree_->GetClientRect(&rect);
	VERIFY(typeTabs_.Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, treeRect.right, treeRect.top), this, 0));

	typeTabs_.SetFont(CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT)));
	typeTabs_.InsertItem (TAB_SOURCES,		 TRANSLATE("Источники"));
	typeTabs_.InsertItem (TAB_ENVIRONMENT,   TRANSLATE("Окружение"));
	typeTabs_.InsertItem (TAB_UNITS,         TRANSLATE("Юниты"));
	typeTabs_.InsertItem (TAB_CAMERA,        TRANSLATE("Камеры"));
	typeTabs_.InsertItem (TAB_ANCHORS,       TRANSLATE("Якори"));
	typeTabs_.SetCurSel (TAB_SOURCES);

	if(!layout_.isInitialized()){
		layout_.init(this);
		layout_.add(true, true, true, false, &typeTabs_);
		layout_.add(true, true, true, true, tree_);
	}

	LRESULT result;
	OnTabChanged(0, &result);

	eventMaster().signalWorldChanged().connect(this, &CObjectsManagerWindow::onWorldChanged);
	eventMaster().signalObjectChanged().connect(this, &CObjectsManagerWindow::onObjectChanged);
	eventMaster().signalSelectionChanged().connect(this, &CObjectsManagerWindow::onSelectionChanged);
	return 0;
}

void CObjectsManagerWindow::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	layout_.onSize(cx, cy);
}





void CObjectsManagerWindow::OnTabChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	if(typeTabs_.GetCurSel() >= 0){
		tree_->tree()->setTab(ObjectsManagerTab(typeTabs_.GetCurSel()));
	}
	else
		tree_->tree()->rebuild();

	*pResult = 0;
}


void CObjectsManagerWindow::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CWnd::OnShowWindow(bShow, nStatus);

	if(IsWindow(typeTabs_.GetSafeHwnd())){
		LRESULT result;
		OnTabChanged(0, &result);
	}
}

void CObjectsManagerWindow::onSelectionChanged(SelectionObserver* observer)
{
	if(observer == tree()->tree())
		return;
	tree_->tree()->updateSelectFromWorld();
}


void CObjectsManagerWindow::OnObjectsTreeRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if(!universe() || !(sourceManager))
		return;

	CPoint cursorPosition;
	GetCursorPos(&cursorPosition);

	if(pNMHDR->hwndFrom == typeTabs_.GetSafeHwnd()){
		popupMenu_->clear();
		PopupMenuItem& root = popupMenu_->root();

		root.add(TRANSLATE("Выделить все в текущей вкладке"))
			.connect(bindMethod(*this, &Self::onMenuTabSelectAll));
		root.add(TRANSLATE("Снять выделение в текущей вкладке"))
			.connect(bindMethod(*this, &Self::onMenuTabDeselectAll));
		root.add(TRANSLATE("Снять выделение с других вкладок"))
			.connect(bindMethod(*this, &Self::onMenuTabDeselectOthers));


		popupMenu_->spawn(cursorPosition, GetSafeHwnd());
		*pResult = 1;
	}
	*pResult = 0;
}


CObjectsManagerWindow::UpdateLock::UpdateLock(CObjectsManagerWindow& objectsManager)
: objectsManager_(objectsManager)
{
    objectsManager_.skipSelChanged_ = true;
}

CObjectsManagerWindow::UpdateLock::~UpdateLock()
{
    objectsManager_.skipSelChanged_ = false;
    objectsManager_.tree_->tree()->update();
}

void CObjectsManagerWindow::onMenuTabDeselectOthers()
{
	//selectOnWorld(); FIXME
}

void CObjectsManagerWindow::onMenuTabSelectAll()
{
	/* FIXME
	kdw::TreeRow::iterator it;
	FOR_EACH(*tree_->root(), it){
		WorldTreeObject* object = safe_cast<WorldTreeObject*>(&**it);
		object->select();
		object->setSelected(true);
	}
	eventMaster().eventSelectionChanged().emit();
	*/
}

void CObjectsManagerWindow::onMenuTabDeselectAll()
{
	/* FIXME
	kdw::TreeRow::iterator it;
	FOR_EACH(*tree_->root(), it){
		WorldTreeObject* object = safe_cast<WorldTreeObject*>(&**it);
		object->deselect();
		object->setSelected(false);
	}
	eventMaster().eventSelectionChanged().emit();
	*/
}

CObjectsManagerTree* CObjectsManagerWindow::tree()
{
	return tree_;
}

void CObjectsManagerWindow::onWorldChanged(WorldObserver* changer)
{
	tree_->clear();
}

void CObjectsManagerWindow::onObjectChanged(ObjectObserver* changer)
{
	tree()->tree()->rebuild();
	LRESULT result;
	OnTabChanged (0, &result);
}

BOOL CObjectsManagerWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	popupMenu_->onCommand(wParam, lParam);

	return CWnd::OnCommand(wParam, lParam);
}


int CObjectsManagerWindow::currentTab()
{
	return typeTabs_.GetCurSel();
}
