#include "stdafx.h"
#include "UIEditor.h"
#include "SnapOptionsDlg.h"
#include "Options.h"
#include ".\snapoptionsdlg.h"

IMPLEMENT_DYNAMIC(CSnapOptionsDlg, CDialog)
CSnapOptionsDlg::CSnapOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSnapOptionsDlg::IDD, pParent)
	, m_bShowGrid(FALSE)
	, m_bSnapToGrid(FALSE)
	, m_fGridXSize(0)
	, m_fGridYSize(0)
	, m_iLargeGridXSize(0)
	, m_iLargeGridYSize(0)
{
}

CSnapOptionsDlg::~CSnapOptionsDlg()
{
}

void CSnapOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SHOW_GRID_CHECK, m_bShowGrid);
	DDX_Check(pDX, IDC_SNAP_TO_GRID_CHECK, m_bSnapToGrid);
	
	DDX_Text(pDX, IDC_GRID_XSIZE_EDIT, m_fGridXSize);
	DDX_Text(pDX, IDC_GRID_YSIZE_EDIT, m_fGridYSize);
	DDX_Text(pDX, IDC_LARGE_GRID_XSIZE_EDIT, m_iLargeGridXSize);
	DDX_Text(pDX, IDC_LARGE_GRID_YSIZE_EDIT, m_iLargeGridYSize);
}


BEGIN_MESSAGE_MAP(CSnapOptionsDlg, CDialog)
END_MESSAGE_MAP()

BOOL CSnapOptionsDlg::OnInitDialog()

{
	CDialog::OnInitDialog();

	m_fGridXSize = Options::instance().gridSize().x;
	m_fGridYSize = Options::instance().gridSize().y;
	m_iLargeGridXSize = Options::instance().largeGridSize().x;
	m_iLargeGridYSize = Options::instance().largeGridSize().y;
	m_bShowGrid = Options::instance().showGrid ();
	m_bSnapToGrid = Options::instance().snapToGrid ();

	UpdateData (FALSE);
	return TRUE;
}

void CSnapOptionsDlg::OnOK()
{
	UpdateData (TRUE);
	Options::instance().setGridSize(Vect2f(clamp(m_fGridXSize, 0.00001, 1.0f), clamp(m_fGridYSize, 0.00001, 1.0f)));
	Options::instance().setLargeGridSize (Vect2i(m_iLargeGridXSize, m_iLargeGridYSize));
	Options::instance().setShowGrid(bool(m_bShowGrid));
	Options::instance().setSnapToGrid(bool(m_bSnapToGrid));
	CDialog::OnOK();
}
