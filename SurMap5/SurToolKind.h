#ifndef __SUR_TOOL_KIND_H_INCLUDED__
#define __SUR_TOOL_KIND_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"
#include "MFC\SizeLayoutManager.h"

// CSurToolKind dialog

class CSurToolKind : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolKind)

public:
	CSurToolKind(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolKind();

	unsigned int state_radiobutton_Kind;
	//void setKindState(int state);

	CEScrollVx m_FilterMinH;
	CEScrollVx m_FilterMaxH;
	static int filterMinHValue;
	static int filterMaxHValue;
	static bool flag_EnableFilterH;

	bool onDrawAuxData(void);
	bool onOperationOnMap(int x, int y);

	void serialize(Archive& ar);
	static void staticSerialize(Archive& ar);
// Dialog Data
	int getIDD() const { return IDD_BARDLG_HARDNESS; }

protected:
    CSizeLayoutManager layout_;
	void refrawAux();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCheckEnableHFilter();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKindButtonClicked(UINT nID);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif
