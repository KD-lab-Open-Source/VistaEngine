#ifndef __DLGSELECTWORLD_H__
#define __DLGSELECTWORLD_H__

#include "ListViewCtrlEx.h"
#include "string"

class CDlgSelectWorld : public CDialog
{
	DECLARE_DYNAMIC(CDlgSelectWorld)
public:
	CDlgSelectWorld(const char* _path2worlds, const char* _title, bool _enableCreateDir = false, CWnd* pParent = 0);
	virtual ~CDlgSelectWorld();

	bool enableCreateDir;
	string title;
	string path2worlds;
	string selectWorldName;

	enum { IDD = IDD_DLG_SELECTWORLD };

	static int previsionWorldSelect;

	void clearWorldList();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
public:
	CListCtrlEx m_listWorld;
	void fillWorldList();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
protected:
	virtual void OnOK();
public:
	afx_msg void OnButtonNew();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonRename();
	afx_msg void OnListDoubleClick(NMHDR *pNMHDR, LRESULT *pResult);
};

#endif //__DLGSELECTWORLD_H__
