#include "stdafx.h"
#include <algorithm>

#include "TreeInterface.h"
#include "..\AttribEditor\AttribEditorDlg.h"
#include "..\AttribEditor\AttribEditorCtrl.h"
#include "..\AttribEditor\DLL\TreeNodeStorageImpl.h"

#include "Dictionary.h"

class AttribEditorInterfaceImpl : public AttribEditorInterface{
public:
	bool isTreeEditorRegistered(const char* name);
    TreeEditor* createTreeEditor(const char* name);
	void free(TreeNode const* node);
    TreeNode* edit(const TreeNode* node, HWND wnd, TreeControlSetup& treeControlSetup);
	TreeNodeStorage& treeNodeStorage();
	TreeNodeClipboard& clipboard();
private:
	typedef std::list< ShareHandle< CAttribEditorDlg > > Dialogs;
	Dialogs dialogs_;
};

bool AttribEditorInterfaceImpl::isTreeEditorRegistered(const char* name)
{
	return (TreeEditorFactory::instance().find(name) != 0);
}

TreeEditor* AttribEditorInterfaceImpl::createTreeEditor(const char* name)
{
	return TreeEditorFactory::instance().create(name, true);
}

void AttribEditorInterfaceImpl::free(TreeNode const* node)
{
	for(Dialogs::iterator it = dialogs_.begin(); it != dialogs_.end();){
		if((*it)->rootNode() == node){
			it = dialogs_.erase(it);
			return;
		}
		else
			++it;
	}
	xassert(0);
}

TreeNode* AttribEditorInterfaceImpl::edit(const TreeNode* node, HWND wnd, TreeControlSetup& treeControlSetup)
{
	ShareHandle<CAttribEditorDlg> dialog = new CAttribEditorDlg(CWnd::FromHandle(wnd));
	if(ShareHandle<TreeNode> editedNode = const_cast<TreeNode*>(dialog->edit(node, wnd, treeControlSetup))){
		dialogs_.push_back(dialog);
		return editedNode;
	}
	else
		return 0;
}

TreeNodeStorage& AttribEditorInterfaceImpl::treeNodeStorage()
{
	return TreeNodeStorageImpl::instance();
}

TreeNodeClipboard& AttribEditorInterfaceImpl::clipboard()
{
	return TreeNodeClipboard::instance();
}

AttribEditorInterface& attribEditorInterface(){
    static AttribEditorInterfaceImpl impl;
    return impl;
} 
