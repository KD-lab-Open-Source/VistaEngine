#include "stdafx.h"

#include <stack>
#include "AttribEditorDlg.h"
#include "TreeInterface.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

typedef std::stack< ShareHandle<TreeNode> > Trees;

Trees& getTrees()
{
	static Trees trees;
	return trees;
}

extern ATTRIB_EDITOR_API AttribEditorInterface& attribEditorInterfaceExternal()
{
	AttribEditorInterface& attribEditorInterface();
	return attribEditorInterface();
}

/*
//  опирует дерево, возвращает указатель на статические данные, либо 0.
extern "C" DLL_API TreeNode const* treeControlEdit(TreeNode const * treeNode, 
                                                   HWND hwnd, TreeControlSetup& treeControlSetup)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	Trees& trees = getTrees();
    
	CAttribEditorDlg dlg(CWnd::FromHandle(hwnd));
	ShareHandle<TreeNode> editedNode = const_cast<TreeNode*>(dlg.Edit(treeNode, hwnd, treeControlSetup));
	if(!editedNode)
		return 0;
	trees.push(editedNode);
    return trees.top();
}

// ќчищает статические данные
extern "C" DLL_API void treeControlFree(TreeNode const* p)
{
	Trees& trees = getTrees();
	xassert(trees.top() == p);
	trees.pop();
}

*/
