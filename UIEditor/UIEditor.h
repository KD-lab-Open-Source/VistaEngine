#ifndef __U_I_EDITOR_H_INCLUDED__
#define __U_I_EDITOR_H_INCLUDED__

#define UIEDITOR_GUID "SURMAP5_UIEDITOR_UNIQUE_GUID"

class CUIMainFrame;

extern const UINT WM_CHECK_ITS_ME;

class CUIEditorApp : public CWinApp
{
public:
	CUIEditorApp();
	static CUIMainFrame* GetMainFrame ();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnAppAbout();
	virtual BOOL OnIdle (LONG lCount);

	bool active_;
};

extern CUIEditorApp theApp;

#endif
