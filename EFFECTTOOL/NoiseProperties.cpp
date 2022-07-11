// NoiseProperties.cpp : implementation file
//

#include "stdafx.h"
#include "EffectTool.h"
#include "NoiseProperties.h"
#include ".\noiseproperties.h"


// CNoiseProperties dialog

IMPLEMENT_DYNAMIC(CNoiseProperties, CDialog)
CNoiseProperties::CNoiseProperties(CWnd* pParent /*=NULL*/)
	: CDialog(CNoiseProperties::IDD, pParent)
	, m_Frequency(1)
	, m_Amplitude(1)
	, m_Positive(FALSE)
{
	ZeroMemory(m_OctaveAmplitude,sizeof(float)*6);
}

CNoiseProperties::~CNoiseProperties()
{
}

void CNoiseProperties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OCTAVES_COUNT, m_OctavesCountCtrl);
	DDX_Control(pDX, IDC_SLIDER_OCTAVE1, m_Octave[0]);
	DDX_Control(pDX, IDC_SLIDER_OCTAVE2, m_Octave[1]);
	DDX_Control(pDX, IDC_SLIDER_OCTAVE3, m_Octave[2]);
	DDX_Control(pDX, IDC_SLIDER_OCTAVE4, m_Octave[3]);
	DDX_Control(pDX, IDC_SLIDER_OCTAVE5, m_Octave[4]);
	DDX_Control(pDX, IDC_SLIDER_OCTAVE6, m_Octave[5]);
	DDX_Text(pDX, IDC_AMPLITUDE1, m_OctaveAmplitude[0]);
	DDX_Text(pDX, IDC_AMPLITUDE2, m_OctaveAmplitude[1]);
	DDX_Text(pDX, IDC_AMPLITUDE3, m_OctaveAmplitude[2]);
	DDX_Text(pDX, IDC_AMPLITUDE4, m_OctaveAmplitude[3]);
	DDX_Text(pDX, IDC_AMPLITUDE5, m_OctaveAmplitude[4]);
	DDX_Text(pDX, IDC_AMPLITUDE6, m_OctaveAmplitude[5]);
	DDX_Control(pDX, IDC_PREVIEW, m_NoisePreview);
	DDX_Text(pDX, IDC_FREQ, m_Frequency);
	DDX_Text(pDX, IDC_AMPL, m_Amplitude);
	DDX_Check(pDX, IDC_POSITIVE, m_Positive);
}


BEGIN_MESSAGE_MAP(CNoiseProperties, CDialog)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_OCTAVES_COUNT, OnCbnSelchangeOctavesCount)
	ON_BN_CLICKED(IDC_REFRESH, OnBnClickedRefresh)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_OK, OnBnClickedMyOk)
	ON_EN_KILLFOCUS(IDC_FREQ, OnEnKillfocusFreq)
	ON_EN_KILLFOCUS(IDC_AMPL, OnEnKillfocusAmpl)
	ON_BN_CLICKED(IDC_POSITIVE, OnBnClickedPositive)
END_MESSAGE_MAP()


// CNoiseProperties message handlers

BOOL CNoiseProperties::OnInitDialog()
{
	CDialog::OnInitDialog();

	for (int i=0; i<6; i++)
	{
		m_Octave[i].SetRange(0,1000);
		m_Octave[i].SetPos(500);
	}
	float ampl = 0.5;
	m_OctaveAmplitude[0] = ampl;
	ampl /=2;
	m_OctaveAmplitude[1] = ampl;
	ampl /=2;
	m_OctaveAmplitude[2] = ampl;
	ampl /=2;
	m_OctaveAmplitude[3] = ampl;
	ampl /=2;
	m_OctaveAmplitude[4] = ampl;
	ampl /=2;
	m_OctaveAmplitude[5] = ampl;
	for (int i=0; i<octaves.size(); i++)
	{
		m_OctaveAmplitude[i] = octaves[i].f;
		float f = 0.5f/pow(2,i);
		int pos;
		if(m_OctaveAmplitude[i]>0.5f/pow(2,i))
		{
			pos = (m_OctaveAmplitude[i]-f)/(1-f)*500+500;

		}else
		{
			pos = m_OctaveAmplitude[i]/f*500;
		}
		m_Octave[i].SetPos(pos);
	}
	if (octaves.size()==0)
		octaves.resize(1);
	m_OctavesCountCtrl.AddString("1");
	m_OctavesCountCtrl.AddString("2");
	m_OctavesCountCtrl.AddString("3");
	m_OctavesCountCtrl.AddString("4");
	m_OctavesCountCtrl.AddString("5");
	m_OctavesCountCtrl.AddString("6");
	m_OctavesCountCtrl.SetCurSel(octaves.size()-1);

	UpdateData(FALSE);
	OnCbnSelchangeOctavesCount();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CNoiseProperties::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	DWORD id = pScrollBar->GetDlgCtrlID();
	switch(id) {
	case IDC_SLIDER_OCTAVE1:
		{
			int pos = m_Octave[0].GetPos();
//			m_Amplitude1.Format("%01.04f",float(pos)/1000.f);
			m_OctaveAmplitude[0] = float(pos)/1000.f;
		}
		break;
	case IDC_SLIDER_OCTAVE2:
		{
			int pos = m_Octave[1].GetPos();
			float ampl;
			if(pos > 500)
			{
				ampl = float(pos-500)/500.f*0.75f+0.25f;
			}else
			{
				ampl = float(pos)/500.f*0.25f;
			}
			//m_Amplitude2.Format("%01.04f",ampl);
			m_OctaveAmplitude[1] = ampl;
		}
		break;
	case IDC_SLIDER_OCTAVE3:
		{
			int pos = m_Octave[2].GetPos();
			float ampl;
			if(pos > 500)
			{
				ampl = float(pos-500)/500.f*0.875f+0.125f;
			}else
			{
				ampl = float(pos)/500.f*0.125f;
			}
			//m_Amplitude3.Format("%01.04f",ampl);
			m_OctaveAmplitude[2] = ampl;
		}
		break;
	case IDC_SLIDER_OCTAVE4:
		{
			int pos = m_Octave[3].GetPos();
			float ampl;
			if(pos > 500)
			{
				ampl = float(pos-500)/500.f*0.9375f+0.0625f;
			}else
			{
				ampl = float(pos)/500.f*0.0625f;
			}
			m_OctaveAmplitude[3] = ampl;
			//m_Amplitude4.Format("%01.04f",ampl);
		}
		break;
	case IDC_SLIDER_OCTAVE5:
		{
			int pos = m_Octave[4].GetPos();
			float ampl;
			if(pos > 500)
			{
				ampl = float(pos-500)/500.f*0.96875f+0.03125f;
			}else
			{
				ampl = float(pos)/500.f*0.03125f;
			}
			m_OctaveAmplitude[4] = ampl;
			//m_Amplitude5.Format("%01.04f",ampl);
		}
		break;
	case IDC_SLIDER_OCTAVE6:
		{
			int pos = m_Octave[5].GetPos();
			float ampl;
			if(pos > 500)
			{
				ampl = float(pos-500)/500.f*0.984375f+0.015625f;
			}else
			{
				ampl = float(pos)/500.f*0.015625f;
			}
			m_OctaveAmplitude[5] = ampl;
			//m_Amplitude6.Format("%01.04f",ampl);
		}
		break;
	}
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
	UpdateData(FALSE);
	CalcNoise(false);
}

void CNoiseProperties::OnCbnSelchangeOctavesCount()
{
	int n = m_OctavesCountCtrl.GetCurSel();
	for (int i=1; i<6;i++)
	{
		m_Octave[i].EnableWindow(i<=n);
	}
	GetDlgItem(IDC_AMPLITUDE2)->EnableWindow(1<=n);
	GetDlgItem(IDC_AMPLITUDE3)->EnableWindow(2<=n);
	GetDlgItem(IDC_AMPLITUDE4)->EnableWindow(3<=n);
	GetDlgItem(IDC_AMPLITUDE5)->EnableWindow(4<=n);
	GetDlgItem(IDC_AMPLITUDE6)->EnableWindow(5<=n);

	GetDlgItem(IDC_OCTAVE_LABEL2)->EnableWindow(1<=n);
	GetDlgItem(IDC_OCTAVE_LABEL3)->EnableWindow(2<=n);
	GetDlgItem(IDC_OCTAVE_LABEL4)->EnableWindow(3<=n);
	GetDlgItem(IDC_OCTAVE_LABEL5)->EnableWindow(4<=n);
	GetDlgItem(IDC_OCTAVE_LABEL6)->EnableWindow(5<=n);
	CalcNoise(true);
}
void CNoiseProperties::CalcNoise(bool refresh)
{
	UpdateData();
	vector<float> &points = m_NoisePreview.m_points;
	CKey amplitudes;
	RECT rect;
	m_NoisePreview.GetClientRect(&rect);
	points.resize(rect.right-rect.left);
	int octaves = m_OctavesCountCtrl.GetCurSel()+1;
	float freq = m_Frequency;
	float ampl = m_Amplitude;
	amplitudes.resize(octaves);
	for (int i=0; i<octaves; i++)
	{
		amplitudes[i].f = m_OctaveAmplitude[i];
	}
	m_Noise.SetParameters(16,freq,ampl,amplitudes,m_Positive,refresh);
	int size = points.size();
	for (int i=0; i<size; i++)
	{
		points[i] = m_Noise.Get(float(i)/(float(size)/2));
	}
	m_NoisePreview.Invalidate(FALSE);
}

void CNoiseProperties::OnBnClickedRefresh()
{
	CalcNoise(true);
}

void CNoiseProperties::OnBnClickedOk()
{
	CalcNoise(true);
	//OnOK();
}

BOOL CNoiseProperties::PreTranslateMessage(MSG* pMsg)
{
	//if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	//	return false;
	return CDialog::PreTranslateMessage(pMsg);
}

void CNoiseProperties::OnBnClickedMyOk()
{
	UpdateData();
	int n = m_OctavesCountCtrl.GetCurSel()+1;
	octaves.resize(n);
	for (int i=0; i<n;i++)
	{
		octaves[i].f = m_OctaveAmplitude[i];
	}
	EndDialog(IDOK);
}

void CNoiseProperties::OnEnKillfocusFreq()
{
	UpdateData();
	CalcNoise(true);
}

void CNoiseProperties::OnEnKillfocusAmpl()
{
	UpdateData();
	CalcNoise(true);
}

void CNoiseProperties::OnBnClickedPositive()
{
	UpdateData();
	CalcNoise(true);
}
