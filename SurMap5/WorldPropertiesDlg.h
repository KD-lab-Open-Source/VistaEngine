#ifndef __WORLD_PROPERTIES_DLG_H_INCLUDED__
#define __WORLD_PROPERTIES_DLG_H_INCLUDED__

class CWorldPropertiesDlg : public CDialog
{
	DECLARE_DYNAMIC(CWorldPropertiesDlg)

public:
	CWorldPropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CWorldPropertiesDlg();

// Dialog Data
	enum { IDD = IDD_DLG_FILE_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CString m_strMapSize;
};

#endif
