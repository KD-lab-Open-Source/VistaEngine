#ifndef __SUR_TOOL_GRASS_H_INCLUDED__
#define __SUR_TOOL_GRASS_H_INCLUDED__

#include "SurToolAux.h"
#include "SurToolEditable.h"
#include "Resource.h"

class CSurToolGrass : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolGrass)
public:
	CSurToolGrass(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolGrass();

	int getIDD() const { return IDD_BARDLG_GRASS; }

	bool onDrawAuxData();
	bool onOperationOnMap(int x, int y);
	bool onDrawPreview(int width, int height);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	static int nextGrass;
	int grassNumber;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	int intensity1[7];
	int intensity2[7];
	int density[7];
	int oldSelectedGrass;

public:
	afx_msg void OnBnClickedSelectTexture();
	afx_msg void OnBnClickedApply();
	afx_msg void OnCbnSelchangeGrassList();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};

#endif