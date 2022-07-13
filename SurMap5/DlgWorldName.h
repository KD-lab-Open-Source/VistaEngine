#ifndef __DLG_WORLD_NAME_H_INCLUDED__
#define __DLG_WORLD_NAME_H_INCLUDED__

// CDlgWorldName dialog

class CDlgWorldName : public CDialog
{
	DECLARE_DYNAMIC(CDlgWorldName)

public:
	CDlgWorldName(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgWorldName();

// Dialog Data
	enum { IDD = IDD_DLG_WORLDNAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString newWorldName;
	virtual BOOL OnInitDialog();
};

#endif
