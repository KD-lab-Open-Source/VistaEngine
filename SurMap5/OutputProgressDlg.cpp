#include "StdAfx.h"
#include "Dictionary.h"
#include ".\OutputProgressDlg.h"

IMPLEMENT_DYNAMIC(COutputProgressDlg, CDialog)
COutputProgressDlg::COutputProgressDlg(const char* title, CWnd* parent)
: CDialog(COutputProgressDlg::IDD, parent)
, title_(title)
{
}

COutputProgressDlg::~COutputProgressDlg()
{
}

BOOL COutputProgressDlg::Create(CWnd* parent)
{
	return __super::Create(IDD, parent);
}

void COutputProgressDlg::progress(int totalPercent, int filePercent, const char* fileName)
{
	totalProgress_.SetPos(min(max(totalPercent, 0), 99));
	fileProgress_.SetPos(min(max(filePercent, 0), 99));
	SetDlgItemText(IDC_FILENAME_LABEL, fileName);
	pumpMessages();
}

void COutputProgressDlg::pumpMessages()
{
	xassert(::IsWindow(GetSafeHwnd()));

    MSG msg;
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
	  if(!IsDialogMessage(&msg)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
}


void COutputProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TOTAL_PROGRESS, totalProgress_);
	DDX_Control(pDX, IDC_FILE_PROGRESS, fileProgress_);
}


BEGIN_MESSAGE_MAP(COutputProgressDlg, CDialog)
END_MESSAGE_MAP()

BOOL COutputProgressDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	totalProgress_.SetRange(0, 99);
	fileProgress_.SetRange(0, 99);
	SetDlgItemText(IDC_FILENAME_LABEL, "");
	SetWindowText(title_.c_str());
	return TRUE;
}
