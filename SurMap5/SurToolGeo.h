#ifndef __SUR_TOOL_GEO_H_INCLUDED__
#define __SUR_TOOL_GEO_H_INCLUDED__
#if !defined(__EXT_TEMPL_H)
	#include <ExtTempl.h>
#endif

#include "EScroll.h"
#include "SurToolAux.h"

// CSurToolGeo dialog

class CSurToolGeo : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolGeo)

public:
	CSurToolGeo(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolGeo();

	CEScroll m_Smpl;

	virtual bool CallBack_OperationOnMap(int x, int y);
// Dialog Data
	int getIDD() const { return IDD_BARDLG_GEO; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	virtual BOOL OnInitDialog();
//	afx_msg void OnEnChangeEdit1();
};

#endif
