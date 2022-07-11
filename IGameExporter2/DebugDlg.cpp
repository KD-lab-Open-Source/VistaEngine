// DebugDlg.cpp : implementation file
//

#include "stdafx.h"
#include "IGameExporter.h"
#include "DebugDlg.h"
#include "resource.h"
#include ".\debugdlg.h"

bool dbg_show_position=false;
bool dbg_show_rotation=false;
bool dbg_show_scale=false;
bool dbg_show_info_polygon=false;
bool dbg_show_info_delete_node=false;

IMPLEMENT_DYNAMIC(CDebugDlg, CDialog)
CDebugDlg::CDebugDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDebugDlg::IDD, pParent)
	, m_bRotate(FALSE)
	, m_bPosition(FALSE)
	, m_bScale(FALSE)
	, m_bPolygon(FALSE)
	, m_bDelNode(FALSE)
{
}

CDebugDlg::~CDebugDlg()
{
}

void CDebugDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_LOG_ROTATION, m_bRotate);
	DDX_Check(pDX, IDC_LOG_POSITION, m_bPosition);
	DDX_Check(pDX, IDC_LOG_SCALE, m_bScale);
	DDX_Check(pDX, IDC_LOG_INFO_POLYGON, m_bPolygon);
	DDX_Check(pDX, IDC_LOG_INFO_DELETE_NODE, m_bDelNode);
}


BEGIN_MESSAGE_MAP(CDebugDlg, CDialog)
END_MESSAGE_MAP()


// CDebugDlg message handlers

BOOL CDebugDlg::OnInitDialog()
{
	CDialog::OnInitDialog();


	m_bRotate	= dbg_show_rotation;
	m_bPosition = dbg_show_position;
	m_bScale	= dbg_show_scale;
	m_bPolygon	= dbg_show_info_polygon;
	m_bDelNode	= dbg_show_info_delete_node;

	UpdateData(FALSE);
	return TRUE;  
}

void CDebugDlg::OnOK()
{
	UpdateData();
	dbg_show_rotation		  = m_bRotate;
	dbg_show_position		  = m_bPosition;
	dbg_show_scale			  = m_bScale;

	dbg_show_info_polygon	  = m_bPolygon;
	dbg_show_info_delete_node = m_bDelNode;

	CDialog::OnOK();
}
