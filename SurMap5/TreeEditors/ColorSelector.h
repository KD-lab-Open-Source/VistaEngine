#ifndef __COLOR_SELECTOR_H_INCLUDED__
#define __COLOR_SELECTOR_H_INCLUDED__

#include "Handle.h"
#include "Functor.h"

class LayoutWindow;
class CSolidColorCtrl;

class CColorSelector : public CWnd{
public:
    CColorSelector(bool noAlpha = false, bool allowPopup = true, CWnd* parent = 0);
    virtual ~CColorSelector ();

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id);

	static const char* className(){ return "VistaEngineColorSelector"; }
	const sColor4f& getColor() const;
	void setColor(const sColor4f& color);
	
	Functor0<void>& signalColorChanged() { return signalColorChanged_; }
protected:
	void PreSubclassWindow();

	PtrHandle<CSolidColorCtrl> colorControl_;
	PtrHandle<LayoutWindow> layout_;

	bool noAlpha_;

	CStatic redLabel_;
	CStatic greenLabel_;
	CStatic blueLabel_;
	CStatic alphaLabel_;

	CSliderCtrl redSlider_;
	CSliderCtrl greenSlider_;
	CSliderCtrl blueSlider_;
	CSliderCtrl alphaSlider_;
	
	CEdit redEdit_;
	CEdit greenEdit_;
	CEdit blueEdit_;
	CEdit alphaEdit_;

	BYTE red_;
	BYTE green_;
	BYTE blue_;
	BYTE alpha_;
	mutable sColor4f color_;

	void onColorChanged();
	Functor0<void> signalColorChanged_;

    DECLARE_MESSAGE_MAP()
private:
	void updateEdits();
	void updateSliders();
	void updateColor();

	afx_msg BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void onEditChange(CEdit* edit);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnHScroll(UINT sbCode, UINT pos, CScrollBar* scrollBar);  
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif // __GRADIENT_EDITOR_H_INCLUDED__
