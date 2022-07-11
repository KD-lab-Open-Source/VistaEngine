#include "StdAfx.h"
#include "UIEditor_Utils.h"
#include "MainFrame.h"

const char* editEffectLibraryName(HWND hwnd, const char* initialString) {
	return "";
}

const char* editVoiceName(HWND hwnd, const char* initialString) {
	return "";
}


const char* editShowHeadNameDialog(HWND hwnd, const char* initialString) {
	return "";
}

CUIMainFrame* uiEditorFrame()
{
	return (CUIMainFrame*)AfxGetMainWnd();
}
