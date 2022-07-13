#ifndef __DLG_STATISTICS_SHOW_H_INCLUDED__
#define __DLG_STATISTICS_SHOW_H_INCLUDED__

#include <map>
#include "..\Render\inc\IVisGeneric.h"
#include "SortListCtrl.h"

#include <set>
#include "afxcmn.h"
// CDlgStatisticsShow dialog
struct UniqObj
{
	cObject3dx* obj;
	int count;
};

class CDlgStatisticsShow : public CDialog
{
	DECLARE_DYNAMIC(CDlgStatisticsShow)

public:
	CDlgStatisticsShow(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgStatisticsShow();

// Dialog Data
	enum { IDD = IDD_DLG_STATISTICS_SHOW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	CString m_EditStat;
	vector<cObject3dx*> objects;
	vector<UniqObj> uniqueObjects;
	set<string> uniqueTextures;
	vector<ListSimply3dx> simply_objects;
	DWORD totalSimplyVertexSize;
	DWORD totalSimplyVertexCount;
	DWORD totalSimplyObjectCount;
	DWORD totalVertexSize;
	DWORD totalTextureSize;
	DWORD totalObjects;
	bool ShowObjects3dx;

public:
	virtual BOOL OnInitDialog();
	cTexture* GetTexture(string name);
	void CalcUniqueTextureSize();
	void PrepareUniqueObjects();
	void CalcTotalSimplyObjects();
	void CalcTotalVertexSize();
	CString GetFormatedSize(DWORD size);
	void SetGeneralInfo();
	void SetObjectsList();
	void SetTextureInfo(cObject3dx* obj, int numInList);
	void SetTextureList(cObject3dx* obj);
	void Recalculate();

	CString m_TotalObjects;
	CString m_UniqueObjects;
	CString m_TotalTextures;
	CString m_TexturesSize;
	CString m_VertexSize;
	CSortListCtrl m_ObjectsList;
	CSortListCtrl m_TexturesList;
	afx_msg void OnLvnItemchangedObjectsList(NMHDR *pNMHDR, LRESULT *pResult);
	BOOL m_bVisibleObjects;
	afx_msg void OnBnClickedOnlyVisibleObjects();
	afx_msg void OnBnClickedAllTextures();
	CTabCtrl m_TabCtrl;
	CTabCtrl m_TabObjects;
	afx_msg void OnTcnSelchangeTabObjects(NMHDR *pNMHDR, LRESULT *pResult);
	CSortListCtrl m_SimplyList;
};

#endif
