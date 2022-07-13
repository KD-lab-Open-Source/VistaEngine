#ifndef __SUR_TOOL_BLUR_H_INCLUDED__
#define __SUR_TOOL_BLUR_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"

// CSurToolBlur dialog

class CSurToolBlur : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolBlur)

public:
	CSurToolBlur(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolBlur();

	CEScroll m_RadiusBlur;
	int m_Intensity;

	virtual bool onOperationOnMap(int x, int y);
	virtual bool onDrawAuxData(void);
// Dialog Data
	int getIDD() const { return IDD_BARDLG_BLUR; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};

#endif
