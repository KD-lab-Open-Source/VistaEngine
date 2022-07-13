#ifndef __SUR_TOOL_WIND_STATIC_H_INCLUDED__
#define __SUR_TOOL_WIND_STATIC_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"

#include "Serialization.h"

// CSurToolWindStatic dialog

class CSurToolWindStatic : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolWindStatic)
	bool mouse_down;
	float cur_z;
	bool is_clear;
public:
	CSurToolWindStatic(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolWindStatic();

	bool CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool CallBack_LMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool CallBack_LMBUp (const Vect3f& worldCoord, const Vect2i& screenCoord);

	bool DrawAuxData(void);

// Dialog Data
	int getIDD() const { return IDD_BARDLG_WINDSTATIC; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeEditWindZ();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedChClearWind();
};

#endif
