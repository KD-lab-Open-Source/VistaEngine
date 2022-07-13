#ifndef __CREATE_ATTRIB_DLG_H_INCLUDED__
#define __CREATE_ATTRIB_DLG_H_INCLUDED__

class CCreateAttribDlg : public CDialog
{
	DECLARE_DYNAMIC(CCreateAttribDlg)

public:
	CCreateAttribDlg(bool allowPaste, bool pasteByDefault, const char* title, const char* defaultName = 0, CWnd* pParent = 0);
	virtual ~CCreateAttribDlg();

	enum{ IDD = IDD_DLG_CREATE_UNIT };

    const char* name() const { return name_.c_str(); };
    bool paste() const{ return paste_; }
protected:
	std::string name_;
    std::string title_;
    std::string defaultName_;
	CButton pasteCheck_;
	CEdit nameEdit_;
    bool allowPaste_;
    bool pasteByDefault_;

    bool paste_;

    // virtuals:
	void OnOK();
	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);

	DECLARE_MESSAGE_MAP()
};

#endif
