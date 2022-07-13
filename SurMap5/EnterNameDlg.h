#ifndef __ENTER_NAME_DLG_H_INCLUDED__
#define __ENTER_NAME_DLG_H_INCLUDED__

// CEnterNameDlg dialog

class CEnterNameDlg : public CDialog
{
	DECLARE_DYNAMIC(CEnterNameDlg)

public:

	CEnterNameDlg(CWnd* pParent = NULL);   // standard constructor
	CEnterNameDlg(const char* text, CWnd* pParent = NULL);   // standard constructor
	virtual ~CEnterNameDlg();

// Dialog Data
	enum { IDD = IDD_DLG_ENTERNAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit text;
	char buf[50];
protected:
	virtual void OnOK();
	virtual void OnCancel();
public:
	virtual BOOL OnInitDialog();
};

#endif
