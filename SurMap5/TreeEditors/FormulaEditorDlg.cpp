#include "stdafx.h"

#include "FormulaEditorDlg.h"
#include "Parameters.h"
#include "Dictionary.h"


IMPLEMENT_DYNAMIC(CFormulaEditorDlg, CDialog)

CFormulaEditorDlg::CFormulaEditorDlg(const FormulaString& formula, FormulaString::LookupFunction lookupFunction, float x_value, CWnd* pParent)
: CDialog(CFormulaEditorDlg::IDD, pParent)
, formula_(formula)
, x_value_(x_value)
, lookupFunction_(lookupFunction)
{

}

CFormulaEditorDlg::~CFormulaEditorDlg()
{
}

void CFormulaEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FORMULA_EDIT, m_ctlEdit);
	//DDX_Control(pDX, IDC_ADD_COMBO, m_ctlAddCombo);
}


BEGIN_MESSAGE_MAP(CFormulaEditorDlg, CDialog)
	ON_EN_CHANGE(IDC_FORMULA_EDIT, OnFormulaEditChange)
	ON_BN_CLICKED(IDC_INSERT_BUTTON, OnInsertButtonClicked)
    ON_COMMAND_RANGE(IDM_FIRST_DYNAMIC_MENU, IDM_LAST_DYNAMIC_MENU, OnInsertVariable)
	ON_WM_SIZING()
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CFormulaEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	ComboStrings strings;
	splitComboList(strings, ParameterValueTable::instance().comboList());

	m_ctlEdit.SetEventMask (m_ctlEdit.GetEventMask() | ENM_CHANGE);
	m_ctlEdit.SetWindowText (formula_.c_str());
	updateSyntaxHighlighting();

	m_ctlEdit.SetFocus ();

	CRect rt;

	GetDlgItem(IDC_INSERT_BUTTON)->GetWindowRect(&rt);
	ScreenToClient(&rt);
	rt.right += 200;
	paramCombo_.Create(ParameterValueReference(), WS_CHILD, rt, this, 0);

	GetWindowRect(&rt);
	int cx = rt.Width();
	int cy = rt.Height();
	layout_.init(this);
	layout_.add(1, 1, 1, 1, IDC_FORMULA_EDIT);
	layout_.add(0, 0, 1, 1, &paramCombo_);
	layout_.add(0, 0, 1, 1, IDC_INSERT_BUTTON);
	layout_.add(0, 0, 1, 1, IDOK);
	layout_.add(0, 0, 1, 1, IDCANCEL);
	layout_.add(1, 0, 1, 1, IDC_H_LINE);

	return FALSE;
}

struct HighlightRange {
	HighlightRange (CRichEditCtrl& ctrl, const char* start, COLORREF color, int size = 0) 
		: control_ (ctrl)
		, color_ (color)
		, start_ (start)
		, size_ (size) {}

	inline void operator() (const char* start, const char* end) const {
		CHARRANGE old_sel;
		control_.GetSel (old_sel);
		control_.SetSel (start - start_, end - start_);
		CHARFORMAT cf;
		ZeroMemory (&cf, sizeof (cf));
		cf.cbSize = sizeof (cf);
		cf.dwMask = CFM_COLOR;
		if (size_ != 0) {
			cf.dwMask |= CFM_SIZE;
			cf.yHeight = size_ * 10;
		}

		cf.crTextColor = color_;
		control_.SetSelectionCharFormat (cf);
		control_.SetSel (old_sel);
	}

	COLORREF color_;
	int size_;
	mutable CRichEditCtrl& control_;
	const char* start_;
};

namespace{


const char* tokenEnd(const char* begin){
	if(*begin == '*')
		return ++begin;
	do{
		++begin;
	}while(*begin != '*' && *begin != '\0');

	return begin;
}

bool matchMask(const char* mask, const char* text)
{
	if(StrCmpI(mask, text) == 0)
		return true;

	const char* mask_it = mask;
	const char* text_it = text;

	while(*text_it != '\0'){
		if(*mask_it == '*'){
			++mask_it;
			if(*mask_it == '*')
				continue;
			if(*mask_it == '\0')
				return true;
			const char* nextToken = tokenEnd(mask_it);
			
			std::string token(mask_it, nextToken);
			
			if(text_it = const_cast<char*>(StrStrI(text_it, token.c_str())))
				text_it += nextToken - mask_it;
			else
				return false;

			mask_it = nextToken;
		}
		else{
			const char* nextToken = tokenEnd(mask_it);
			std::size_t len = nextToken - mask_it;
			if(strlen(text_it) < len || StrCmpNI(text_it, mask_it, len) != 0)
				return false;
			mask_it = nextToken;
			text_it += len;
		}

	}
	return true;
}

struct LookupParameter
{
	ParameterGroupReference group_;

	LookupParameter(const ParameterGroupReference& group)
	: group_(group)
	{}

	bool operator() (const char* name, float& value){
		const ParameterValueTable::Strings& strings = ParameterValueTable::instance().strings();
		ParameterValueTable::Strings::const_iterator it;
		FOR_EACH(strings, it){
			if(group_ == it->group() ? matchMask(name, it->c_str()) : stricmp(it->c_str(), name) == 0){
				value = it->value();
				return true;
			}
		}
		return false;
	}
};
}

void CFormulaEditorDlg::updateSyntaxHighlighting ()
{
	CString formula_str;
	m_ctlEdit.GetWindowText (formula_str);
	formula_.set (formula_str);

	const char* buf = formula_.c_str();
	
	HighlightRange default_highlight (m_ctlEdit, buf, RGB (0, 0, 0), 21);

	default_highlight(buf, buf + strlen (buf));

	HighlightRange var_highlight     (m_ctlEdit, buf, RGB (0, 128, 0));
	HighlightRange bad_var_highlight (m_ctlEdit, buf, RGB (255, 0, 0));
	HighlightRange op_highlight      (m_ctlEdit, buf, RGB (0, 0, 192));

	float value = 0.0f;
	FormulaString::EvalResult result = formula_.evaluate(value, x_value_, lookupFunction_, var_highlight, bad_var_highlight, op_highlight);
	CString str;
	switch (result) {
	case FormulaString::EVAL_SUCCESS:
		str.Format (TRANSLATE("Результат: %f"), value);
		break;
	case FormulaString::EVAL_UNDEFINED_NAME:
		str = TRANSLATE("Ошибка: Использован неопределнный параметр");
		break;
	case FormulaString::EVAL_SYNTAX_ERROR:
		str = TRANSLATE("Ошибка: Плохой синтаксис");
		break;
	};
	SetDlgItemText (IDC_RESULT_LABEL, str);
}

void CFormulaEditorDlg::OnFormulaEditChange()
{
	updateSyntaxHighlighting ();
}

void CFormulaEditorDlg::OnInsertButtonClicked()
{
    paramCombo_.showDropDown();
}

void CFormulaEditorDlg::OnOK()
{
	CString str;
	m_ctlEdit.GetWindowText (str);
	formula_.set (str);

	CDialog::OnOK();
}

void CFormulaEditorDlg::OnInsertVariable (UINT nID)
{
}

void CFormulaEditorDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialog::OnSizing(fwSide, pRect);
	CRect* rt = static_cast<CRect*>(pRect);

	int height = layout_.initialSize().cy;
	if (fwSide == WMSZ_BOTTOM || fwSide == WMSZ_BOTTOMLEFT || fwSide == WMSZ_BOTTOMRIGHT) {
		rt->bottom = rt->top + height; 
	}
	if (fwSide == WMSZ_TOP || fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_TOPRIGHT) {
		rt->top = rt->bottom - height; 
	}
}

void CFormulaEditorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	layout_.onSize (cx, cy);
	RedrawWindow (0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
}

void CFormulaEditorDlg::onParameterInserted(ParameterValueReference ref)
{
	std::string str = std::string(" '") + ref->c_str() + "' ";
	long start, end;
	m_ctlEdit.GetSel (start, end);
	m_ctlEdit.ReplaceSel (str.c_str(), 1);
	m_ctlEdit.SetSel (end + strlen(str.c_str()), end + strlen(str.c_str()));
	m_ctlEdit.SetFocus ();
}
