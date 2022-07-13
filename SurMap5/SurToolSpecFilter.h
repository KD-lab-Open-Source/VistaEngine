#pragma once

#include "EScroll.h"
#include "SurToolAux.h"

// CSurToolSpecFilter dialog

class CSurToolSpecFilter : public CSurToolBase//CDialog
{
	DECLARE_DYNAMIC(CSurToolSpecFilter)

public:
	CSurToolSpecFilter(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolSpecFilter();

	CEScrollVx maxDH;
	CEScroll kDetail;
	CEScroll balansH;
	CEScroll kUnBrightness;

	bool onOperationOnMap(int x, int y); //virtual
	bool onDrawAuxData(void); //virtual

	void serialize(Archive& ar);
	// Dialog Data
	int getIDD() const { return IDD_BARDLG_SPECFILTER; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};
