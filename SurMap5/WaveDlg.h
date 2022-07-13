#ifndef __WAVE_DLG_H_INCLUDED__
#define __WAVE_DLG_H_INCLUDED__

#include "..\Water\Waves.h"

class CWaveDlg : public CDialog
{
	DECLARE_DYNAMIC(CWaveDlg)

public:
	CWaveDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CWaveDlg();

// Dialog Data
	enum { IDD = IDD_DLG_WAVE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
protected:
	cFixedWaves* currentWave_;
public:
	enum {
		NONE,
		CREATE_POINTS,
		SELECT_POINTS,
		EDIT_POINTS,
		NEW_POINTS,
	} mouseMode_;
	CListBox m_ListWaveLines;

public:
	void OnMapLBClick(const Vect3f& coord);
	void OnMapRBClick(const Vect3f& coord);
	void OnMapMouseMove(const Vect3f& coord);

	void RefreshList();

	afx_msg void OnBnClickedCreateWave();
	afx_msg void OnLbnSelchangeListWaveLines();
	afx_msg void OnBnClickedAddPoint();
	afx_msg void OnBnClickedRemovePoint();
	afx_msg void OnBnClickedRemoveWave();
	afx_msg void OnBnClickedSelectTexture();

	float m_Distance;
	float m_Speed;
	float m_GenerationTime;
	BOOL m_Invert;
	afx_msg void OnBnClickedApply();
	void ShowControls(bool show = TRUE);
protected:
	virtual void OnCancel();
	virtual void OnOK();
public:
	float m_SizeMin;
	float m_SizeMax;
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
};

#endif
