#pragma once

#include "SurToolAux.h"
//#include "Resource.h"

// CSurToolLighting dialog

class CSurToolLighting : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolLighting)

public:
	CSurToolLighting(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolLighting();

	bool CallBack_OperationOnMap(int x, int y);
	bool DrawAuxData();

// Dialog Data
	enum { IDD = IDD_BARDLG_LIGHTING };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	

	DECLARE_MESSAGE_MAP()
};
