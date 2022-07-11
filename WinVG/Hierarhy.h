#pragma once
#include "afxcmn.h"


// CHierarhy dialog

class CHierarhy : public CDialog
{
	DECLARE_DYNAMIC(CHierarhy)

public:
	CHierarhy(cObject3dx* pObject,CWnd* pParent = NULL);   // standard constructor
	virtual ~CHierarhy();

// Dialog Data
	enum { IDD = IDD_SHOW_HIERARCHY };
	int selected_object;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	void Init();
public:
	CTreeCtrl tree;
	cObject3dx* pObject;
	vector<HTREEITEM> nodes;
};
