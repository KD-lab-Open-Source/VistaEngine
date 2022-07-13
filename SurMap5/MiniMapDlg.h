#ifndef __MINI_MAP_DLG_H_INCLUDED__
#define __MINI_MAP_DLG_H_INCLUDED__

//#include "Resource.h"
#include "MiniMapWindow.h"

// CMiniMapDlg dialog

class CMiniMapDlg : public CDialog
{
	DECLARE_DYNAMIC(CMiniMapDlg)

public:
	CMiniMapDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMiniMapDlg();

	CMiniMapWindow* getMiniMap () {
		return &m_ctlMiniMap;
	}

// Dialog Data
	enum { IDD = IDD_BARDLG_MINIMAP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CMiniMapWindow m_ctlMiniMap;
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif
