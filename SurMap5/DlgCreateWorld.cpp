#include "stdafx.h"
#include "SurMap5.h"
#include "DlgCreateWorld.h"
#include "EditArchive.h"
#include "Dictionary.h"

// CDlgCreateWorld dialog

IMPLEMENT_DYNAMIC(CDlgCreateWorld, CDialog)
CDlgCreateWorld::CDlgCreateWorld(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCreateWorld::IDD, pParent)
	, m_strWorldName(_T(""))
{
}

CDlgCreateWorld::~CDlgCreateWorld()
{
}

void CDlgCreateWorld::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ATTRIB_EDITOR, m_ctlAttribEditor);
	DDX_Text(pDX, IDC_NAME_EDIT, m_strWorldName);
}


BEGIN_MESSAGE_MAP(CDlgCreateWorld, CDialog)
END_MESSAGE_MAP()

BOOL CDlgCreateWorld::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ctlAttribEditor.setStyle(CAttribEditorCtrl::AUTO_SIZE);
	m_ctlAttribEditor.initControl ();
	EditOArchive oarchive;
#ifdef _VISTA_ENGINE_EXTERNAL_
	oarchive.serialize(m_CreationParam, "m_CreationParam", "Свойства новой карты");
#else
	oarchive.serialize(m_CreationParam, "m_CreationParam", "Map Properties");
#endif
	m_ctlAttribEditor.setRootNode(oarchive.rootNode ());
	m_strWorldName = "New World";
	UpdateData (FALSE);

	((CEdit*)GetDlgItem (IDC_NAME_EDIT))->SetSel (0, -1);

#ifdef _VISTA_ENGINE_EXTERNAL_
	SetWindowText(TRANSLATE("Создать новый мир"));
	GetDlgItem(IDC_DEFAULT_BUTTON)->SetWindowText(TRANSLATE("По умолчанию"));
	GetDlgItem(IDC_WORLD_NAME_LABEL)->SetWindowText(TRANSLATE("Название"));
	GetDlgItem(IDOK)->SetWindowText(TRANSLATE("ОК"));
	GetDlgItem(IDCANCEL)->SetWindowText(TRANSLATE("Отмена"));
#endif

	GetDlgItem (IDC_NAME_EDIT)->SetFocus ();
	return FALSE;
}

void CDlgCreateWorld::OnOK()
{
	UpdateData (TRUE);
	EditIArchive iarchive (m_ctlAttribEditor.tree());
#ifdef _VISTA_ENGINE_EXTERNAL_
	iarchive.serialize(m_CreationParam, "m_CreationParam", "Свойства новой карты");
#else
	iarchive.serialize(m_CreationParam, "m_CreationParam", "Map Properties");
#endif

	CDialog::OnOK();
}
