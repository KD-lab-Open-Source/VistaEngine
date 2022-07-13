#ifndef __SUR_MAP5_H_INCLUDED__
#define __SUR_MAP5_H_INCLUDED__

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif



// CSurMap5App:
// See SurMap5.cpp for the implementation of this class
//
class CSurMap5App : public CWinApp
{
	DECLARE_MESSAGE_MAP()
public:
	CSurMap5App();
	virtual ~CSurMap5App();

protected:
	CMenu* m_pMainFrameMenu;
// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	void SetupUiAdvancedOptions(CWnd* wnd);

public:
	afx_msg void OnAppAbout();
	virtual BOOL OnIdle(LONG lCount);
	bool flag_animation;
	afx_msg void OnViewAnimation();
	afx_msg void OnUpdateViewAnimation(CCmdUI *pCmdUI);
};

extern CSurMap5App theApp;

//AUX interface function & data
#define EXTENDED_COLOR_BITMAP_IN_CONTROL
extern CSize MakeVoluntaryImageList(UINT inBitmapID, UINT toolBarBitDepth, int amountImg, CImageList&	outImageList);
const COLORREF ALFA_COLOR_IN_BITMAP_IN_CONTROL=RGB(255,0,255);

#endif
