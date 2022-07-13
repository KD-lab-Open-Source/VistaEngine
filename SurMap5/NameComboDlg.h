#ifndef __NAME_COMBO_DLG_H_INCLUDED__
#define __NAME_COMBO_DLG_H_INCLUDED__

// CNameComboDlg dialog

class CNameComboDlg : public CDialog
{
	DECLARE_DYNAMIC(CNameComboDlg)

public:
	CNameComboDlg(const char* title, const char* name, const char* combo);   // standard constructor
	virtual ~CNameComboDlg();

    const char* name() { return m_strName; }

// Dialog Data
	enum { IDD = IDD_DLG_NAME_COMBO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
protected:
	virtual void OnOK();
private:
	CString m_strName;
	CString m_strTitle;
	CString m_strCombo;
};

#endif
