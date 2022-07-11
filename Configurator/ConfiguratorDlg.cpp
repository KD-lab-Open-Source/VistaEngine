#include "stdafx.h"
#include "Configurator.h"
#include "ConfiguratorDlg.h"
#include "mfc\LayoutMFC.h"
#include "AttribEditor\AttribEditorCtrl.h"
#include "EditorGameOptions.h"
#include "Game\GameOptions.h"
#include "Serialization\Dictionary.h"
#include "kdw\PropertyTree.h"
#include "kdw\PropertyTreeModel.h"

#include "ConfiguratorAttribEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();
	enum { IDD = IDD_ABOUTBOX };
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// ---------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4355)

CConfiguratorDlg::CConfiguratorDlg(CWnd* pParent)
: CDialog(CConfiguratorDlg::IDD, pParent)
, layout_(new LayoutWindow(this))
, attribEditor_(new CConfiguratorAttribEditor)
{
	icon_ = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CConfiguratorDlg::~CConfiguratorDlg()
{
	attribEditor_ = 0;
}

void CConfiguratorDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_ATTRIB_EDITOR, *attribEditor_);
	CDialog::DoDataExchange(pDX);
}

IMPLEMENT_DYNAMIC(CConfiguratorDlg, CDialog)
BEGIN_MESSAGE_MAP(CConfiguratorDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_CLOSE_BUTTON, OnCloseButtonClicked)
	ON_BN_CLICKED(IDC_SAVE_BUTTON, OnSaveButtonClicked)
END_MESSAGE_MAP()


BOOL CConfiguratorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

    attribEditor_->initControl();
	attribEditor_->setStyle(CAttribEditorCtrl::HIDE_ROOT_NODE | CAttribEditorCtrl::EXPAND_ALL | CAttribEditorCtrl::DISABLE_MENU);

	static EditorGameOptions options;
	attribEditor_->attachSerializer(Serializer(options));
	kdw::PropertyTree* tree = &attribEditor_->tree();
	tree->model()->root()->expandAll(tree);
	tree->update();

	SetIcon(icon_, TRUE);
	SetIcon(icon_, FALSE);

    onConfigChanged();
	
	return TRUE;
}

void CConfiguratorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if((nID & 0xFFF0) == IDM_ABOUTBOX){
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else{
		CDialog::OnSysCommand(nID, lParam);
	}
}

void CConfiguratorDlg::OnPaint() 
{
	if(IsIconic()){
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, icon_);
	}
	else{
		CDialog::OnPaint();
	}
}

HCURSOR CConfiguratorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(icon_);
}

void CConfiguratorDlg::OnCloseButtonClicked()
{
	EndModalLoop(IDOK);
}

bool saveRegistryString(const char* folderName, const char* keyName, const char* value)
{
	HKEY key;
	DWORD deposition;
	LONG result;
	
	result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, folderName, 0, "", 0, KEY_ALL_ACCESS, NULL, &key, &deposition);
	if(result != ERROR_SUCCESS){
		result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, folderName, 0, KEY_ALL_ACCESS, &key);
	}
	if(result == ERROR_SUCCESS){
		result = RegSetValueEx(key, keyName, 0, REG_SZ, (LPBYTE)(value), strlen(value));
		RegCloseKey(key);
		return true;
	}
	return false;
}

void CConfiguratorDlg::OnSaveButtonClicked()
{
    GameOptions::instance().saveLibrary();

	const char* language = GameOptions::instance().getLanguage();
	if(!saveRegistryString("Software\\Codemasters\\Maelstrom", "Language", language)){
		// unable to write to registry
	}

	EndModalLoop(IDOK);
}

void CConfiguratorDlg::updateControlsLanguage()
{
	//attribEditor_->treeControl().SetColumnText(0, TRANSLATE("Параметр"));
	//attribEditor_->treeControl().SetColumnText(1, TRANSLATE("Значение"));
	SetDlgItemText(IDC_SAVE_BUTTON, TRANSLATE("Сохранить"));
	SetDlgItemText(IDC_CLOSE_BUTTON, TRANSLATE("Закрыть"));
}

void CConfiguratorDlg::onConfigChanged()
{
	GameOptions::instance().setTranslate();
	TranslationManager::instance().setLanguage(GameOptions::instance().getLanguage());
    updateControlsLanguage();
}

#pragma warning(pop)
