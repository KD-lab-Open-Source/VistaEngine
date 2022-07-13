#include "StdAfx.h"

#include "mfc\LayoutMFC.h"
#include "mfc\PopupMenu.h"

#include "LibraryEditorWindow.h"
#include "LibraryEditorTree.h"
#include "mfc\ObjectsTreeCtrl.h"
#include "TreeEditors\TreeSelector.h"
#include "LibrariesManager.h"
#include "LibraryWrapper.h"
#include "LibraryAttribEditor.h"
#include "LibraryTab.h"
#include "LibraryTreeObject.h"

#include "Dictionary.h"

#include "EditArchive.h"

void loadAllLibraries(bool = false);

IMPLEMENT_DYNAMIC(CLibraryEditorWindow, CWnd)

BEGIN_MESSAGE_MAP(CLibraryEditorWindow, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CLOSE()
    ON_NOTIFY(TCN_SELCHANGE, 0, onTabChange)
END_MESSAGE_MAP()

CLibraryEditorWindow::CLibraryEditorWindow(CWnd* parent)
: parent_(parent)
, tree_(0)
, hbox_(0)
, attribEditor_(new CLibraryAttribEditor)
, modalResult_(false)
, layout_(new LayoutWindow(this))
, currentTab_(0)
, popupMenu_(new PopupMenu(100))
{
    WNDCLASS wndclass;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if(!::GetClassInfo(hInst, className(), &wndclass)){
        wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|WS_TABSTOP;
        wndclass.lpfnWndProc	= ::DefWindowProc;
        wndclass.cbClsExtra		= 0;
        wndclass.cbWndExtra		= 0;
        wndclass.hInstance		= hInst;
        wndclass.hIcon			= 0;
        wndclass.hCursor		= AfxGetApp()->LoadStandardCursor(IDC_ARROW);
        wndclass.hbrBackground	= reinterpret_cast<HBRUSH>(COLOR_WINDOW);
        wndclass.lpszMenuName	= 0;
        wndclass.lpszClassName	= className();

        if(!AfxRegisterClass(&wndclass))
            AfxThrowResourceException();
    }
	tree_ = new CLibraryEditorTree(attribEditor_);

	TreeEditorFactory::instance().buildMap();
	LibraryCustomEditorFactory::instance().registerQueued();

	loadAllLibraries();
}

CLibraryEditorWindow::~CLibraryEditorWindow()
{
	setCurrentTab(0, true);
}

void CLibraryEditorWindow::serialize(Archive& ar)
{
	ar.serialize(openTabs_, "openTabs", 0);

	if(ar.isInput()){
		Tabs::iterator it;
		for(it = openTabs_.begin(); it != openTabs_.end(); ){
			if((*it)->isValid()){
				(*it)->setWindow(this);
				++it;
			}
			else{
				it = openTabs_.erase(it);
			}
		}
	}
}


BOOL CLibraryEditorWindow::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	return CWnd::CreateEx(WS_EX_CONTROLPARENT, className(), "Library Editor", style | WS_TABSTOP, rect, parent, id);
}

int CLibraryEditorWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CWaitCursor waitCursor;

	CFont* font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));

	LayoutVBox* box = new LayoutVBox(5, 5);
	{
        VERIFY(backButton_.Create("&Back", WS_VISIBLE | WS_CHILD | WS_TABSTOP, CRect(0, 0, 50, 25), this, 0));
        backButton_.SetFont(font);
		backButton_.EnableWindow(FALSE);

		VERIFY(tabs_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(0, 0, 50, 25), this, 0));
		tabs_.signalMouseButtonDown() = bindMethod(*this, &Self::onTabMouseButtonDown);
        tabs_.SetFont(font);

        VERIFY(selectLibraryButton_.Create("&Open Library...", WS_VISIBLE | WS_CHILD | WS_TABSTOP, CRect(0, 0, 100, 25), this, 0));
        selectLibraryButton_.SetFont(font);

		LayoutHBox* hbox = new LayoutHBox(0);
		hbox->pack(new LayoutControl(&backButton_));
		hbox->pack(new LayoutControl(&selectLibraryButton_));
		hbox->pack(new LayoutControl(&tabs_), true, true, 5);
		box->pack(hbox);
	}

	{
        VERIFY(tree_->Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP, CRect(0, 0, 50, 100), this, 0));
        tree_->SetFont(font);

        
        VERIFY(attribEditor_->Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP, CRect(0, 0, 200, 100), this, 0));
        attribEditor_->SetFont(font);
        attribEditor_->setStyle(CAttribEditorCtrl::AUTO_SIZE | CAttribEditorCtrl::HIDE_ROOT_NODE);

        // TODO: Заменить на LahoutHSplitter
		hbox_ = new LayoutHBox(5);
		hbox_->pack(new LayoutControl(tree_), true, true);
		hbox_->pack(new LayoutControl(attribEditor_), true, true);
		box->pack(hbox_, true, true);
	}

	{
		VERIFY(progressBar_.Create(PBS_SMOOTH | WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 25), this, 0));
		progressBar_.SetRange(0, 10000);

		VERIFY(saveButton_.Create("S&ave", BS_DEFPUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP, CRect(0, 0, 80, 25), this, 0));
        saveButton_.SetFont(font);

        VERIFY(closeButton_.Create("&Close", WS_VISIBLE | WS_CHILD | WS_TABSTOP, CRect(0, 0, 80, 25), this, 0));
        closeButton_.SetFont(font);

		LayoutHBox* buttonsBox = new LayoutHBox(5);
        buttonsBox->pack(new LayoutControl(&progressBar_), true, true);
        buttonsBox->pack(new LayoutControl(&saveButton_));
        buttonsBox->pack(new LayoutControl(&closeButton_));
		box->pack(buttonsBox);
	}


	layout_->add(box);
	if(libraryToOpen_.empty()){
		setCurrentTab(openTabs_.empty() ? 0 : openTabs_.front(), false);
	}
	else{
		openBookmark(LibraryBookmark(libraryToOpen_.c_str()), false);
	}
	updateTabs();
	return 0;
}

bool CLibraryEditorWindow::doModal(const char* libraryName)
{
	if(parent_ && ::IsWindow(parent_->GetSafeHwnd()))
		parent_->EnableWindow(FALSE);

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int width = 750;
	int height = 550;
	int left = (screenWidth - width) / 2;
	int top = (screenHeight - height) / 2;

	libraryToOpen_ = libraryName;
	VERIFY(Create(WS_VISIBLE | WS_OVERLAPPEDWINDOW, CRect(left, top, left + width, top + height), parent_, 0));
	RunModalLoop(0);

	ShowWindow(SW_HIDE);
	DestroyWindow();

	if(parent_ && ::IsWindow(parent_->GetSafeHwnd())){
		parent_->EnableWindow(TRUE);
		parent_->SetActiveWindow();
		parent_->SetFocus();
	}  


	return modalResult_;
}

void CLibraryEditorWindow::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	layout_->relayout();
}

class LibrariesTreeBuilder : public TreeBuilder {
public:
	LibrariesTreeBuilder(std::string& name)
	: name_(name)
	{}
protected:
	/*
	class LibraryObject : public Object{
		LibraryObject(const char* editName, const char* name)
			: Object(name, name)
			, libraryName_(name)
		{
		}
		const char* libraryName() const { return libraryName_.c_str(); }
	protected:
		std::string libraryName_;
	};
	*/
    std::string& name_;

    Object* buildTree(Object* root){
        LibrariesManager::Libraries& libs = LibrariesManager::instance().editorLibraries();
        LibrariesManager::Libraries::iterator it;
		Object* result = 0;
        FOR_EACH(libs, it){
            const std::string& name = it->first;
            LibrariesManager::LibraryInstanceFunc instance = it->second;

            Object* o = root->add(new Object(instance().editName(), name.c_str()), true);
			if(name_ == name)
				result = o;
        }
		return result;
    }
	bool select(Object* object){
		if(object->selectable()){
            name_ = object->key();
			return true;
		}
		else{
			return false;
		}
	}
private:
};

LibraryTabBase* CLibraryEditorWindow::closeTab(LibraryTabBase* tab)
{
    Tabs::iterator it;
    it = std::find(openTabs_.begin(), openTabs_.end(), tab);
    xassert(it != openTabs_.end());
	tab->onClose();

    LibraryTabBase* newTab = 0;
    it = openTabs_.erase(it);
    if(openTabs_.empty())
        newTab = 0;
    else{
        if(it == openTabs_.end())
            newTab = openTabs_.back();
        else
            newTab = *it;
    }
	if(tab == currentTab_)
		currentTab_ = 0;
	return newTab;
}

/*
void CLibraryEditorWindow::onMenuCloseLibrary()
{
    Tabs::iterator it;
    it = std::find(openTabs_.begin(), openTabs_.end(), currentTab_);
    xassert(it != openTabs_.end());

    LibraryTabBase* newTab = 0;
    it = openTabs_.erase(it);
    if(openTabs_.empty())
        newTab = 0;
    else{
        if(it == openTabs_.end())
            newTab = openTabs_.back();
        else
            newTab = *it;
    }
	setCurrentTab(newTab, false);
    updateTabs();
}
*/

void CLibraryEditorWindow::updateTabs()
{
    tabs_.DeleteAllItems();

    Tabs::iterator it;
    int currentTab = -1;
    int index = 0;
    FOR_EACH(openTabs_, it){
        if(*it == currentTab_)
            currentTab = index;
        tabs_.InsertItem(index++, (*it)->title());
    }
	xassert(!currentTab_ || currentTab != -1); 
    tabs_.SetCurSel(currentTab);
}

void CLibraryEditorWindow::setLayout(bool singleLayout)
{
    hbox_->clear();
	tree_->MoveWindow(0, 0, 50, 100, FALSE);
	attribEditor_->MoveWindow(0, 0, 200, 100, FALSE);
    if(singleLayout){
        hbox_->pack(new LayoutControl(tree_), true, true);
        attribEditor_->ShowWindow(SW_HIDE);
    }
    else{
        hbox_->pack(new LayoutControl(tree_), true, true);
        hbox_->pack(new LayoutControl(attribEditor_), true, true);
        attribEditor_->ShowWindow(SW_SHOWNOACTIVATE);
    }
    hbox_->relayout();
    layout_->relayout();
}

void CLibraryEditorWindow::addTab(LibraryTabBase* tab)
{
	openTabs_.push_back(tab);
}

void CLibraryEditorWindow::setCurrentTab(LibraryTabBase* tab, bool enableHistory)
{
	if(currentTab_){
		if(enableHistory)
			addToHistory(currentTab_->bookmark());
		currentTab_->onClose();
	}

	currentTab_ = tab;

	tree_->signalElementSelected().clear();
	attribEditor_->signalElementChanged().clear();
	attribEditor_->signalElementSelected().clear();
    attribEditor_->signalSearchTreeNode().clear();
    attribEditor_->signalFollowReference().clear();

	if(!::IsWindow(GetSafeHwnd()))
		return;

	if(tab){
		setLayout(tab->singleLayout());
		tab->onSelect();
	}
	else{
		tree_->clear();
		attribEditor_->detachData();
	}
}

void CLibraryEditorWindow::openLibrary(const char* name, const char* elementName)
{
}

void CLibraryEditorWindow::onSelectLibraryButton()
{
    CTreeSelectorDlg selectorDialog(this);
    std::string name;
    LibrariesTreeBuilder builder(name);
    selectorDialog.setBuilder(&builder);
    if(selectorDialog.DoModal() == IDOK){
        openBookmark(LibraryBookmark(name.c_str()), false);
    }
}

void CLibraryEditorWindow::onSaveButton()
{
	modalResult_ = true;
	EndModalLoop(IDOK);
}

void CLibraryEditorWindow::onCloseButton()
{
	modalResult_ = false;
	EndModalLoop(IDCANCEL);
}

BOOL CLibraryEditorWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if(attribEditor_ && tree_){
		if(wParam == BN_CLICKED){
			if(HWND(lParam) == selectLibraryButton_.GetSafeHwnd())
				onSelectLibraryButton();
			if(HWND(lParam) == backButton_.GetSafeHwnd())
				onBackButton();
			if(HWND(lParam) == saveButton_.GetSafeHwnd())
				onSaveButton();
			if(HWND(lParam) == closeButton_.GetSafeHwnd())
				onCloseButton();
		}
		popupMenu_->onCommand(wParam, lParam);
	}
    return __super::OnCommand(wParam, lParam);
}

void CLibraryEditorWindow::onTabClose(LibraryTabBase* tab)
{
	setCurrentTab(closeTab(tab), false);
	updateTabs();
}

void CLibraryEditorWindow::onTabMouseButtonDown(int tabIndex, UINT flags)
{
	CPoint pt;
	GetCursorPos(&pt);
    xassert(tabIndex >= 0 && tabIndex < openTabs_.size());
    LibraryTabBase* tab = openTabs_[tabIndex];
    if(flags & MK_RBUTTON){
        popupMenu_->clear();
        PopupMenuItem& root = popupMenu_->root();
		tab->onMenuConstruction(root);
		if(!root.empty())
			root.addSeparator();
        root.add(TRANSLATE("Закрыть"))
            .connect(bindArgument(bindMethod(*this, &Self::onTabClose), tab));
        root.add(TRANSLATE("Закрыть другие"))
            .enable(false);

        popupMenu_->spawn(pt, GetSafeHwnd());
    }
    else if(flags & MK_MBUTTON){
        onTabClose(tab);
	}
}

void CLibraryEditorWindow::onTabChange(NMHDR *nmhdr, LRESULT *result)
{
    int tab = tabs_.GetCurSel();

	xassert(tab >= 0 && tab < openTabs_.size());
	Tabs::iterator it = openTabs_.begin();
	std::advance(it, tab);
    setCurrentTab(*it, false);
}

void CLibraryEditorWindow::OnClose()
{
	modalResult_ = false;
	EndModalLoop(IDCANCEL);
}

void CLibraryEditorWindow::openBookmark(LibraryBookmark bookmark, bool enableHistory)
{
	const char* name = bookmark.libraryName();
    LibraryTabBase* libraryTab = 0;
    Tabs::iterator it;
    FOR_EACH(openTabs_, it){
        LibraryTabBase* tab = *it;
        if(!strcmp(tab->bookmark().libraryName(), name)){
            libraryTab = tab;
            break;
        }
    }

	bool createNewTab = !libraryTab;
    if(createNewTab){
        libraryTab = new LibraryTabEditable(this, bookmark);
        openTabs_.push_back(libraryTab);
    }
	setCurrentTab(libraryTab, enableHistory);
	if(!createNewTab)
        libraryTab->setBookmark(bookmark);
	updateTabs();
}


void CLibraryEditorWindow::onProgress(float progress)
{
	progressBar_.SetPos(round(10000.0f * progress));
}


void CLibraryEditorWindow::onBackButton()
{
	if(!history_.empty()){
		LibraryBookmark bookmark = history_.back();
		history_.pop_back();
		openBookmark(bookmark, false);	

		backButton_.EnableWindow(history_.empty() ? FALSE : TRUE);
	}
	else{
		xassert(0);
	}	
}

void CLibraryEditorWindow::addToHistory(const LibraryBookmark& bookmark)
{
	history_.push_back(bookmark);
	if(::IsWindow(GetSafeHwnd()))
		backButton_.EnableWindow(TRUE);
}

void CLibraryEditorWindow::onElementCopied(const char* name, Serializeable serializeable)
{
}

void CLibraryEditorWindow::onElementPasted(Serializeable serializeable)
{
}
