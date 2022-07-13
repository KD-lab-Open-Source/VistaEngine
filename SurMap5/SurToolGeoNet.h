#ifndef __SUR_TOOL_GEO_NET_H_INCLUDED__
#define __SUR_TOOL_GEO_NET_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"

// CSurToolGeoNet dialog

class CSurToolGeoNet : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolGeoNet)

public:
	CSurToolGeoNet(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolGeoNet();

	bool flag_init_dialog;
	//CEScroll m_Width;
	//CEScroll m_Height;
	CEScrollVx m_H;

	CVoxVarWin m_GNoise;
	CIntVarWin m_Mesh;
	int m_PowCellSize;
	int m_PowShiftCS4RG;
	int m_Curving;

	virtual bool onOperationOnMap(int x, int y);

	void serialize(Archive& ar);

// Dialog Data
	int getIDD() const { return IDD_BARDLG_GEONET; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

#endif
