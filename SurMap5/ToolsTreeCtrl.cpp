#include "stdafx.h"
#include ".\ToolsTreeCtrl.h"
#include "mfc\PopupMenu.h"
#include "SurMap5.h"
#include "ToolsTreeWindow.h"
#include "MainFrame.h"
#include "Serialization.h"
#include "XPrmArchive.h"
#include "MultiArchive.h"
#include "Dictionary.h"
#include "EditArchive.h"
	
#include "SurTool3DM.h"
#include "SurToolBlur.h"
#include "SurToolGeoNet.h"
#include "SurToolImp.h"
#include "SurToolToolzer.h"
#include "SurToolEmpty.h"
#include "SurToolWater.h"
#include "SurToolKind.h"
#include "SurToolColorPic.h"
#include "SurToolUnit.h"
#include "SurToolUnitFolder.h"
#include "SurToolCamera.h"
#include "SurToolCameraRestriction.h"
#include "SurToolGeoTx.h"
#include "SurToolRoad.h"
#include "SurToolMiniDetaile.h"
#include "SurToolSource.h"
#include "SurToolWaves.h"
#include "SurToolAnchor.h"
#include "SurToolGrass.h"

#include <algorithm>

// ������� �� �������!  ����� ��� ��������: _VISTA_ENGINE_EXTERNAL_

BEGIN_MESSAGE_MAP(CToolsTreeCtrl, CTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnTvnSelchanged)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRClick)
	ON_COMMAND(ID_POPUP_DELETE, OnPopupDelete)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnTvnEndlabeledit)
    ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnTvnBeginLabelEdit)
	ON_COMMAND(ID_POPUP_CREATECOLORPIC, OnPopupCreateColorPic)
	ON_COMMAND(ID_POPUP_CREATE3DMONWORLD, OnPopupCreate3dmOnWorld)
	ON_COMMAND(ID_POPUP_ADDM3D, OnPopupAddm3d)
	ON_COMMAND(ID_POP_ADDCOLORPIC, OnPopAddColorPic)
	ON_COMMAND(ID_POP_ADD_M3D_2W, OnPopAddM3d2w)
	ON_WM_VSCROLL()
	ON_COMMAND(ID_POP_SORT, OnPopSort)
	ON_WM_DESTROY()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

class CSurTool3DM_2W;

REGISTER_CLASS(CSurToolBase, CSurToolEmpty,      "Folder");
REGISTER_CLASS(CSurToolBase, CSurToolGeoNet,     "Geo");
REGISTER_CLASS(CSurToolBase, CSurToolBlur,       "Blur");
REGISTER_CLASS(CSurToolBase, CSurToolImp,        "Imp");
REGISTER_CLASS(CSurToolBase, CSurToolToolzer,    "Toolzer");
REGISTER_CLASS(CSurToolBase, CSurTool3DM,        "3DM");
REGISTER_CLASS_CONVERSION(CSurToolBase, CSurToolEnvironment, "Environment Model", typeid(CSurTool3DM_2W).name());
REGISTER_CLASS(CSurToolBase, CSurToolWater,      "Water");
REGISTER_CLASS(CSurToolBase, CSurToolKind,       "Hardness");
REGISTER_CLASS(CSurToolBase, CSurToolColorPic,   "Texture");
REGISTER_CLASS(CSurToolBase, CSurToolUnit,       "Unit");
REGISTER_CLASS(CSurToolBase, CSurToolUnitFolder, "Unit Folder");
REGISTER_CLASS(CSurToolBase, CSurToolCamera,     "Camera");
REGISTER_CLASS(CSurToolBase, CSurToolCameraRestriction,
			   									 "Camera Restriction");
REGISTER_CLASS(CSurToolBase, CSurToolGeoTx,      "Geo Texture");
REGISTER_CLASS(CSurToolBase, CSurToolRoad,       "Road");
REGISTER_CLASS(CSurToolBase, CSurToolMiniDetail, "Detail");
REGISTER_CLASS(CSurToolBase, CSurToolSource,     "Source");
REGISTER_CLASS(CSurToolBase, CSurToolAnchor,     "Anchor");
REGISTER_CLASS(CSurToolBase, CSurToolWaves,      "Waves");
REGISTER_CLASS(CSurToolBase, CSurToolGrass,      "Grass");

static const char* treeToolDefaultCfgFName=".\\Scripts\\Engine\\VistaEngine.scr";
static const char* treeToolCfgFName=".\\Scripts\\Content\\VistaEngine.scr";
static const char* treeToolCfgSection="Tree777";

IMPLEMENT_DYNAMIC(CToolsTreeCtrl, CTreeCtrl)
CToolsTreeCtrl::CToolsTreeCtrl()
: mainFrame_(0)
, popupMenu_(new PopupMenu(ClassCreatorFactory<CSurToolBase>::instance().size() + 5))
{
	pCurrentNode=0;
	flag_tree_build=0; //������� ��� ������ �� ���������
	curTreeItemRClick=NULL;

	if(XStream(0).open(treeToolCfgFName, XS_IN)){
		XPrmIArchive ia(treeToolCfgFName);
		ia.serialize(*this, treeToolCfgSection, 0);
	}
	else if(XStream(0).open(treeToolDefaultCfgFName, XS_IN)){
		XPrmIArchive(treeToolDefaultCfgFName).serialize(*this, treeToolCfgSection, 0);
	}

}

CToolsTreeCtrl::~CToolsTreeCtrl()
{
	mainFrame_=0;
	flag_tree_build=0; //������� ��� ������ �� ���������
	save();
}

void CToolsTreeCtrl::save()
{
	XPrmOArchive oa(treeToolCfgFName);
	oa.serialize(*this, treeToolCfgSection, 0);
}

void CToolsTreeCtrl::init(CMainFrame* mainFrame)
{
	mainFrame_ = mainFrame;
}

BOOL CToolsTreeCtrl::DestroyWindow()
{
	flag_tree_build=0; //������� ��� ������ �� ���������
	return CTreeCtrl::DestroyWindow();
}

enum e_BitmapTreeState{
	BTS_Folder=0,
	BTS_OpenFolder=0,
	BTS_Tool=1,
	BTS_OpenTool=1
};
// CToolsTreeCtrl message handlers

void CToolsTreeCtrl::OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	*pResult = 0;

	ASSERT(mainFrame_);

	HTREEITEM curHTI;

	CWaitCursor wait;

	destroyToolWindows();

	curHTI=pNMTreeView->itemNew.hItem;
	if(curHTI == 0)
		return;

	pCurrentNode = findNode(curHTI);
	CToolsTreeWindow* window = safe_cast<CToolsTreeWindow*>(GetParent());
	window->replaceEditorMode(pCurrentNode);
}

void CToolsTreeCtrl::DestroyAllTreeWndAux(CSurToolBase* node)
{
	if(!node)
		return;
	if(::IsWindow(node->GetSafeHwnd())) 
		node->DestroyWindow();
	vector< ShareHandle<CSurToolBase> >::iterator p;
	for(p = node->children().begin(); p != node->children().end(); p++)
		DestroyAllTreeWndAux(*p);
}
void CToolsTreeCtrl::destroyToolWindows()
{
	vector< ShareHandle<CSurToolBase> >::iterator p;
	FOR_EACH(tools_, p)
		DestroyAllTreeWndAux(*p);
	CMainFrame* mainFrame = (CMainFrame*)(AfxGetMainWnd());
}


void CToolsTreeCtrl::DeleteTreeNode(CSurToolBase* pDeleteNode)
{
	pCurrentNode=0; //���!!!
	vector< ShareHandle<CSurToolBase> >::iterator p;
	for(p = tools_.begin(); p != tools_.end();){
		(*p)->deleteChildNode(pDeleteNode);
		if(*p==pDeleteNode) {
			//delete pDeleteNode;
			p=tools_.erase(p);
		}
		else p++;
	}
}


CSurToolBase* CToolsTreeCtrl::findNodeAux(CSurToolBase* node, HTREEITEM treeItem)
{
	if(node->treeItem() == treeItem)
		return node;
	vector< ShareHandle<CSurToolBase> >::iterator p;
	for(p = node->children().begin(); p != node->children().end(); p++){
		CSurToolBase* result = findNodeAux(*p, treeItem);
		if(result)
			return result;
	}
	return 0;
}

CSurToolBase* CToolsTreeCtrl::findNode(HTREEITEM node)
{
	vector< ShareHandle<CSurToolBase> >::iterator p;
	FOR_EACH(tools_, p){
		CSurToolBase* result=findNodeAux(*p, node);
		if(result) return result;
	}
	return 0;
}

void CToolsTreeCtrl::buildNode(CSurToolBase* node, HTREEITEM parent, TV_INSERTSTRUCT& ins)
{
	ins.hParent = parent;
    if(node->isLabelTranslatable())
        ins.item.pszText = (LPSTR)TRANSLATE(node->name());
    else
        ins.item.pszText = (LPSTR)node->name();

	switch(node->iconInSurToolTree){
	case IconISTT_FolderTools:
		ins.item.iImage = BTS_Folder;
		ins.item.iSelectedImage = BTS_OpenFolder;
		break;
	case IconISTT_Tool:
		ins.item.iImage = BTS_Tool;
		ins.item.iSelectedImage = BTS_OpenTool;
		break;
	case IconISTT_Other:
		ins.item.iImage = BTS_Tool;
		ins.item.iSelectedImage = BTS_OpenTool;
		break;
	}

	HTREEITEM htiCurLoc = InsertItem(&ins);
	node->setTreeItem(htiCurLoc);
	vector< ShareHandle<CSurToolBase> >::iterator p;
	for(p = node->children().begin(); p != node->children().end(); ++p){
		ins.item.state     = TVIS_EXPANDED|TVIS_EXPANDEDONCE;
        ins.item.stateMask = TVIS_EXPANDED|TVIS_EXPANDEDONCE; // | TVIS_BOLD
		buildNode(*p, htiCurLoc, ins);
	}
}

void CToolsTreeCtrl::BuildTree()
{
	flag_tree_build=0; //������� ��� ������ �� ���������
	DeleteAllItems();

	TV_INSERTSTRUCT ins;
	ins.hParent = TVI_ROOT;//m_hTreeItem;
	ins.hInsertAfter = TVI_LAST;
	ins.item.mask = TVIF_TEXT | TVIF_STATE;
	ins.item.pszText = (LPTSTR)"rrrr";//qd_gameD->name();//(LPTSTR) "IGRA";
	ins.item.state = ins.item.stateMask = 0;//TVIS_EXPANDED; // | TVIS_BOLD

    ins.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	vector< ShareHandle<CSurToolBase> >::iterator p;
	FOR_EACH(tools_, p){
		if(*p){
			(*p)->FillIn();
			ins.item.state = 0;
			ins.item.stateMask = 0;//TVIS_EXPANDED; // | TVIS_BOLD
			buildNode(*p, TVI_ROOT, ins );
		}
	}

	flag_tree_build=1; //������ ���������

	Invalidate();
}

int CToolsTreeCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	ASSERT(mainFrame_); //not  call fuction init() !

#ifdef EXTENDED_COLOR_BITMAP_IN_CONTROL
	MakeVoluntaryImageList(IDB_TREETOOLDLG_BITMAP256, ILC_COLOR24, 4, m_ImgList);
	SetImageList(&m_ImgList, TVSIL_NORMAL);
#else
	m_ImgList.Create(IDB_TREETOOLDLG_BITMAP, 16, 0, RGB(255,0,255));
	SetImageList(&m_ImgList, TVSIL_NORMAL);
#endif

	BuildTree();
	return 0;
}

#include <windowsx.h> //for GET_X_LPARAM & GET_Y_LPARAM

void CToolsTreeCtrl::OnRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 1;

	CPoint curPoint(GET_X_LPARAM(GetMessagePos()), GET_Y_LPARAM(GetMessagePos()));
	ScreenToClient(&curPoint);
	UINT uFlags;
	HTREEITEM item = HitTest(curPoint, &uFlags);
	CSurToolBase* tool = 0;
	if(item != TVI_ROOT && item != 0)
		 tool = findNode(item);
	
	if(item){
		curTreeItemRClick = item;
		CString str = GetItemText (curTreeItemRClick);
		Select(item, TVGN_CARET);
		SelectItem(item);
		str = GetItemText (curTreeItemRClick);
	}
	else
		curTreeItemRClick=NULL;

	CMenu popMenu;
	popMenu.LoadMenu(IDR_POPUP_TREE);

	typedef ClassCreatorFactory<CSurToolBase> CF;
	typedef CF::CreatorBase CreatorType;

	const ComboStrings& strings = CF::instance().comboStringsAlt();

	popupMenu().clear();
	PopupMenuItem& root = popupMenu().root();
	PopupMenuItem& createItem = root.add(TRANSLATE("&�������"));
	
	ComboStrings::const_iterator it;
	int index = 0;
	FOR_EACH(strings, it){
		createItem
			.add(it->c_str())
			.connect(bindArgument(bindMethod(*this, &CToolsTreeCtrl::onMenuCreateTool), index++));
	}
	root.addSeparator();
	root.add(TRANSLATE("&�������"))
		.connect(bindArgument(bindMethod(*this, &CToolsTreeCtrl::onMenuDeleteTool), tool))
		.enable(tool != 0);
	root.addSeparator();
	root.add(TRANSLATE("&��������"))
		.connect(bindArgument(bindMethod(*this, &CToolsTreeCtrl::onMenuProperties), tool))
		.enable(tool != 0);
	
	CPoint posMouse(GET_X_LPARAM(GetMessagePos()), GET_Y_LPARAM(GetMessagePos()));

	if(flag_TREE_BAR_EXTENED_MODE)
		popupMenu().spawn(posMouse, GetSafeHwnd());
	else {
		if(tool){
			switch(tool->getPopUpMenuRestriction()){
			case PUMR_PermissionAll:
				popMenu.GetSubMenu(0)->TrackPopupMenu(0,posMouse.x,posMouse.y,this);
				break;
#ifndef _VISTA_ENGINE_EXTERNAL_
			case PUMR_Permission3DM:
				popMenu.GetSubMenu(1)->TrackPopupMenu(0,posMouse.x,posMouse.y,this);
				break;
			case PUMR_Permission3DM_2W:
				popMenu.GetSubMenu(2)->TrackPopupMenu(0,posMouse.x,posMouse.y,this);
				break;
#endif
			case PUMR_PermissionColorPic:
				popMenu.GetSubMenu(3)->TrackPopupMenu(0,posMouse.x,posMouse.y ,this);
				break;
			case PUMR_PermissionZone:
				popMenu.GetSubMenu(6)->TrackPopupMenu(0,posMouse.x,posMouse.y,this);
				break;
			case PUMR_PermissionEffect:
				popMenu.GetSubMenu(7)->TrackPopupMenu(0,posMouse.x,posMouse.y,this);
				break;
			case PUMR_PermissionToolzerBlurGeo:
				break;
			case PUMR_PermissionDelete:
				popMenu.GetSubMenu(5)->TrackPopupMenu(0,posMouse.x,posMouse.y,this);
				break;
			case PUMR_NotPermission:
			default:
				break;
			}
		}
	}
}


void CToolsTreeCtrl::AddNode2Tree(CSurToolBase* tool)
{
	CSurToolBase* pNode;
	if(curTreeItemRClick==TVI_ROOT || (curTreeItemRClick==NULL) || (pNode=findNode(curTreeItemRClick))==0){
		tools_.push_back(tool);
	}
	else {
		pNode->children().push_back(tool);
	}
	BuildTree();
}

void CToolsTreeCtrl::OnPopupCreate3dm()
{
	CSurToolBase* tool = new CSurTool3DM;
	tool->setName("3DM");
	AddNode2Tree(tool);
}

void CToolsTreeCtrl::OnPopupCreate3dmOnWorld()
{
	CSurToolBase* tool = new CSurToolEnvironment;
	tool->setName("3DM_2W");
	AddNode2Tree(tool);
}

void CToolsTreeCtrl::OnPopupCreateColorPic()
{
	CSurToolBase* tool=new CSurToolColorPic;
	tool->setName("Color picture");
	AddNode2Tree(tool);
}

void CToolsTreeCtrl::OnPopupDelete()
{
	CSurToolBase* pNode;
	if( curTreeItemRClick!=TVI_ROOT && (curTreeItemRClick!=NULL) && (pNode=findNode(curTreeItemRClick))!=0 ){
		destroyToolWindows();
		DeleteTreeNode(pNode);
	}
	BuildTree();
}

//////////////////////////////////////////////////////////////////////

void CToolsTreeCtrl::OnTvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	*pResult = 0;
	
	CSurToolBase* pCurNode;
	pCurNode=findNode(pTVDispInfo->item.hItem);
	if(pCurNode) {
        if(pTVDispInfo->item.pszText)
            pCurNode->setName(pTVDispInfo->item.pszText);
    }
	BuildTree();
}

void CToolsTreeCtrl::OnTvnBeginLabelEdit (NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
    CSurToolBase* pCurNode = findNode(pTVDispInfo->item.hItem);
    xassert (pCurNode != 0);

	if(flag_TREE_BAR_EXTENED_MODE || pCurNode->isLabelEditable())
		*pResult = FALSE; // ��������� �������������� ����
	else 
		*pResult = TRUE; // ��������� �������������� ����
    return;
}

///////////////////////////////////////////////////////////////////////////
void CToolsTreeCtrl::Create_PushBrowse_And_DestroyIfErr(CSurToolBase* tool)
{
	HTREEITEM curHTI;
	destroyToolWindows();

	curHTI = tool->treeItem();
	if( !tool->CreateExt( mainFrame_->getResizableBarDlg()	)) {
		TRACE0("Failed to create ....\n");
	}

	tool->SendMessage(WM_COMMAND, MAKEWPARAM(IDC_BTN_BROWSE_FILE ,BN_CLICKED), (LPARAM)tool->GetDlgItem(IDC_BTN_BROWSE_FILE)->GetSafeHwnd());
	string tmp=tool->dataFileName;
	if(!tmp.empty()){
		string::size_type i=tmp.find_last_of("\\");
		string::size_type j=tmp.find_last_of(".");
		if(i!=string::npos){
			if(j!=string::npos && i<j){
				tool->setName(string(tmp, i+1, j-i-1).c_str());
			}
			else
                tool->setName(&(tmp.c_str()[i+1]));
		}
		else 
			tool->setName(tmp.c_str());
		AddNode2Tree(tool);
		SelectItem(tool->treeItem());
	}
	else {
		tool->DestroyWindow();
		delete tool;
	}
}

void CToolsTreeCtrl::OnPopAddColorPic()
{
	CSurToolBase* tool=new CSurToolColorPic;
	Create_PushBrowse_And_DestroyIfErr(tool);
}

void CToolsTreeCtrl::OnPopupAddm3d()
{
	CSurToolBase* tool=new CSurTool3DM;
	Create_PushBrowse_And_DestroyIfErr(tool);
}

void CToolsTreeCtrl::OnPopAddM3d2w()
{
	CSurToolBase* tool=new CSurToolEnvironment;
	Create_PushBrowse_And_DestroyIfErr(tool);
}

void CToolsTreeCtrl::serialize(Archive& ar)
{
	CSurToolColorPic::staticSerialize(ar);
	//ar.serialize(CSurToolEnvironment::staticSettings, "environmentStaticSettings", 0);

	ar.serialize(flag_tree_build, "flag_tree_build", 0);
	ar.serializeArray(shortcuts_, "shortcuts", 0);
	ar.serialize(tools_, "treeSDTB", 0);
}

////////////////////////////////////////////////////

void CToolsTreeCtrl::onMenuProperties(CSurToolBase* tool)
{
	if(tool){
		EditArchive ea;
		ea.setTranslatedOnly(false);
		if(ea.edit(*tool))
			BuildTree();
	}
}

struct SurToolSorter{
	inline bool operator()(const CSurToolBase* lhs, const CSurToolBase* rhs){
		return (std::string(lhs->name()) < std::string(rhs->name()));
	}
};

void CToolsTreeCtrl::OnPopSort()
{
	if (CSurToolBase* node = getSelectedTool()) {
		std::stable_sort (node->children().begin(), node->children().end(), SurToolSorter());

		BuildTree ();
	}
}

void CToolsTreeCtrl::onMenuCreateTool(int index)
{
	typedef ClassCreatorFactory<CSurToolBase> CF;
	int typesCount = CF::instance().size();
	xassert(index >= 0 && index < typesCount);

	CF::ClassCreatorBase& creator = CF::instance().findByIndex(index);
	CSurToolBase* tool = creator.create();
	if(strlen(tool->name()) == 0)
	   tool->setName(creator.nameAlt());
	AddNode2Tree(tool);
}

void CToolsTreeCtrl::onMenuDeleteTool(CSurToolBase* tool)
{
	if(tool){
		destroyToolWindows();
		DeleteTreeNode(tool);
	}
	BuildTree();
}

void CToolsTreeCtrl::OnDestroy()
{
	destroyToolWindows();

	CTreeCtrl::OnDestroy();
}

BOOL CToolsTreeCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	popupMenu_->onCommand(wParam, lParam);

	return CTreeCtrl::OnCommand(wParam, lParam);
}

CSurToolBase* CToolsTreeCtrl::findTool(const ComboStrings& path)
{
	int level = 0;
	SurTools* currentList = &tools_;
	while(level < path.size()){
		SurTools::iterator it;
		FOR_EACH(*currentList, it){
			if(path[level] == (*it)->name()){
				if(level == path.size() - 1)
					return *it;
				else{
					++level;
					currentList = &(*it)->children();
					break;
				}
			}
		}
		if(it == currentList->end())
			return 0;
	}
	return 0;
}
bool CToolsTreeCtrl::getToolPathAux(ComboStrings& path, SurTools* tools, CSurToolBase* tool)
{
	SurTools::iterator it;
	FOR_EACH(*tools, it){
		path.push_back((*it)->name());
		if(*it == tool){
			return true;
		}
		else{
			if(getToolPathAux(path, &(*it)->children(), tool))
				return true;
		}
		path.pop_back();
	}
	return false;
}

bool CToolsTreeCtrl::getToolPath(ComboStrings& path, CSurToolBase* tool)
{
	path.clear();
	return getToolPathAux(path, &tools_, tool);
}
void CToolsTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CWnd* parent = GetParent();
	if(parent->IsKindOf(RUNTIME_CLASS(CToolsTreeWindow))){
		CToolsTreeWindow* window = static_cast<CToolsTreeWindow*>(parent);
		window->onKeyDown(nChar, nFlags);
	}

	CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}
