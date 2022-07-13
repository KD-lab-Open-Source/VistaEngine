#ifndef __S_TOOL_BAR_H_INCLUDED__
#define __S_TOOL_BAR_H_INCLUDED__

#include "SComboBox.h"

class CSToolBar : public CExtToolControlBar
{
	DECLARE_DYNAMIC(CSToolBar)

public:

	static UINT ID() { 
#ifdef _VISTA_ENGINE_EXTERNAL_
		return IDR_MAINFRAME_EXTERNAL;
#else
		return IDR_MAINFRAME;
#endif
	}

	CSToolBar();
	virtual ~CSToolBar();


	CImageList m_imgLstFormBrush;
	CComboBoxEx m_CBoxFormBrush;

	bool CreateExt(CWnd* pParentWnd);
protected:
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnUpdateBrushRadius(CCmdUI* pCmdUI);
	afx_msg void OnBrushRadiusComboSelChanged ();
};



#endif
