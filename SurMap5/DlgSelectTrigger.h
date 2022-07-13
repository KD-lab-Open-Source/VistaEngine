#ifndef __DLGSELECTTRIGGER_H__
#define __DLGSELECTTRIGGER_H__

//#include "Resource.h"
#include "ListViewCtrlEx.h"
#include "string"

// CDlgSelectTrigger dialog

class CDlgSelectTrigger : public CDialog
{
	DECLARE_DYNAMIC(CDlgSelectTrigger)

public:
	CDlgSelectTrigger(const char* _path2triggersFiles, const char* _title, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSelectTrigger();

	string title;
	string path2triggersFiles;
	string selectTriggersFile;

	CListCtrlEx m_listTriggers;
	void fillTriggerList();

// Dialog Data
	enum { IDD = IDD_DLG_SELECTTRIGGER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
protected:
	virtual void OnOK();
public:
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnNewtrigger();
	afx_msg void OnBnClickedBtnDeletetrigger();
	afx_msg void OnCopyTriggerButton();
};

#endif //__DLGSELECTTRIGGER_H__
