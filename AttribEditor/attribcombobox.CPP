#include "StdAfx.h"

#include "..\Util\MFC\TreeListCtrl.h"

#include "AttribEditorCtrl.h"
#include ".\AttribComboBox.h"

BEGIN_MESSAGE_MAP(CAttribComboBox, CComboBox)
	ON_CONTROL_REFLECT(CBN_CLOSEUP, OnCloseUp)
	ON_CONTROL_REFLECT(CBN_SELENDOK, OnSelEndOk)
	ON_CONTROL_REFLECT(CBN_SELENDCANCEL, OnSelEndCancel)
	ON_CONTROL_REFLECT(CBN_SETFOCUS, OnSetFocus)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void CAttribComboBox::OnCloseUp()
{
}

CAttribComboBox::~CAttribComboBox()
{

}

void CAttribComboBox::OnSelEndOk()
{
	this->PostMessage(WM_KEYDOWN, VK_RETURN, 1);
}

void CAttribComboBox::OnSelEndCancel()
{
	this->PostMessage(WM_KEYDOWN, VK_ESCAPE, 1);
}

void CAttribComboBox::OnSetFocus()
{
	ShowDropDown (TRUE);
}


void CAttribComboBox::OnSelChange()
{

}

void CAttribComboBox::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);
	treeListCtrl().SetCtrlFocus(pNewWnd, FALSE);
}

CTreeListCtrl& CAttribComboBox::treeListCtrl()
{
	xassert(GetParent()->IsKindOf(RUNTIME_CLASS(CTreeListCtrl)));
	CTreeListCtrl* tree = static_cast<CTreeListCtrl*>(GetParent());
	xassert(tree);
	return *tree;
}
