#include "stdafx.h"
#include "SurMap5.h"
#include ".\DlgExImWorld.h"
#include "FileUtils.h"
#include "Dictionary.h"
#include "..\terra\terra.h"
#include "..\Network\NetPlayer.h"

#include <WinBase.h>

const char* PACKER_EXE = "packer.exe";
// CDlgExImWorld dialog
int CDlgExImWorld::previsionWorldSelect=0;

IMPLEMENT_DYNAMIC(CDlgExImWorld, CDialog)
CDlgExImWorld::CDlgExImWorld(const char* _path2worlds, const char* _title, bool _enableCreateDir, CWnd* pParent)
: CDialog(CDlgExImWorld::IDD, pParent)
{
	path2worlds=_path2worlds;
	title=_title;
	enableCreateDir=_enableCreateDir;
}

CDlgExImWorld::~CDlgExImWorld()
{
}

void CDlgExImWorld::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LST_WORLD, m_listWorld);
}


BEGIN_MESSAGE_MAP(CDlgExImWorld, CDialog)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_IMPORT, OnButtonImport)
	ON_BN_CLICKED(IDC_BTN_EXPORT, OnButtonExport)
END_MESSAGE_MAP()


// CDlgExImWorld message handlers

void CDlgExImWorld::clearWorldList()
{
	for(int nItem = m_listWorld.GetItemCount(); --nItem >= 0;){
		CString* str = reinterpret_cast<CString*>(m_listWorld.GetItemData(nItem));
		delete str;
	}
	m_listWorld.DeleteAllItems();
}

void CDlgExImWorld::fillWorldList()
{
	clearWorldList();

	MissionDescriptions descriptions;
	descriptions.readUserWorldsFromDir(path2worlds.c_str());

	MissionDescriptions::iterator it;
	int index = 0;
	FOR_EACH(descriptions, it){
		MissionDescription& desc = *it;
		CString* str = new CString;
		*str = desc.worldName();
		m_listWorld.InsertItem(LVIF_PARAM | LVIF_TEXT, index, static_cast<const char*>(*str),
			0, 0, index + 1, reinterpret_cast<LPARAM>(str));

		XBuffer sizeText;
		sizeText <= round(desc.worldSize().x) < "x" <= round(desc.worldSize().y);
		m_listWorld.SetItemText(index, 1, desc.interfaceName());
		m_listWorld.SetItemText(index, 2, sizeText);

		++index;
	}

	int itemCount=m_listWorld.GetItemCount();
	if(previsionWorldSelect >= itemCount) previsionWorldSelect=itemCount-1;
	if(previsionWorldSelect < 0) previsionWorldSelect=0;
	if(itemCount){
		m_listWorld.SetItemState(previsionWorldSelect, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
		m_listWorld.EnsureVisible(previsionWorldSelect, TRUE);
	}
}

BOOL CDlgExImWorld::OnInitDialog()
{
	CDialog::OnInitDialog();

	if(!IsDlgButtonChecked(IDC_FULL_EXPORT) && !IsDlgButtonChecked(IDC_PARTIAL_EXPORT))
		CheckDlgButton(IDC_PARTIAL_EXPORT, 1);

	SetWindowText(title.c_str());

	m_listWorld.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

	CRect rect;
	m_listWorld.GetClientRect(&rect);
	int nameColumnWidth = rect.Width() - 100 - 80 - GetSystemMetrics(SM_CXHSCROLL);
	m_listWorld.InsertColumn(0, TRANSLATE("Файл"), LVCFMT_LEFT, 100);
	m_listWorld.InsertColumn(1, TRANSLATE("Название"), LVCFMT_LEFT, nameColumnWidth);
	m_listWorld.InsertColumn(2, TRANSLATE("Размер"), LVCFMT_RIGHT, 80);

	fillWorldList();

	return TRUE;
}


void CDlgExImWorld::OnDestroy()
{
	CDialog::OnDestroy();

	clearWorldList();
}

void CDlgExImWorld::OnOK()
{
	CDialog::OnCancel();
}

DWORD CreateProcessEx ( LPCSTR lpAppPath, LPCSTR lpCmdLine, 
BOOL bAppInCmdLine, BOOL bCompletePath,
BOOL bWaitForProcess, BOOL bMinimizeOnWait, HWND hMainWnd ) 
{
	STARTUPINFO startupInfo;
	PROCESS_INFORMATION    processInformation;    

	char szAppPath    [ _MAX_PATH  ];
	char szCmdLine    [ _MAX_PATH  ];
	char drive        [ _MAX_DRIVE ];
	char dir        [ _MAX_DIR   ];
	char name       [ _MAX_FNAME ];
	char ext        [ _MAX_EXT ];

	szAppPath[ 0 ] = '\0';
	szCmdLine[ 0 ] = '\0';

	ZeroMemory( &startupInfo, sizeof( STARTUPINFO ));

	startupInfo.cb = sizeof( STARTUPINFO );

	ZeroMemory( &processInformation, sizeof( PROCESS_INFORMATION ));

	if ( bCompletePath ) {

		GetModuleFileName( GetModuleHandle( NULL ), szAppPath, _MAX_PATH );

		_splitpath( szAppPath, drive, dir, NULL, NULL );
		_splitpath( lpAppPath, NULL, NULL, name, ext );

		_makepath ( szAppPath, drive, dir, name, strlen( ext ) ? ext : ".exe" );
	}
	else strcpy( szAppPath, lpAppPath );

	if ( bAppInCmdLine ) {

		strcpy( szCmdLine, "\"" );
		strcat( szCmdLine, szAppPath );
		strcat( szCmdLine, "\"" );
	}

	if ( lpCmdLine ) {

		// We must use quotation marks around the command line (if any)...

		if ( bAppInCmdLine ) strcat( szCmdLine, " " );
		else strcpy( szCmdLine, "" );

		strcat( szCmdLine, lpCmdLine );
		//strcat( szCmdLine, "\"" );
	}

	DWORD dwExitCode = -1;

	if ( CreateProcess(    bAppInCmdLine ? NULL: szAppPath,    // lpszImageName
		szCmdLine,                            // lpszCommandLine
		0,                                    // lpsaProcess
		0,                                    // lpsaThread
		FALSE,                                // fInheritHandles
		DETACHED_PROCESS,                    // fdwCreate
		0,                                    // lpvEnvironment
		0,                                    // lpszCurDir
		&startupInfo,                        // lpsiStartupInfo
		&processInformation                    // lppiProcInfo
		)) {

			if ( bWaitForProcess ) {

				if ( bMinimizeOnWait )

					if ( IsWindow( hMainWnd )) ShowWindow( hMainWnd, SW_MINIMIZE );
#ifdef __AFX_H__
					else AfxGetMainWnd()->ShowWindow( SW_MINIMIZE );
#endif

					while(WaitForSingleObject( processInformation.hProcess, 10 ) == WAIT_TIMEOUT){
						MSG msg;
						while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
							if(!::IsDialogMessage(hMainWnd, &msg)){
								::TranslateMessage(&msg);
								::DispatchMessage(&msg);
							}
						}
					}

					if ( bMinimizeOnWait )

						if ( IsWindow( hMainWnd )) ShowWindow( hMainWnd, SW_RESTORE );
#ifdef __AFX_H__
						else AfxGetMainWnd()->ShowWindow( SW_RESTORE );
#endif

						GetExitCodeProcess( processInformation.hProcess, &dwExitCode );
			}
			else {

				CloseHandle( processInformation.hThread );
				CloseHandle( processInformation.hProcess );

				dwExitCode = 0;
			}
		}

		return dwExitCode;
}

void CDlgExImWorld::OnButtonExport()
{
	CString* pstr=reinterpret_cast<CString*>(m_listWorld.GetItemData(m_listWorld.GetSelectionMark()));
		if(!pstr->IsEmpty()){
			CFileDialog fileDlg (FALSE, "*.mpw", 0,
				OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT,
				"Maelstrom Packed World (*.mpw)|*.mpw||", 0);
			fileDlg.m_ofn.lpstrTitle = TRANSLATE("Сохранить файл...");
			std::string worldName = static_cast<const char*>(*pstr);
			std::string defaultFilename = worldName;
			fileDlg.m_ofn.lpstrFile = const_cast<char*>(defaultFilename.c_str());

			if(fileDlg.DoModal() == IDOK){
				CWaitCursor waitCursor;
				std::string path = fileDlg.GetPathName();
				if(isFileExists(path.c_str()))
					DeleteFile(path.c_str());
				EnableWindow(false);
				std::string commandLine = "";
				if(IsDlgButtonChecked(IDC_FULL_EXPORT))
					commandLine = " e \"" + path + "\" \"" + worldName + "\"";
				else
					commandLine = " ex \"" + path + "\" \"" + worldName + "\"";
				CreateProcessEx(PACKER_EXE, commandLine.c_str(), true, true, true, false , this->m_hWnd);
				EnableWindow(true);
				/*int result = _spawnl (_P_WAIT , PACKER_EXE, PACKER_EXE, "ex", path.c_str(), worldName.c_str(), NULL);
				if (result < 0)
				{
					CString message (TRANSLATE("Не могу запустить экспорт мира!"));
					MessageBox (message, 0, MB_OK | MB_ICONERROR);
				}*/
			}
		}
}

void CDlgExImWorld::OnButtonImport()
{
	CFileDialog fileDlg (TRUE, "*.mpw", 0,
		OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST,
		"Maelstrom Packed World (*.mpw)|*.mpw||", 0);
	fileDlg.m_ofn.lpstrTitle = TRANSLATE("Открыть файл...");

	if(fileDlg.DoModal() == IDOK){
		CWaitCursor waitCursor;
		std::string path = fileDlg.GetPathName();	

		EnableWindow(false);
		std::string commandLine = " i \"" + path + "\"";
		CreateProcessEx(PACKER_EXE, commandLine.c_str(), true, true, true, false , this->m_hWnd);
		EnableWindow(true);
		/*int result = _spawnl(_P_WAIT , PACKER_EXE, PACKER_EXE, "i", path.c_str(), NULL);
		if (result < 0)
		{
			CString message (TRANSLATE("Не могу запустить импорт мира!"));
			MessageBox (message, 0, MB_OK | MB_ICONERROR);
		}*/
		clearWorldList();
		fillWorldList();
	}

}
