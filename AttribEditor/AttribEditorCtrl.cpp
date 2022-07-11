#include "StdAfx.h"
#include "AttribEditorCtrl.h"
#include "kdw/Win32Proxy.h"
#include "kdw/PropertyTree.h"
#include "kdw/PropertyTreeModel.h"
#include "kdw/PropertyRow.h"
#include "Serialization/BinaryArchive.h"

BEGIN_MESSAGE_MAP(CAttribEditorCtrl, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CAttribEditorCtrl::CAttribEditorCtrl()
: propertyTree_(0)
, proxy_(0)
, style_(0)
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
}

BOOL CAttribEditorCtrl::Create(DWORD style, const CRect& rect, CWnd* parentWnd, UINT id)
{
	DWORD dwExStyle = 0;

    if(!CWnd::Create(className(), 0, style | WS_TABSTOP, rect, parentWnd, id, 0))
		return FALSE;

	return TRUE;
}


int CAttribEditorCtrl::initControl()
{
	proxy_ = new kdw::Win32Proxy(GetSafeHwnd());
	propertyTree_ = new kdw::PropertyTree();
	propertyTree_->signalBeforeApply().connect(this, &CAttribEditorCtrl::onBeforeApply);
	propertyTree_->signalChanged().connect(this, &CAttribEditorCtrl::onChanged);
	propertyTree_->setSelectFocused(true);
	proxy_->add(propertyTree_);
	setStyle(style_);
	proxy_->showAll();
	return 0;
}

void CAttribEditorCtrl::setStyle(int newStyle)
{
	style_ = newStyle;
	if(propertyTree_)
		propertyTree_->setCompact((style_ & COMPACT) != 0);
}

void CAttribEditorCtrl::attachSerializer(Serializer& s)
{
	xassert(propertyTree_);
	propertyTree_->attach(s);
	sers_.clear();
}

void CAttribEditorCtrl::mixIn(Serializer& holder)
{
	sers_.push_back(holder);
}

void CAttribEditorCtrl::showMix()
{
	xassert(propertyTree_);
	propertyTree_->attach(sers_);
	sers_.clear();
}

void CAttribEditorCtrl::detachData()
{
	xassert(propertyTree_);
	propertyTree_->detach();
	sers_.clear();
}

void CAttribEditorCtrl::loadExpandState(XBuffer& buffer)
{
	BinaryIArchive ia(buffer.buffer(), buffer.size());
	ia.setFilter(kdw::SERIALIZE_STATE);
	tree().serialize(ia);
}

void CAttribEditorCtrl::saveExpandState(XBuffer& buffer)
{
	BinaryOArchive oa;
	oa.setFilter(kdw::SERIALIZE_STATE);
	tree().serialize(oa);
    buffer.write(oa.data(), oa.size());
}

void CAttribEditorCtrl::serialize(Archive& ar)
{
	tree().serialize(ar);
}

bool CAttribEditorCtrl::getItemPath(ComboStrings& path, ItemType item)
{
	std::vector<ItemType> items;
	path.clear();
	while(item){
		items.push_back(item);
		item = item->parent();
	}
	if(!items.empty()){
		//if(style_ & HIDE_ROOT_NODE)
		//	path.push_back("");
		path.reserve(path.size() + items.size());
		for(int i = items.size() - 1; i >= 0; --i){
			bool isElement = item->parent() && item->parent()->isContainer();
			if(isElement){
				XBuffer buf;
				int index = std::distance(item->parent()->begin(),
					                      std::find(item->parent()->begin(),
										            item->parent()->end(), item));
				buf <= index;
				path.push_back(static_cast<const char*>(buf));
			}
			else{
				const char* text = item->name();
				path.push_back(text);
			}
		}
	}
	return !path.empty();
}

bool CAttribEditorCtrl::selectItemByPath(const ComboStrings& itemPath, bool resetSelection)
{
	if(resetSelection)
		tree().model()->deselectAll();

	ItemType rootItem = tree().model()->root();
	ItemType root = tree().model()->root();
	ItemType item = root;
	if(itemPath.empty())
		return 0;

	ComboStrings::const_iterator it = itemPath.begin();
	while(it != itemPath.end()){
		bool isContainer = false;
		if(root != rootItem){
			tree().expandRow(root);
			isContainer = root->isContainer();
		}
		
		if(root->empty())
			break;
		
		int child_index = 0;

		kdw::PropertyRow::iterator rit;
		FOR_EACH(*root, rit){
			ItemType item = safe_cast<ItemType>(&**rit);
			const char* text = item->name();
			int index = isContainer ? index = atoi(it->c_str()) : -1;
			if((child_index == index) || *it == text){
				++it;
				root = item;
				if(it == itemPath.end())
					goto break_loop;
				else
					break;
			}
			++child_index;
		}
		if(rit == root->end())
			break;
	}
break_loop:
	if(root == rootItem)
		return false;
	else{
		tree().model()->setFocusedRow(root);
		tree().model()->selectRow(root, true);
		tree().update();
		tree().ensureVisible(root);
		return true;
	}
	return false;
}


int CAttribEditorCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	return initControl();
}

void CAttribEditorCtrl::OnSize(UINT type, int cx, int cy)
{
	proxy_->_setPosition(Recti(0, 0, cx, cy));
}

void CAttribEditorCtrl::onBeforeApply()
{
	beforeResave();
}

