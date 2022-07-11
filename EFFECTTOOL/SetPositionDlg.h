#pragma once


// CSetPositionDlg dialog

class CSetPositionDlg : public CDialog
{
	DECLARE_DYNAMIC(CSetPositionDlg)

public:
	CSetPositionDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSetPositionDlg();

// Dialog Data
	enum { IDD = IDD_SET_KEY_POSITION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	float& Position(){return m_fPosition;}
	float m_fPosition;
protected:
	virtual void OnOK();
};
