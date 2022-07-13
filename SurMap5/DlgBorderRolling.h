#pragma once


// DlgBorderRolling dialog

class DlgBorderRolling : public CDialog
{
	DECLARE_DYNAMIC(DlgBorderRolling)

public:
	DlgBorderRolling(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgBorderRolling();

// Dialog Data
	enum { IDD = IDD_DLG_BORDER_ROLLING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int borderHeight;
	int boderAngle;
};
