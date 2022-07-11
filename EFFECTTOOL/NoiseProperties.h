#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "noisepreview.h"
#include "..\\Render\\src\\NParticle.h"


// CNoiseProperties dialog

class CNoiseProperties : public CDialog
{
	DECLARE_DYNAMIC(CNoiseProperties)

public:
	CNoiseProperties(CWnd* pParent = NULL);   // standard constructor
	virtual ~CNoiseProperties();

// Dialog Data
	enum { IDD = IDD_NOISE_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_OctavesCountCtrl;
	CSliderCtrl m_Octave[6];
	CSliderCtrl m_Octave2;
	CSliderCtrl m_Octave3;
	CSliderCtrl m_Octave4;
	CSliderCtrl m_Octave5;
	CSliderCtrl m_Octave6;
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	float m_OctaveAmplitude[6];
	CNoisePreview m_NoisePreview;
	afx_msg void OnCbnSelchangeOctavesCount();
	float m_Frequency;
	float m_Amplitude;
	void CalcNoise(bool refresh);
	PerlinNoise m_Noise;
	afx_msg void OnBnClickedRefresh();


	CKey octaves;
	afx_msg void OnBnClickedOk();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedMyOk();
	afx_msg void OnEnKillfocusFreq();
	afx_msg void OnEnKillfocusAmpl();
	BOOL m_Positive;
	afx_msg void OnBnClickedPositive();
};
