#ifndef __TEST_FRAME_H_INCLUDED__
#define __TEST_FRAME_H_INCLUDED__

#include "../AttribEditor/AttribEditorCtrl.h"

class CTestFrame : public CFrameWnd
{
    DECLARE_DYNAMIC (CTestFrame)

public:
    CTestFrame ();
    virtual ~CTestFrame ();

	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnSize(UINT nType, int cx, int cy);
protected:
	CAttribEditorCtrl* m_pAttribEditor;
	InterfaceAttributes m_InterfaceAttributes;

    DECLARE_MESSAGE_MAP ()
};

#endif
