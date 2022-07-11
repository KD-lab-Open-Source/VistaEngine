#include "StdAfx.h"

#include "xmath.h" //for configuration tool
#include "Resource.h"
#include "AttribEditorDlg.h"

// CAttribEditorDlg dialog

IMPLEMENT_DYNAMIC(CAttribEditorDlg, CDialog)
CAttribEditorDlg::CAttribEditorDlg(CWnd* pParent /*=NULL*/)
: CDialog(CAttribEditorDlg::IDD, pParent)
, treeControlSetup_ (0, 0, 640, 480, "")
, okLabel_(0)
, cancelLabel_(0)
{
}

CAttribEditorDlg::~CAttribEditorDlg()
{
}

void CAttribEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ATTRIB_EDITOR, attribEditor_);
}


BEGIN_MESSAGE_MAP(CAttribEditorDlg, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CAttribEditorDlg message handlers

BOOL CAttribEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (treeControlSetup_.showRootNode_) {
		attribEditor_.setStyle(CAttribEditorCtrl::AUTO_SIZE);
	} else {
		attribEditor_.setStyle(CAttribEditorCtrl::AUTO_SIZE |
								   CAttribEditorCtrl::HIDE_ROOT_NODE);
	}

    attribEditor_.initControl();
    if(rootNode_)
        attribEditor_.setRootNode(rootNode_);
    else if(serializeable_)
        attribEditor_.attachSerializeable(serializeable_);
    else
        xassert(0);

    attribEditor_.setStatusLabel(static_cast<CStatic*>(GetDlgItem(IDC_STATUS_LABEL)));

	CRect rt;
	GetWindowRect (&rt);

    if(!layout_.isInitialized()){
		layout_.init(this);
        layout_.add(1, 1, 1, 1, IDC_ATTRIB_EDITOR);
        layout_.add(1, 0, 1, 1, IDC_STATUS_LABEL);
        layout_.add(1, 0, 1, 1, IDC_HORIZONTAL_LINE);
        layout_.add(0, 0, 1, 1, IDOK);
        layout_.add(0, 0, 1, 1, IDCANCEL);
    }

	CString strTitle = "Editing \"";
	strTitle += treeControlSetup_.getConfigName ();
	strTitle += "\"";
	SetWindowText (strTitle);

	if(okLabel_)
		GetDlgItem(IDOK)->SetWindowText(okLabel_);
	if(cancelLabel_)
		GetDlgItem(IDCANCEL)->SetWindowText(cancelLabel_);

	MoveWindow(&treeControlSetup_.window);
	loadEditorState();
	return TRUE;
}

void CAttribEditorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

    layout_.onSize (cx, cy);
    Invalidate ();
}

bool CAttribEditorDlg::edit(Serializeable serializeable, HWND hwndParent, const TreeControlSetup& treeControlSetup, const char* okLabel, const char* cancelLabel)
{
	okLabel_ = okLabel;
	cancelLabel_ = cancelLabel;
    treeControlSetup_ = treeControlSetup;
    serializeable_ = serializeable;

    if(DoModal() == IDOK)
        return true;
    else
        return false;
}

const TreeNode* CAttribEditorDlg::edit(const TreeNode* oldRoot, HWND wndParent, const TreeControlSetup& treeControlSetup)
{
    treeControlSetup_ = treeControlSetup;
	rootNode_ = new TreeNode ("");
	*rootNode_ = *oldRoot;

	if(DoModal() == IDOK)
        return attribEditor_.tree();
    else
        return 0;
}

void CAttribEditorDlg::loadEditorState()
{
	const char* configName = treeControlSetup_.getConfigName ();
	XStream file(0);
	if(file.open(configName, XS_IN)){
		loadWindowGeometry(file);
		attribEditor_.loadExpandState(file);
	}
}

void CAttribEditorDlg::saveEditorState()
{
	const char* configName = treeControlSetup_.getConfigName();
	XStream file(configName, XS_OUT);

	saveWindowGeometry(file);
	attribEditor_.saveExpandState(file);
}

void CAttribEditorDlg::loadWindowGeometry(XStream& file)
{
	CRect windowRect (0, 0, 0, 0);
	if(file.read(reinterpret_cast<char*>(&windowRect), sizeof(windowRect)) == sizeof(windowRect) && !windowRect.IsRectEmpty()){
		windowRect.left   = clamp(windowRect.left, 0, GetSystemMetrics(SM_CXSCREEN));
		windowRect.right  = clamp(windowRect.right, 0, GetSystemMetrics(SM_CXSCREEN));
		windowRect.top    = clamp(windowRect.top, 0, GetSystemMetrics(SM_CYSCREEN));
		windowRect.bottom = clamp(windowRect.bottom, 0, GetSystemMetrics(SM_CYSCREEN));
		MoveWindow(&windowRect, TRUE);
	}
}

void CAttribEditorDlg::saveWindowGeometry(XStream& file)
{
	CRect windowRect;
	GetWindowRect(&windowRect);
	file.write(reinterpret_cast<char*>(&windowRect), sizeof(windowRect));
}

void CAttribEditorDlg::OnOK()
{
	saveEditorState();
	__super::OnOK();
}
