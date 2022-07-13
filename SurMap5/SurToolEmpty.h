#ifndef __SUR_TOOL_EMPTY_H_INCLUDED__
#define __SUR_TOOL_EMPTY_H_INCLUDED__

#include "SurToolAux.h"

// CSurToolEmpty dialog
class CSurToolEmpty : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolEmpty)

public:
	CSurToolEmpty(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolEmpty();

// Dialog Data
	int getIDD() const { return IDD_BARDLG_EMPTY; }
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:

public:
};

#endif
