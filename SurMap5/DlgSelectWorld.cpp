#include "stdafx.h"
#include "SurMap5.h"
#include ".\DlgSelectWorld.h"
#include "DlgWorldName.h"
#include "FileUtils.h"
#include "Dictionary.h"
#include "..\terra\terra.h"
#include "..\Network\NetPlayer.h"

// CDlgSelectWorld dialog
int CDlgSelectWorld::previsionWorldSelect=0;

IMPLEMENT_DYNAMIC(CDlgSelectWorld, CDialog)
CDlgSelectWorld::CDlgSelectWorld(const char* _path2worlds, const char* _title, bool _enableCreateDir, CWnd* pParent)
	: CDialog(CDlgSelectWorld::IDD, pParent)
{
	path2worlds=_path2worlds;
	title=_title;
	enableCreateDir=_enableCreateDir;
}

CDlgSelectWorld::~CDlgSelectWorld()
{
}

void CDlgSelectWorld::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LST_WORLD, m_listWorld);
}


BEGIN_MESSAGE_MAP(CDlgSelectWorld, CDialog)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_NEWWORLD, OnButtonNew)
	ON_BN_CLICKED(IDC_BTN_DELETEWORLD, OnButtonDelete)
	ON_BN_CLICKED(IDC_BTN_RENAME, OnButtonRename)
	ON_NOTIFY(NM_DBLCLK, IDC_LST_WORLD, OnListDoubleClick)
END_MESSAGE_MAP()


// CDlgSelectWorld message handlers

void CDlgSelectWorld::clearWorldList()
{
	for(int nItem = m_listWorld.GetItemCount(); --nItem >= 0;){
		CString* str = reinterpret_cast<CString*>(m_listWorld.GetItemData(nItem));
		delete str;
	}
	m_listWorld.DeleteAllItems();
}

void CDlgSelectWorld::fillWorldList()
{
	clearWorldList();

	MissionDescriptions descriptions;
//#ifdef _VISTA
#ifndef _VISTA_ENGINE_EXTERNAL_
	descriptions.readFromDir(path2worlds.c_str(), GAME_TYPE_SCENARIO);
#else
	descriptions.readUserWorldsFromDir(path2worlds.c_str());
#endif

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

BOOL CDlgSelectWorld::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(title.c_str());
	if(!enableCreateDir)
		GetDlgItem(IDC_BTN_NEWWORLD)->ShowWindow(SW_HIDE);

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


void CDlgSelectWorld::OnDestroy()
{
	CDialog::OnDestroy();

	clearWorldList();
}

void CDlgSelectWorld::OnListDoubleClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	OnOK();
}


void CDlgSelectWorld::OnOK()
{
	int selidx=m_listWorld.GetSelectionMark();
	if(selidx!=-1){
		previsionWorldSelect=selidx;
		CString* pstr=reinterpret_cast<CString*>(m_listWorld.GetItemData(m_listWorld.GetSelectionMark()));
		selectWorldName=*pstr;
		CDialog::OnOK();
	}
	else
		CDialog::OnCancel();
}

void CDlgSelectWorld::OnButtonNew()
{
	CDlgWorldName dlgWN;
	int nRet=dlgWN.DoModal();
	if( nRet==IDOK ) {
		XBuffer str;
		str < path2worlds.c_str() < "\\" < (LPCTSTR)dlgWN.newWorldName;
		if(::CreateDirectory(str, NULL)){
			selectWorldName=(LPCTSTR)dlgWN.newWorldName;
			EndDialog(IDOK);
		}
		else {
			XBuffer strerr;
			strerr < TRANSLATE("Ошибка создания мира: ") < (LPCTSTR)dlgWN.newWorldName;
			AfxMessageBox(strerr);
		}
	}
}

void CDlgSelectWorld::OnButtonDelete()
{
	CString* pstr=reinterpret_cast<CString*>(m_listWorld.GetItemData(m_listWorld.GetSelectionMark()));
	if(!pstr->IsEmpty()){
		char str[256];
		sprintf(str, TRANSLATE("Вы уверены что хотите удалить мир: %s?"), *pstr );
		int result=AfxMessageBox(str, MB_OKCANCEL);
		if(result==IDOK){
			vMap.deleteWorld(*pstr);
			string fileMission= path2worlds + "\\" + (LPCTSTR)(*pstr) + ".spg";
			::DeleteFile(fileMission.c_str());
			fileMission= path2worlds + "\\" + (LPCTSTR)(*pstr) + ".spg.bin";
			::DeleteFile(fileMission.c_str());
			fillWorldList();
		}
	}
}

bool moveFileVerbose(const char* oldPath, const char* newPath)
{
	if(!::MoveFile(oldPath, newPath)){
		CString str;
		str.Format(TRANSLATE("Не могу переименновать файл из %s в %s!"), oldPath, newPath);
		AfxMessageBox(str, MB_OK | MB_ICONERROR);
		return false;
	}
	else
		return true;
}

bool renameWorldVerbose(const char* worldsDir, const char* oldWorldName, const char* newWorldName)
{
	std::string oldPath;
	std::string newPath;

	typedef std::vector<std::pair<std::string, std::string> > Filenames;
	Filenames filenames;

	std::string source, destination;
	DirIterator it((std::string(worldsDir) + "\\" + oldWorldName + ".*").c_str());
	DirIterator end;
	while(it != end){
		source = std::string(worldsDir) + "\\" + *it;
		destination = std::string(worldsDir) + "\\" + newWorldName + (*it + strlen(oldWorldName));
		if(::isFileExists(destination.c_str())){
			AfxMessageBox(TRANSLATE("Такой мир уже существует!"), MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		filenames.push_back(std::make_pair<std::string, std::string>(source, destination));
		++it;
	}

	Filenames::iterator fit;
	FOR_EACH(filenames, fit){
		if(!moveFileVerbose(fit->first.c_str(), fit->second.c_str()))
			return false;
	}
	return true;
}

void CDlgSelectWorld::OnButtonRename()
{
	CString* pstr=reinterpret_cast<CString*>(m_listWorld.GetItemData(m_listWorld.GetSelectionMark()));
	if(!pstr->IsEmpty()){
		XBuffer buf;

		std::string oldWorldName = static_cast<const char*>(*pstr);
		CDlgWorldName dlg(this);
		dlg.newWorldName = oldWorldName.c_str();

		while(true){
			if(dlg.DoModal() == IDOK){
				if(renameWorldVerbose(path2worlds.c_str(), oldWorldName.c_str(), dlg.newWorldName))
				    fillWorldList();
					break;
			}
			else
				break;
		}
	}
}
