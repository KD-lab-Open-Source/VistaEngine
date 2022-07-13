#ifndef __SUR_TOOL_IMP_H_INCLUDED__
#define __SUR_TOOL_IMP_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"

// CSurToolImp dialog

class CSurToolImp : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolImp)

public:
	CSurToolImp(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolImp();

	CEScroll m_Width;
	CEScroll m_Height;
	CEScrollVx m_H;

	virtual bool onOperationOnMap(int x, int y);
// Dialog Data
	int getIDD() const { return IDD_BARDLG_IMP; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

#endif
