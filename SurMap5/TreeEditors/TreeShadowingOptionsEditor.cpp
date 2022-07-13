#include "StdAfx.h"
#include <typeinfo>

#include "TreeEditor.h"
#include "..\AttribEditor\AttribComboBox.h"
#include "EditArchive.h"
#include "..\Water\Water.h"
#include "Dictionary.h"
#include "..\Water\SkyObject.h"

namespace {

struct ShadowingOptionsPreset {
    const char* name;
    ShadowingOptions options;
};

ShadowingOptionsPreset gb_shadowingPresets[] = {
    { "Песок 1", ShadowingOptions (0.1f, 0.5f, 0.5f) },
    { "Песок 2", ShadowingOptions (0.5f, 0.5f, 0.5f) },
    { "Песок 3", ShadowingOptions (0.9f, 0.5f, 0.5f) },
    
	{ "Лёд 1",   ShadowingOptions (0.1f, 0.8f, 0.3f) },
	{ "Лёд 2",   ShadowingOptions (0.5f, 0.8f, 0.3f) },
	{ "Лёд 3",   ShadowingOptions (0.9f, 0.8f, 0.3f) },

	{ "Металл 1", ShadowingOptions (0.1f, 0.25f, 0.9f) },
	{ "Металл 2", ShadowingOptions (0.5f, 0.25f, 0.9f) },
	{ "Металл 3", ShadowingOptions (0.9f, 0.25f, 0.9f) },

	{ "Обыч. 1", ShadowingOptions (0.1f, 0.5f, 1.0f) },
	{ "Обыч. 2", ShadowingOptions (0.5f, 0.5f, 1.0f) },
	{ "Обыч. 3", ShadowingOptions (0.9f, 0.5f, 1.0f) },

	{ "Initial", ShadowingOptions (0.75f, 0.2f, 1.0f) }
};

};

class TreeShadowingOptionsEditor : public TreeEditorImpl<TreeShadowingOptionsEditor, ShadowingOptions> {
public:
	TreeShadowingOptionsEditor () {
	}
	bool hideContent () const {
		return true;
	}
    CWnd* beginControl (CWnd* parent, const CRect& rect) {
		CAttribComboBox* pCombo = new CAttribComboBox ();
		assert(pCombo);
        CRect rcComboRect (rect);
        rcComboRect.bottom += 200;
		if (! pCombo->Create (WS_VISIBLE | WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
                              rcComboRect, parent, 0))
        {
            delete pCombo;
            return 0;
        }
		int selection = -1;
        int count = sizeof (gb_shadowingPresets) / sizeof (gb_shadowingPresets[0]);
        for (int index = 0; index < count; ++index) {
            if (gb_shadowingPresets[index].options == getData ())
                selection = index;
            pCombo->InsertString (-1, TRANSLATE (gb_shadowingPresets[index].name));
        }
		pCombo->SetCurSel (selection);
		return pCombo;
    }
    void endControl (ShadowingOptions& data, CWnd* control) {
		int curSel = static_cast<CAttribComboBox*>(control)->GetCurSel ();
        data = gb_shadowingPresets[curSel].options;
        /*
		CString strBuffer;
		ColorContainer::const_iterator it = data.comboList ().begin ();
		std::advance (it, curSel);
		data = *it;
        */
    }
    std::string nodeValue () const {
        int count = sizeof (gb_shadowingPresets) / sizeof (gb_shadowingPresets[0]);
        for (int index = 0; index < count; ++index) {
            if (gb_shadowingPresets[index].options == getData ())
                return gb_shadowingPresets[index].name;
        }
		return "";
    }
};

REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid (ShadowingOptions).name (), TreeShadowingOptionsEditor);
