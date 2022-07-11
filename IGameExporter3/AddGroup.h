#pragma once


// CAddGroup dialog

class CAddGroup : public CDialog
{
	DECLARE_DYNAMIC(CAddGroup)

public:
	CAddGroup(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAddGroup();

// Dialog Data
	enum { IDD = IDD_ADD_GROUP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_Name;
protected:
	virtual void OnOK();
};
