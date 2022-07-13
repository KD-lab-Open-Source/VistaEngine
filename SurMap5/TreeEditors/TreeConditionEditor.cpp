#include "StdAfx.h"

#include "TreeEditor.h"
#include "EditableCondition.h"

#include "EditArchive.h"
#include "ConditionEditorDlg.h"

class TreeConditionEditor : public TreeEditorImpl<TreeConditionEditor, EditableCondition> {
public:
    bool invokeEditor(EditableCondition& condition, HWND wnd) {
		
		EditableCondition editedCondition;
		// копируем, для возможной отмены
		EditOArchive oar;
		oar.serialize(condition, "condition", "condition");
		EditIArchive iar(oar);
		iar.serialize(editedCondition, "condition", "condition");

		CConditionEditorDlg dlg(editedCondition, CWnd::FromHandle(wnd));
		if(dlg.DoModal() == IDOK)
			condition = dlg.condition();
	    return true;
    }
    bool hideContent () const { return true; }
	Icon buttonIcon()const { return TreeEditor::ICON_DOTS; }

    bool prePaint (HDC dc, RECT rt) {

        return false;
    }

	std::string nodeValue () const { 
		return "";
	}

};

REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(EditableCondition).name(), TreeConditionEditor);
