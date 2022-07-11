#pragma once
#include "afxcmn.h"
//#include "MainFrm.h"
class cFrame;
class CMainFrame;
// CPhaseControlDialog dialog

class CPhaseControlDialog : public CDialog
{
	DECLARE_DYNAMIC(CPhaseControlDialog)

public:
	CPhaseControlDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPhaseControlDialog();

// Dialog Data
	enum { IDD = IDD_PHASE_CONTROL_DIALOG };

	void SetFrame(cFrame* view, CMainFrame* frm);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	cFrame* frame_;
	CMainFrame* mainFrame;
public:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
protected:
	CSliderCtrl phaseSlider;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
protected:
	virtual void OnCancel();
public:
	CString phaseEdit;
	afx_msg void OnBnClickedCancel();
};
