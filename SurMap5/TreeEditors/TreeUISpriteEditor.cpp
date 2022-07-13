#include "StdAfx.h"
#include "XUtil.h"
#include "umath.h"
#include "EditArchive.h"
#include "UISpriteEditorDlg.h"

#include "..\UserInterface\UI_Controls.h"
#include "..\UserInterface\UserInterface.h"
#include "TreeEditor.h"

class TreeUISpriteEditor : public TreeEditorImpl<TreeUISpriteEditor, UI_Sprite> {
public:
	bool invokeEditor (UI_Sprite& sprite, HWND parent) {
		CUISpriteEditorDlg dlg (sprite, CWnd::FromHandle(parent));
		dlg.DoModal ();
		return true;
	}/*
	bool postPaint (HDC dc, RECT rt) {
		return false;
	}
	*/
	Icon buttonIcon()const { return TreeEditor::ICON_DOTS; }
	bool hideContent () const {
		return true;
	}
    std::string nodeValue () const {
		return getData().textureReference().c_str();
    }
};

REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(UI_Sprite).name (), TreeUISpriteEditor);
