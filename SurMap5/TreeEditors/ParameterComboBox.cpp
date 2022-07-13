#include "stdafx.h"
#include "..\Util\MFC\TreeListCtrl.h"
#include "..\render\inc\umath.h"
#include "ParameterComboBox.h"
#include "..\AttribEditor\AttribEditorDlg.h"
#include "FormulaEditorDlg.h"
#include "UnitAttribute.h"

ParameterValue& CParameterComboBox::selectedValue()
{
    ParameterValue* value = values_[getTree().GetSelectedItem()];
    if(!value)
        value = const_cast<ParameterValue*>(&*initialValue_);
    return *value;
}

BOOL CParameterComboBox::Create(ParameterValueReference reference, DWORD style, const CRect& rect, CWnd* parent, UINT id)
{
	initialValue_ = reference;

	if(!CTreeComboBox::Create(style, rect, parent, id))
        return FALSE;

	setText(reference.c_str());
	CTreeCtrl& tree = getTree();
	std::map<int, HTREEITEM> groupItems;

	const ParameterGroupTable::Strings& strings = ParameterGroupTable::instance().strings();
	ParameterGroupTable::Strings::const_iterator it;
	int index = 0;
	FOR_EACH(strings, it) {
		ParameterGroupReference ref(it->c_str());
		if (it == strings.begin()) {
			groupItems[ref.key()] = TVI_ROOT;
		} else {
            groupItems[ref.key()] = tree.InsertItem(it->c_str());
		}
	}
	HTREEITEM selectedItem = 0;
	{
		ParameterValueTable::Strings::iterator it;
		ParameterValueTable::Strings& strings = const_cast<ParameterValueTable::Strings&>(ParameterValueTable::instance().strings());
		
		FOR_EACH(strings, it) {
			ParameterValue& val = *it;

			std::map<int, HTREEITEM>::iterator git = groupItems.find(val.group().key());
			HTREEITEM parent = TVI_ROOT;
			if (git != groupItems.end())
				parent = git->second;
			HTREEITEM item = tree.InsertItem(val.c_str(), parent);
			values_[item] = &val;
			if(&val == &*reference)
				selectedItem = item;
		}
	}
	
	tree.SelectItem(selectedItem);
	tree.EnsureVisible(selectedItem);
	tree.ScrollWindow(-100, 0);
    return TRUE;
}

void CParameterComboBox::onItemSelected(HTREEITEM item)
{
	__super::onItemSelected(item);

	if (1 || ::IsWindowVisible (tree_.GetSafeHwnd ())) {
		CWnd* parent = GetParent();
		if (parent && parent->IsKindOf(RUNTIME_CLASS(CTreeListCtrl))) {
			CTreeListCtrl* tree = static_cast<CTreeListCtrl*>(GetParent ());
			tree->FinishModify ();
		}
		if (parent && parent->IsKindOf(RUNTIME_CLASS(CFormulaEditorDlg))) {
			CFormulaEditorDlg* dlg = static_cast<CFormulaEditorDlg*>(GetParent ());
			dlg->onParameterInserted(ParameterValueReference(selectedValue().c_str()));
			//tree->FinishModify ();
		}
	}
}

void CParameterComboBox::hideDropDown(bool byLostFocus)
{
	if(GetParent()->IsKindOf(RUNTIME_CLASS(CTreeListCtrl))) {
		if (byLostFocus) {

		} else {
			//CTreeListCtrl* tree = static_cast<CTreeListCtrl*>(GetParent ());
			//tree->FinishModify ();
		}
	} else {
		__super::hideDropDown(byLostFocus);
	}
}
