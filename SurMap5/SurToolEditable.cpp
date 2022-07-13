#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolEditable.h"

#include "..\AttribEditor\AttribEditorCtrl.h"
#include "SafeCast.h"

class CSurToolAttribEditor : public CAttribEditorCtrl {
	void onElementChanged(CTreeListItem* item) {
		__super::onElementChanged(item);
		CSurToolEditable* tool = safe_cast<CSurToolEditable*>(GetParent());
		tool->onPropertyChanged();
	}
};


IMPLEMENT_DYNAMIC(CSurToolEditable, CSurToolBase)
CSurToolEditable::CSurToolEditable(CWnd* parent)
: CSurToolBase(getIDD(), parent)
, attribEditor_(0)
{
}

CSurToolEditable::~CSurToolEditable()
{
	delete attribEditor_;
	attribEditor_ = 0;
}

void CSurToolEditable::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolEditable, CSurToolBase)
    ON_WM_SIZE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()



BOOL CSurToolEditable::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	CRect attribEditorRect;
	GetDlgItem(IDC_ATTRIB_EDITOR)->GetWindowRect(&attribEditorRect);
	GetParent()->ScreenToClient(&attribEditorRect);
	
	attribEditor_ = new CSurToolAttribEditor();
	attribEditor_->setStyle(CAttribEditorCtrl::AUTO_SIZE |
							CAttribEditorCtrl::COMPACT | 
							CAttribEditorCtrl::HIDE_ROOT_NODE);
	attribEditor_->Create(WS_VISIBLE | WS_CHILD, attribEditorRect, this, 0);

	// layout
	layout_.init(this);
	layout_.add(1, 0, 1, 0, IDC_SOURCE_TYPE_LABEL);
	layout_.add(1, 1, 1, 1, attribEditor_);

	return FALSE;
}

void CSurToolEditable::OnSize(UINT nType, int cx, int cy)
{
	CSurToolBase::OnSize(nType, cx, cy);

	layout_.onSize(cx, cy);
}

void CSurToolEditable::OnDestroy()
{
	CSurToolBase::OnDestroy();

	layout_.reset();
	attribEditor_->DestroyWindow();
}

void CSurToolEditable::setLabels(const char* createLabel, const char* typeLabel)
{
	if(createLabel)
		SetDlgItemText(IDC_CREATE_LABEL, createLabel);
	if(typeLabel)
		SetDlgItemText(IDC_TYPE_LABEL, typeLabel);
}

CAttribEditorCtrl& CSurToolEditable::attribEditor()
{
	xassert(attribEditor_ != 0);
	return *attribEditor_;
}
