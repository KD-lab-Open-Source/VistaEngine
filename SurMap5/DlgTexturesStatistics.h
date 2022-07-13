#ifndef __DLG_TEXTURES_STATISTICS_H_INCLUDED__
#define __DLG_TEXTURES_STATISTICS_H_INCLUDED__
#include "SortListCtrl.h"

// CDlgTexturesStatistics dialog

class CDlgTexturesStatistics : public CDialog
{
	DECLARE_DYNAMIC(CDlgTexturesStatistics)

public:
	CDlgTexturesStatistics(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgTexturesStatistics();

// Dialog Data
	enum { IDD = IDD_DLG_TEXTURES_STATISTICS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CSortListCtrl m_TexturesList;
	CString m_NumTextures;
	CString m_TexturesSize;
	virtual BOOL OnInitDialog();
};

#endif
