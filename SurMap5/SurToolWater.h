#ifndef __SUR_TOOL_WATER_H_INCLUDED__
#define __SUR_TOOL_WATER_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"

// CSurToolWaterSrc dialog

class CSurToolWaterSrc : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolWaterSrc)

public:
	CSurToolWaterSrc(int idd = IDD_BARDLG_WATERSRC, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolWaterSrc();

	unsigned int state_radio_button_InsDel;
	CEScrollVx m_DeltaH;

	void serialize(Archive& ar);

	bool CallBack_DrawAuxData(void);

// Dialog Data
	int getIDD() const { return IDD_BARDLG_WATERSRC; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedRdbInsertdeleteSource1();
	afx_msg void OnBnClickedRdbInsertdeleteSource2();
};


class CSurToolWater : public CSurToolWaterSrc
{
public:
	bool CallBack_OperationOnMap(int x, int y);
	void serialize(Archive& ar);
	bool CallBack_DrawAuxData(void);

// Dialog Data
	int getIDD() const { return IDD_BARDLG_WATERSRC; }

public:
	virtual BOOL OnInitDialog();
};

#endif
