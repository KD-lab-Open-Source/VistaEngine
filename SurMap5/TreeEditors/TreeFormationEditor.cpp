#include "StdAfx.h"

#include "TreeEditor.h"

#include "EditArchive.h"

#include "..\Units\AttributeSquad.h"

#include "FormationEditorDlg.h"

class TreeFormationEditor : public TreeEditorImpl<TreeFormationEditor, FormationPattern> {
public:
    bool invokeEditor (FormationPattern& pattern, HWND parent) {
		CFormationEditorDlg dlg (pattern, CWnd::FromHandle(parent));
		if (dlg.DoModal () == IDOK) {
			pattern = dlg.pattern ();
		}
        return true;
    }
    bool hideContent () const { return true; }
	Icon buttonIcon()const { return TreeEditor::ICON_DOTS; }
    bool prePaint (HDC dc, RECT rt) {

        return false;
    }
	virtual std::string nodeValue () const { 
		return getData ().c_str ();
	}

};

REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid (FormationPattern).name (), TreeFormationEditor);
