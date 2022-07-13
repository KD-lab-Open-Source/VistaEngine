#include "stdafx.h"
#include "SurMap5.h"
#include ".\DlgSelectTrigger.h"

#include "DlgWorldName.h"
#include "Dictionary.h"

IMPLEMENT_DYNAMIC(CDlgSelectTrigger, CDialog)
CDlgSelectTrigger::CDlgSelectTrigger(const char* _path2triggersFiles, const char* _title, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSelectTrigger::IDD, pParent)
{
	path2triggersFiles=_path2triggersFiles;
	title=_title;
}

CDlgSelectTrigger::~CDlgSelectTrigger()
{
}

void CDlgSelectTrigger::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LST_TRIGGER, m_listTriggers);
}


BEGIN_MESSAGE_MAP(CDlgSelectTrigger, CDialog)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_NEWTRIGGER, OnBnClickedBtnNewtrigger)
	ON_BN_CLICKED(IDC_BTN_DELETETRIGGER, OnBnClickedBtnDeletetrigger)
	ON_BN_CLICKED(IDC_BTN_COPY_TRIGGER, OnCopyTriggerButton)
END_MESSAGE_MAP()


// CDlgSelectTrigger message handlers
void CDlgSelectTrigger::fillTriggerList()
{
	m_listTriggers.DeleteAllItems();
	int nItem=0;
	WIN32_FIND_DATA FindFileData;
	XBuffer str;
	str < path2triggersFiles.c_str() < "\\*.scr";
	HANDLE hf = FindFirstFile( str, &FindFileData );
	if(hf != INVALID_HANDLE_VALUE){
		do{
			if ( !(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ) {
				CString* pstr = new CString(FindFileData.cFileName);
				m_listTriggers.InsertItem(/*LVIF_IMAGE |*/ LVIF_PARAM|LVIF_TEXT, nItem, (LPCTSTR)*pstr, 0, 0, nItem+1, reinterpret_cast<LPARAM>(pstr));
				nItem++;
			}
		} while(FindNextFile( hf, &FindFileData ));
		FindClose( hf );
	}
}

BOOL CDlgSelectTrigger::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	SetWindowText(title.c_str());

	m_listTriggers.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

	//CString str;
	m_listTriggers.InsertColumn(0, "TriggerName", LVCFMT_LEFT, 140);
	fillTriggerList();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgSelectTrigger::OnOK()
{
	if(m_listTriggers.GetSelectedCount() == 0){
		MessageBox(TRANSLATE("Триггер не выбран"), TRANSLATE("Ошибка"), MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	else{
		CString* pstr=reinterpret_cast<CString*>(m_listTriggers.GetItemData(m_listTriggers.GetSelectionMark()));
		selectTriggersFile=*pstr;

		CDialog::OnOK();
	}
}

void CDlgSelectTrigger::OnDestroy()
{
	CDialog::OnDestroy();

	for (int nItem = m_listTriggers.GetItemCount(); --nItem >= 0;)
    delete reinterpret_cast<CString*>(m_listTriggers.GetItemData(nItem));
}

void CDlgSelectTrigger::OnBnClickedBtnNewtrigger()
{
	CDlgWorldName dlgNT;
	int nRet=dlgNT.DoModal();
	if( nRet==IDOK ) {
		selectTriggersFile=(LPCTSTR)dlgNT.newWorldName;
		EndDialog(IDOK);
	}
}

void CDlgSelectTrigger::OnBnClickedBtnDeletetrigger()
{
	CString* pstr=reinterpret_cast<CString*>(m_listTriggers.GetItemData(m_listTriggers.GetSelectionMark()));
	if(pstr && !pstr->IsEmpty()){
		char str[256];
		sprintf(str, "Really want to remove trigger - %s ?", *pstr );
		int result=AfxMessageBox(str, MB_OKCANCEL);
		if(result==IDOK){
			string fileTrigger= path2triggersFiles + "\\" + (LPCTSTR)(*pstr);
			::DeleteFile(fileTrigger.c_str());
			fillTriggerList();
		}
	}
}

void CDlgSelectTrigger::OnCopyTriggerButton()
{
	CString* str = reinterpret_cast<CString*>(m_listTriggers.GetItemData(m_listTriggers.GetSelectionMark()));
	if(str && !str->IsEmpty()){
		CDlgWorldName dlg;
		if(dlg.DoModal() == IDOK){
			selectTriggersFile = static_cast<const char*>(dlg.newWorldName);
			
			int len = strlen(selectTriggersFile.c_str());
			
			if(len < 4 || stricmp(std::string(&selectTriggersFile[len - 4], &selectTriggersFile[len]).c_str(), ".scr") != 0)
				selectTriggersFile += ".scr";

			std::string source = path2triggersFiles + "\\" + static_cast<const char*>(*str);
			std::string destination = path2triggersFiles + "\\" + selectTriggersFile;
			if(!CopyFile(source.c_str(), destination.c_str(), TRUE)){
				CString message;
				message.Format("Unable to copy trigger!");
				AfxMessageBox(message, MB_OK | MB_ICONERROR);
				selectTriggersFile = "";
				return;
			}
			EndDialog(IDOK);
		}
	}	
}
