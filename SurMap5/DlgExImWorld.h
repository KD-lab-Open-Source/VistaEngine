#ifndef __DLGEXIMWORLD_H__
#define __DLGEXIMWORLD_H__

#include "ListViewCtrlEx.h"
#include "string"

class CDlgExImWorld : public CDialog
{
	DECLARE_DYNAMIC(CDlgExImWorld)
public:
	CDlgExImWorld(const char* _path2worlds, const char* _title, bool _enableCreateDir = false, CWnd* pParent = 0);
	virtual ~CDlgExImWorld();

	bool enableCreateDir;
	string title;
	string path2worlds;
	string selectWorldName;

	enum { IDD = IDD_DLG_EXIMWORLD };

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
	afx_msg void OnButtonImport();
	afx_msg void OnButtonExport();
};

#endif //__DLGEXIMWORLD_H__
