#include "StdAfx.h"
#include "EditArchive.h"

#include "FormulaEditorDlg.h"

class TreeParameterFormulaEditor : public TreeEditorImpl<TreeParameterFormulaEditor, ParameterFormulaString> {
public:
	bool invokeEditor (ParameterFormulaString& formula, HWND parent) {
		xassert(::IsWindow(parent));

		CFormulaEditorDlg dlg(formula, LookupParameter(formula.group_), formula.value_, CWnd::FromHandle(parent));
		if (dlg.DoModal () == IDOK) {
			(FormulaString&)formula = dlg.formula();
		}
		return true;
	}
    std::string nodeValue () const {
        return getData().c_str();
    }
	bool hideContent () const {
		return true;
	}
};

REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(ParameterFormulaString).name(), TreeParameterFormulaEditor);
