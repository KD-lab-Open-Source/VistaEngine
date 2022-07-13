#include "stdafx.h"
#include "TreeSelector.h"
#include "SafeCast.h"

#include "..\Util\MFC\SizeLayoutManager.h"

IMPLEMENT_DYNAMIC(CTreeSelectorDlg, CDialog)
CTreeSelectorDlg::CTreeSelectorDlg(CWnd* pParent /*=NULL*/)
: CDialog(CTreeSelectorDlg::IDD, pParent)
, builder_(0)
, layout_(new CSizeLayoutManager())
, tree_(new CBuildableTreeCtrl())
{
}

CTreeSelectorDlg::~CTreeSelectorDlg()
{
}

void CTreeSelectorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_CONTROL, *tree_);
}


BEGIN_MESSAGE_MAP(CTreeSelectorDlg, CDialog)
	ON_WM_SIZE()
	ON_EN_CHANGE(IDC_FILTER_EDIT, OnFilterEditChange)
END_MESSAGE_MAP()


// CTreeSelectorDlg message handlers

BOOL CTreeSelectorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	tree_->initControl(TLC_TREELIST
					| TLC_READONLY
					| TLC_SHOWSELACTIVE
					| TLC_SHOWSELALWAYS	
					| TLC_SHOWSELFULLROWS
					| TLC_IMAGE
					| TLC_DRAG | TLC_DROP | TLC_DROPTHIS
//					| TLC_HEADER
					| TLC_BUTTON
					| TLC_TREELINE);

	tree_->insertColumn("main", 200);

	if(builder_) {
		CWaitCursor waitCursor;
		TreeBuilder::Object* object = builder_->buildTree(tree_->rootObject());
		tree_->rootObject()->matchFilter("");
		if(object && !object->matching())
			object = 0;
		tree_->rootObject()->buildFiltered();
		if(object && object->matching())
			object->focus();
	}

	layout_->init(this);
	layout_->add(1, 1, 1, 1, tree_);
	layout_->add(1, 0, 1, 1, IDC_HORIZONTAL_LINE);
           
	layout_->add(1, 0, 0, 1, IDC_FILTER_LABEL);
	layout_->add(1, 0, 1, 1, IDC_FILTER_EDIT);
          
	layout_->add(0, 0, 1, 1, IDOK);
	layout_->add(0, 0, 1, 1, IDCANCEL);

	tree_->SetFocus();
	return FALSE;
}
 
CObjectsTreeCtrl& CTreeSelectorDlg::tree()
{
	return *tree_;
}

void CTreeSelectorDlg::setBuilder(TreeBuilder* builder)
{
	builder_ = builder;
}

void CTreeSelectorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	layout_->onSize(cx, cy);

	CRect rect;
	tree_->GetWindowRect(&rect);
	tree_->SetColumnWidth (0, rect.Width() - GetSystemMetrics (SM_CXHSCROLL) - 2);
}

void CTreeSelectorDlg::OnOK()
{
	TreeObjects objects = tree_->selection();
	if(!objects.empty() && builder_){
		if(builder_->select(safe_cast<TreeBuilder::Object*>(objects.front()))){
			CDialog::OnOK();
		}
	}
}

void CTreeSelectorDlg::OnFilterEditChange()
{
	if(builder_) {
		tree_->AllowRedraw(FALSE);
		tree_->clear();
		CString filter;
		GetDlgItemText(IDC_FILTER_EDIT, filter);
		TreeBuilder::Object* focus = builder_->buildTree(tree_->rootObject());
	
		tree_->rootObject()->matchFilter(filter);
		if(focus && !focus->matching())
			focus = 0;
		tree_->rootObject()->buildFiltered();

		if(filter != "")
			tree_->rootObject()->expandAll();
		if(focus)
			focus->focus();
		tree_->AllowRedraw(TRUE);
	}
}

static bool stringMatchFilter(const char* name, const char* _filter)
{
	std::string nameString(name);
	CharLowerBuff(&nameString[0], nameString.size());
	if(_filter[0] == 0)
		return true;
	std::string filter = _filter;
	CharLowerBuff(&filter[0], filter.size());
	if(nameString.find(filter) != std::string::npos)
		return true;
	else
		return false;
}

TreeObjectFilteredBase::TreeObjectFilteredBase()
: matchFilter_(false)
, removeIfEmpty_(false)
{
}

bool TreeObjectFilteredBase::onDoubleClick()
{
    CBuildableTreeCtrl* tree = safe_cast<CBuildableTreeCtrl*>(tree_);
	CTreeSelectorDlg* dlg = safe_cast<CTreeSelectorDlg*>(tree->GetParent());
	dlg->OnOK();
	return true;
}

TreeObjectFilteredBase* TreeObjectFilteredBase::addFiltered(TreeObjectFilteredBase* object, bool sort)
{
	object->matchFilter_ = false;
	if(sort){
		Children::iterator it;
		for(it = potentialChildren_.begin(); it != potentialChildren_.end(); ++it){
			if(stricmp((*it)->name(), object->name()) > 0){
				potentialChildren_.insert(it, object);
				return object;
			}
		}
	}
	potentialChildren_.push_back(object);
	return object;
}

bool TreeObjectFilteredBase::matchFilter(const char* filter)
{
	matchFilter_ = stringMatchFilter(name(), filter);
	Children::iterator it;

	bool haveMatchingChildren = false;
	FOR_EACH(potentialChildren_, it){
		TreeObjectFilteredBase* child = *it;
		if(child->matchFilter(filter)){
			matchFilter_ = true;
			haveMatchingChildren = true;
		}
	}
	if(!haveMatchingChildren && removeIfEmpty_)
		matchFilter_ = false;
	return matchFilter_;
}

bool TreeObjectFilteredBase::buildFiltered()
{
	if(!matchFilter_)
		return false;

	Children::iterator it;
	FOR_EACH(potentialChildren_, it){
		TreeObjectFilteredBase* child = *it;
		if(child->matching())
			TreeObject::add(child);
		child->buildFiltered();
	}
	potentialChildren_.clear();
	return true;
}

TreeBuilder::Object* CBuildableTreeCtrl::rootObject()
{
	return safe_cast<TreeBuilder::Object*>(__super::rootObject());
}

CBuildableTreeCtrl::CBuildableTreeCtrl()
{
	overrideRoot(new TreeBuilder::Object(""));
}