#ifndef __SUR_TOOL_KIND_H_INCLUDED__
#define __SUR_TOOL_KIND_H_INCLUDED__

#include "SurToolAux.h"

// CSurToolKind dialog

class CSurToolKind : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolKind)

public:
	CSurToolKind(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolKind();

	unsigned int state_radiobutton_Kind;
	void setKindState(int state);

	bool CallBack_DrawAuxData(void);
	bool CallBack_OperationOnMap(int x, int y);
	void serialize(Archive& ar);
// Dialog Data
	int getIDD() const { return IDD_BARDLG_HARDNESS; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnKindButtonClicked(UINT nID);
	afx_msg void OnDestroy();
};

#endif
