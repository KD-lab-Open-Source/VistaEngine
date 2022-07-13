#ifndef __COLOR_SELECTOR_WINDOW_H_INCLUDED__
#define __COLOR_SELECTOR_WINDOW_H_INCLUDED__

#include "Handle.h"
#include "Umath.h"
class LayoutWindow;
class CColorRampCtrl;
class CColorSelector;
class CSolidColorCtrl;

class CColorSelectorWindow : public CWnd{
    DECLARE_DYNAMIC(CColorSelectorWindow)
public:
	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);
	bool doModal(CWnd* parent = 0);

    static const char* className() { return "VistaEngineColorSelectorWindow"; }


	void setColor(const sColor4f& color);
	const sColor4f& getColor() const{ return color_; }

    CColorSelectorWindow(bool hasAlpha = true, CWnd* parent = 0);
    virtual ~CColorSelectorWindow();
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKeyDown(UINT nChar, UINT repCnt, UINT flags);
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	BOOL PreTranslateMessage(MSG* msg);

	void onColorChanged(int index);

	sColor4f color_;
    bool modalResult_;
    PtrHandle<LayoutWindow> layout_;
    PtrHandle<CColorRampCtrl> colorRamp_;
	PtrHandle<CColorSelector> colorSelector_;
	CButton okButton_;
	CButton cancelButton_;

    DECLARE_MESSAGE_MAP()
};

#endif
