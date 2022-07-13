// DlgTexturesStatistics.cpp : implementation file
//

#include "stdafx.h"
#include "..\Render\d3d\StdAfxRD.h"
#include "DlgTexturesStatistics.h"
#include ".\dlgtexturesstatistics.h"


// CDlgTexturesStatistics dialog

IMPLEMENT_DYNAMIC(CDlgTexturesStatistics, CDialog)
CDlgTexturesStatistics::CDlgTexturesStatistics(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgTexturesStatistics::IDD, pParent)
	, m_NumTextures(_T(""))
	, m_TexturesSize(_T(""))
{
}

CDlgTexturesStatistics::~CDlgTexturesStatistics()
{
}

void CDlgTexturesStatistics::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TEXTURES_LIST, m_TexturesList);
	DDX_Text(pDX, IDC_NUM_TEXTURES, m_NumTextures);
	DDX_Text(pDX, IDC_SIZE_TEXTURES, m_TexturesSize);
}


BEGIN_MESSAGE_MAP(CDlgTexturesStatistics, CDialog)
END_MESSAGE_MAP()


BOOL CDlgTexturesStatistics::OnInitDialog()
{
	CDialog::OnInitDialog();

	DWORD texturesSize = 0;

	m_TexturesList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_TexturesList.InsertColumn(0,"Имя",LVCFMT_LEFT,350);
	m_TexturesList.InsertColumn(1,"Размер",LVCFMT_RIGHT,100);

	cTexLibrary* texLab  = GetTexLibrary();
	for (int i=0; i<texLab->GetNumberTexture(); i++)
	{
		cTexture* cur;
		cur = texLab->GetTexture(i);
		m_TexturesList.InsertItem(i,cur->GetName());
		int size = cur->CalcTextureSize();
		CString str;
		str.Format("%d",size);
		for (int j=str.GetLength(); j>0; j-=3)
			str.Insert(j,' ');
		m_TexturesList.SetItem(i,1,LVIF_TEXT,str,0,0,0,0);
		m_TexturesList.SetItemData(i,i);
		texturesSize += size;
	}
	m_NumTextures.Format("%d",texLab->GetNumberTexture());
	m_TexturesSize.Format("%d",texturesSize);
	for (int j=m_TexturesSize.GetLength(); j>0; j-=3)
		m_TexturesSize.Insert(j,' ');

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
