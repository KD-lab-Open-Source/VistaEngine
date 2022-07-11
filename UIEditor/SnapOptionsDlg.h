#ifndef __SNAP_OPTIONS_DLG_H_INCLUDED__
#define __SNAP_OPTIONS_DLG_H_INCLUDED__

class CSnapOptionsDlg : public CDialog
{
	DECLARE_DYNAMIC(CSnapOptionsDlg)

public:
	CSnapOptionsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSnapOptionsDlg();

// Dialog Data
	enum { IDD = IDD_UI_SNAP_OPTIONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	BOOL m_bShowGrid;
	BOOL m_bSnapToGrid;
	float m_fGridXSize;
	float m_fGridYSize;
	int m_iLargeGridXSize;
	int m_iLargeGridYSize;
protected:
	virtual void OnOK();
};

#endif
