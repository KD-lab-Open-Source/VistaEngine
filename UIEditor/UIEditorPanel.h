#ifndef __UI_EDITOR_PANEL_H_INCLUDED__
#define __UI_EDITOR_PANEL_H_INCLUDED__

class CControlsTreeCtrl;
class CAttribEditorCtrl;

class CUIEditorPanel : public CWnd
{
	DECLARE_DYNCREATE(CUIEditorPanel)
public:
	CUIEditorPanel();
	virtual ~CUIEditorPanel();

	CImageList controlsTreeImages_;
	
	CAttribEditorCtrl& attribEditor() { return *attribEditor_; }
	CControlsTreeCtrl& controlsTree() { return *controlsTree_; }

	CControlsTreeCtrl* controlsTree_;
protected:
	DECLARE_MESSAGE_MAP()
public:
    CAttribEditorCtrl* attribEditor_;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif
