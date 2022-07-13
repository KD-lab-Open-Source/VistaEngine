#include "StdAfx.h"
#include <typeinfo>

#include "TreeEditor.h"
#include "..\..\AttribEditor\AttribColorComboBox.h"
#include "EditArchive.h"
#include "ComboListColor.h"

typedef ComboListColor DataType;

inline COLORREF toColorRef (const sColor4f& color) {
	return RGB(round(color.r * 255.0f), round(color.g * 255.0f), round(color.b * 255.0f));
}

class TreeColorComboEditor : public TreeEditorImpl<TreeColorComboEditor, DataType> {
public:
	TreeColorComboEditor () {
	}
	bool invokeEditor (DataType&, HWND parent) {
		return false;
	}
	bool hideContent () const {
		return true;
	}
	Icon buttonIcon()const { return TreeEditor::ICON_DROPDOWN; }

	bool prePaint (HDC dc, RECT rt) {
        sColor4f color = getData ();
        InflateRect (&rt, -1, -1);
        rt.bottom -= 1;
        HBRUSH hColorBrush = ::CreateSolidBrush (toColorRef (color));
        ::FillRect (dc, &rt, hColorBrush);
        ::DeleteObject (hColorBrush);
		return true;
	}
    CWnd* beginControl (CWnd* parent, const CRect& rect) {
		CAttribColorComboBox* pCombo = new CAttribColorComboBox ();
		assert(pCombo);
        CRect rcComboRect (rect);
        rcComboRect.bottom += 200;
		if (! pCombo->Create (WS_VISIBLE | WS_CHILD | WS_VSCROLL |
							  CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS,
                              rcComboRect, parent, 0))
        {
            delete pCombo;
            return 0;
        }
		int selection = 0;
		const ComboListColor& colorList = getData ();
		ColorContainer::const_iterator it;
		int index = 0;
		FOR_EACH (colorList.comboList (), it) {
			COLORREF currentColor = toColorRef (*it);
			pCombo->AddColour ("", currentColor);
			if (currentColor == toColorRef (colorList.value ())) {
				selection = index;
			}
			++index;
		}
		pCombo->SetCurSel (selection);
		return pCombo;
    }
    void endControl (DataType& data, CWnd* control) {
		CString strBuffer;
		int curSel = static_cast<CAttribColorComboBox*>(control)->GetCurSel ();
		ColorContainer::const_iterator it = data.comboList ().begin ();
		std::advance (it, curSel);
		data = *it;
    }
};

REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid (ComboListColor).name (), TreeColorComboEditor);
