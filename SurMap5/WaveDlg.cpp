// WaveDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WaveDlg.h"
#include "EnterNameDlg.h"
#include "Environment\Environment.h"
#include "SurToolAux.h"
#include "wavedlg.h"
// CWaveDlg dialog

IMPLEMENT_DYNAMIC(CWaveDlg, CDialog)
CWaveDlg::CWaveDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWaveDlg::IDD, pParent)
	, m_Distance(0)
	, m_Speed(0)
	, m_GenerationTime(0)
	, m_Invert(FALSE)
	, m_SizeMin(0)
	, m_SizeMax(0)
{
	currentWave_ = NULL;
}

CWaveDlg::~CWaveDlg()
{
}

void CWaveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_WAVE_LINES, m_ListWaveLines);
	DDX_Text(pDX, IDC_DISTANCE, m_Distance);
	DDX_Text(pDX, IDC_SPEED, m_Speed);
	DDX_Text(pDX, IDC_GENERATION_TIME, m_GenerationTime);
	DDX_Check(pDX, IDC_INVERT, m_Invert);
	DDX_Text(pDX, IDC_SCALE, m_SizeMin);
	DDX_Text(pDX, IDC_SCALE2, m_SizeMax);
}


BEGIN_MESSAGE_MAP(CWaveDlg, CDialog)
	ON_BN_CLICKED(IDC_CREATE_WAVE, OnBnClickedCreateWave)
	ON_LBN_SELCHANGE(IDC_LIST_WAVE_LINES, OnLbnSelchangeListWaveLines)
	ON_BN_CLICKED(IDC_ADD_POINT, OnBnClickedAddPoint)
	ON_BN_CLICKED(IDC_REMOVE_POINT, OnBnClickedRemovePoint)
	ON_BN_CLICKED(IDC_REMOVE_WAVE, OnBnClickedRemoveWave)
	ON_BN_CLICKED(IDC_SELECT_TEXTURE, OnBnClickedSelectTexture)
	ON_BN_CLICKED(IDC_APPLY, OnBnClickedApply)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()


void CWaveDlg::OnBnClickedCreateWave()
{
	CEnterNameDlg dlg;
	if (dlg.DoModal() == IDOK)
	{
		for (int i=0; i<environment->fixedWaves()->GetCount(); i++)
		{
			if (environment->fixedWaves()->GetWave(i)->name() == dlg.buf)
			{
				MessageBox("Такое имя уже существует!!!","Error!!!",MB_ICONERROR|MB_OK);
				return;
			}
		}
		currentWave_ = environment->fixedWaves()->AddWaves();
		currentWave_->name() = dlg.buf;
		mouseMode_ = NEW_POINTS;
	}
}
void CWaveDlg::OnMapMouseMove(const Vect3f& coord)
{
	if(vMap.isWorldLoaded () && currentWave_) 
	{
		switch (mouseMode_) 
		{
		case EDIT_POINTS:
			{
				currentWave_->SetPoint(coord);
				currentWave_->CreateSegments();
			}
			break;
		}
	}
}

void CWaveDlg::OnMapLBClick(const Vect3f& coord)
{
	if(vMap.isWorldLoaded () && currentWave_) 
	{
		switch (mouseMode_) 
		{
		case NEW_POINTS:
			{
				currentWave_->AddPoint(coord);
				currentWave_->SelectPoint(0);
				break;
			}
		case EDIT_POINTS:
			{
				currentWave_->SelectPoint(coord);
				break;
			}
		case CREATE_POINTS:
			{
				currentWave_->AddPoint(coord);
				mouseMode_ = EDIT_POINTS;
				break;
			}
		}
	}
}

void CWaveDlg::OnMapRBClick(const Vect3f& coord)
{
	mouseMode_ = EDIT_POINTS;
	RefreshList();
}
void CWaveDlg::RefreshList()
{
	m_ListWaveLines.ResetContent();
	for (int i=0; i<environment->fixedWaves()->GetCount(); i++)
	{
		m_ListWaveLines.AddString(environment->fixedWaves()->GetWave(i)->name().c_str());
	}
}
void CWaveDlg::ShowControls(bool show)
{
	GetDlgItem(IDC_DISTANCE)->EnableWindow(show);
	GetDlgItem(IDC_SPEED)->EnableWindow(show);
	GetDlgItem(IDC_SCALE)->EnableWindow(show);
	GetDlgItem(IDC_SCALE2)->EnableWindow(show);
	GetDlgItem(IDC_GENERATION_TIME)->EnableWindow(show);
	GetDlgItem(IDC_INVERT)->EnableWindow(show);
	GetDlgItem(IDC_TEXTURE_NAME)->EnableWindow(show);
	GetDlgItem(IDC_SELECT_TEXTURE)->EnableWindow(show);
	GetDlgItem(IDC_APPLY)->EnableWindow(show);
}

void CWaveDlg::OnLbnSelchangeListWaveLines()
{
	int sel = m_ListWaveLines.GetCurSel();
	if (sel != -1)
	{
		ShowControls();
	}else
	{
		ShowControls(false);
		return;
	}

	currentWave_ = environment->fixedWaves()->Select(sel);
	CEdit* ew=(CEdit*)GetDlgItem(IDC_TEXTURE_NAME);
	ew->SetWindowText(currentWave_->GetTextureName().c_str());

	m_Distance = currentWave_->distance();
	m_Speed = currentWave_->speed()*10;
	m_GenerationTime = currentWave_->generationTime();
	m_Invert = currentWave_->invertation()<0?TRUE:FALSE;
	m_SizeMin = currentWave_->sizeMin();
	m_SizeMax = currentWave_->sizeMax();
	UpdateData(FALSE);
	mouseMode_ = EDIT_POINTS;
}

void CWaveDlg::OnBnClickedAddPoint()
{
	mouseMode_ = CREATE_POINTS;
}

void CWaveDlg::OnBnClickedRemovePoint()
{
	if (currentWave_)
	{
		currentWave_->DeletePoint();
		currentWave_->SelectPoint(0);
		currentWave_->CreateSegments();
	}
}

void CWaveDlg::OnBnClickedRemoveWave()
{
	environment->fixedWaves()->DeleteWaves(currentWave_);
	currentWave_ = NULL;
	RefreshList();
}

void CWaveDlg::OnBnClickedSelectTexture()
{
	if (currentWave_)
	{
		string dataFileName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Textures", 
			"*.tga;*.avi", "bitmap.tga", "Will select location of an file textures");
		CEdit* ew=(CEdit*)GetDlgItem(IDC_TEXTURE_NAME);
		ew->SetWindowText(dataFileName.c_str());
		currentWave_->SetTexture(dataFileName);
	}
}

void CWaveDlg::OnBnClickedApply()
{
	UpdateData();
	if (currentWave_)
	{
		currentWave_->distance() = m_Distance;
		currentWave_->speed() = m_Speed/10;
		currentWave_->generationTime() = m_GenerationTime;
		currentWave_->invertation() = m_Invert?-1:1;
		currentWave_->sizeMin() = m_SizeMin;
		currentWave_->sizeMax() = m_SizeMax;
		currentWave_->CreateSegments();
	}
}


void CWaveDlg::OnOK()
{
	OnBnClickedApply();
}

void CWaveDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);
	int cnt = m_ListWaveLines.GetCount();
	if (cnt > 0)
	{
		m_ListWaveLines.SetCurSel(0);
		ShowControls();
	}else
	{
		ShowControls(false);
	}
}
