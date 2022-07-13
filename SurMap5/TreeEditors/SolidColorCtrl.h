#ifndef __SOLID_COLOR_CTRL_H_INCLUDED__
#define __SOLID_COLOR_CTRL_H_INCLUDED__

#include "Umath.h"
#include "Rect.h"
#include "ColorUtils.h"

#include "Functor.h"

class PopupMenu;

class CSolidColorCtrl : public CWnd{
public:
    CSolidColorCtrl(bool allowPopup = true, CWnd* = 0);
    virtual ~CSolidColorCtrl();

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);
	static const char* className() { return "VistaEngineSolidColorCtrl"; }

    void setColor(const sColor4f& color);
    const sColor4f& getColor() const;

	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	void onMenuCopy();
	void onMenuPaste();

	Functor0<void>& signalColorChanged() { return signalColorChanged_; }
protected:
	bool allowPopup_;
    sColor4f color_;
	PtrHandle<PopupMenu> popupMenu_;

	Functor0<void> signalColorChanged_;

	void OnDraw (CDC*);
	virtual void PreSubclassWindow();

    DECLARE_MESSAGE_MAP()
};

#endif // __SOLID_COLOR_CTRL_H_INCLUDED__
