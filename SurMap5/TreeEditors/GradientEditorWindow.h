#ifndef __GRADIENT_EDITOR_WINDOW_H_INCLUDED__
#define __GRADIENT_EDITOR_WINDOW_H_INCLUDED__

#include "Handle.h"

enum GradientPositionMode;

class LayoutWindow;
class CColorSelector;

class CKeyColor;
class CGradientEditorView;
class CGradientPositionCtrl;
class CGradientRulerCtrl;

class CGradientEditorWindow : public CWnd{
	DECLARE_DYNAMIC(CGradientEditorWindow)
public:
	enum{
		STYLE_NO_COLOR           = 1 << 0,
		STYLE_NO_ALPHA           = 1 << 1,
		STYLE_FIXED_POINTS_COUNT = 1 << 2,
		STYLE_CYCLED             = 1 << 3,
	};

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);

	bool doModal();

    CGradientEditorWindow(GradientPositionMode positionMode, int style, const CKeyColor& gradient, CWnd* parent = 0);
    virtual ~CGradientEditorWindow();

	const CKeyColor& getGradient() const;

	static const char* className(){ return "VistaEngineGradientEditorWindow"; }
protected:
	void PreSubclassWindow();

	bool modalResult_;
	int style_;
	PtrHandle<LayoutWindow> layout_;
	PtrHandle<CColorSelector> colorSelector_;
	PtrHandle<CGradientPositionCtrl> positionControl_;
	PtrHandle<CGradientRulerCtrl> rulerControl_;
	PtrHandle<CGradientEditorView> gradientView_;
	PtrHandle<CKeyColor> gradient_;
	CButton okButton_;
	CButton cancelButton_;

	CWnd* parent_;

    DECLARE_MESSAGE_MAP()
private:
	void onColorChanged();
	void onGradientViewChanged();
	void onPositionChanged();

	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	
	afx_msg void OnClose();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif // __GRADIENT_EDITOR_H_INCLUDED__
