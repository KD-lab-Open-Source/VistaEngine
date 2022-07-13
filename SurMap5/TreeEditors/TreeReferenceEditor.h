#include "TreeEditor.h"
#include "..\AttribEditor\AttribComboBox.h"
#include "..\AttribEditor\AttribEditorDlg.h"

#include "EditArchive.h"
#include "TypeLibrary.h"
#include "Dictionary.h"

#include "..\Units\UnitAttribute.h"

template<class ReferenceType>
class TreeReferenceComboBox : public TreeEditorImpl<TreeReferenceComboBox<ReferenceType>, ReferenceType> {
public:
	bool spawn_editor_;

	TreeReferenceComboBox () {
		spawn_editor_ = false;
	}	

	bool invokeEditor (ReferenceType& reference, HWND parent) { return false; }

    CWnd* beginControl (CWnd* parent, const CRect& itemRect);
    void endControl (ReferenceType& reference, CWnd* control);
	
	bool hideContent () const { return true; }
	Icon buttonIcon() const{
		return TreeEditor::ICON_DROPDOWN;
	}
    bool prePaint (HDC dc, RECT rt) { return false; }
	virtual std::string nodeValue () const { 
		return getData().c_str();
	}

};

template<class ReferenceType>
class CReferenceComboBox : public CAttribComboBox
{
public:
	CReferenceComboBox (TreeReferenceComboBox<ReferenceType>* editor)
		: CAttribComboBox ()
		, editor_ (editor)
	{}

	virtual void OnSelChange () {
	}
	TreeReferenceComboBox<ReferenceType>* editor_;
};


template<class ReferenceType>
CWnd* TreeReferenceComboBox<ReferenceType>::beginControl (CWnd* parent, const CRect& itemRect)
{
    const ReferenceType& reference = getData();

    CReferenceComboBox<ReferenceType>* pCombo = new CReferenceComboBox<ReferenceType> (this);
    CRect rcComboRect (itemRect);
    rcComboRect.bottom += 200;
    if (! pCombo->Create (WS_VISIBLE | WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
                            rcComboRect, parent, 0))
    {
        delete pCombo;
        return 0;
    }
	ComboStrings::const_iterator it;
	ComboStrings strings;
	splitComboList(strings, ReferenceType::StringTableType::instance().comboList());
    int index = CB_ERR;
    int selected = 0;
    FOR_EACH(strings, it){
        ++index;
        const char* value = it->c_str();
		if(strcmp(value, reference.c_str()) == 0)
			selected = index;
        pCombo->InsertString(-1, value);
        pCombo->SetItemData(index, index);
    }
    pCombo->SetCurSel(selected);

    pCombo->InsertString(-1, "[ Add... ]");
    pCombo->InsertString(-1, "[ Edit... ]");
    pCombo->InsertString(-1, "[ Library... ]");
	
    return pCombo;
}

AttribEditorInterface& attribEditorInterface();

template<class ReferenceType>
void TreeReferenceComboBox<ReferenceType>::endControl (ReferenceType& reference, CWnd* control)
{
    CReferenceComboBox<ReferenceType>* pCombo = static_cast<CReferenceComboBox<ReferenceType>*>(control);
    CString str;
    int sel = pCombo->GetCurSel ();

	TreeControlSetup tcs (0, 0, 400, 400, "Scripts\\TreeControlSetups\\StringTable", true, true);
	EditArchive ea(control->GetSafeHwnd(), tcs);

	if (sel == pCombo->GetCount() - 1) { // Library
		if(ea.edit(ReferenceType::StringTableType::instance())){
			ReferenceType::StringTableType::instance().saveLibrary();
			ReferenceType::StringTableType::instance().buildComboList();
		}
	} else if (sel == pCombo->GetCount() - 2) { // Edit
		ReferenceType::StringTableType::StringType& str = const_cast<ReferenceType::StringTableType::StringType&>(*reference);
		if(&str) {
			if(ea.edit(str))
				ReferenceType::StringTableType::instance().buildComboList();
		}
		else{
			AfxMessageBox(TRANSLATE("¬ыбрана пуста€ ссылка - редактировать нечего!"), MB_ICONEXCLAMATION | MB_OK);
		}
	} else if (sel == pCombo->GetCount() - 3) { // Add
		ReferenceType::StringTableType::StringType str;
		if(ea.edit(str)){
			ReferenceType::StringTableType::instance().add (str.c_str());
			ReferenceType new_reference(str.c_str());
			str.setStringIndex(new_reference->stringIndex());
			const_cast<ReferenceType::StringType&>(*new_reference) = str;
			reference = new_reference;
			ReferenceType::StringTableType::instance().buildComboList();
		}
	} else {
		ComboStrings strings;
		splitComboList(strings, ReferenceType::StringTableType::instance().comboList());
		if (sel >= 0 && sel < strings.size()){
			reference = ReferenceType(strings[sel].c_str());
		}
        else{
			reference = ReferenceType();
		}
	}
}
