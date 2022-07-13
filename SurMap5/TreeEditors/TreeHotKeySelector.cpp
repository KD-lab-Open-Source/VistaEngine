#include "XUtil.h"
#include "StdAfx.h"
#include "TreeEditor.h"
#include "SystemUtil.h"
#include "EditArchive.h"

#include "..\UserInterface\Controls.h"
#include "HotKeySelectorDlg.h"

//////////////////////////////////////////////////////////////////////////////
class TreeHotKeySelector : public TreeEditorImpl<TreeHotKeySelector, sKey> {
public:
    bool invokeEditor (sKey& key, HWND parent){
		CHotKeySelectorDlg dlg(key, CWnd::FromHandle(parent));
		if(dlg.DoModal() == IDOK){
			key = dlg.key();
		}
        return true;
    }
    std::string nodeValue() const{
		return getData().toString();
    }
	bool canBeCleared() const { return true; }
    void onClear(sKey& key){
		key = sKey();
	}
    bool hideContent () const { return true; }
	Icon buttonIcon()const { return TreeEditor::ICON_DOTS; }
};

REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid (sKey).name (), TreeHotKeySelector);
