#ifndef __GRADIENT_POSITION_CTRL_H_INCLUDED__
#define __GRADIENT_POSITION_CTRL_H_INCLUDED__

#include "Functor.h"
#include "Handle.h"

class LayoutWindow;

class CKeyColor;

enum GradientPositionMode{
	GRADIENT_POSITION_FLOAT,
	GRADIENT_POSITION_BYTE,
	GRADIENT_POSITION_TIME
};

class CGradientPositionCtrl : public CWnd{
	DECLARE_DYNAMIC(CGradientPositionCtrl)
public:
	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);


    CGradientPositionCtrl(CKeyColor& gradient, GradientPositionMode positionMode, CWnd* parent = 0);
    virtual ~CGradientPositionCtrl();

	void setGradient(CKeyColor* gradient);
	
	void setPosition(float position);
	float getPosition() const;
	void setSelection(int selection);
	int getSelection() const;

	static const char* className(){ return "VistaEngineGradientPositionCtrl"; }

	Functor0<void>& signalPositionChanged() { return signalPositionChanged_; }
protected:
	void PreSubclassWindow();
	void updateControls(bool updateEdit = true);
	void onEditChange();

	CKeyColor* gradient_;
	int selection_;
	CString position_;
	bool updatingControls_;

	PtrHandle<LayoutWindow> layout_;

	CWnd* parent_;

	CButton nextButton_;
	CButton prevButton_;

	CStatic currentPegLabel_;
	CEdit positionEdit_;

	CStatic pegLabel_;
	CStatic positionLabel_;

	Functor0<void> signalPositionChanged_;

	GradientPositionMode positionMode_;

    DECLARE_MESSAGE_MAP()
private:
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};


class CGradientRulerCtrl : public CWnd{
	DECLARE_DYNAMIC(CGradientRulerCtrl)
public:
	static const char* className(){ return "VistaEngineGradientRulerCtrl"; }

    CGradientRulerCtrl(GradientPositionMode positionMode);
	virtual ~CGradientRulerCtrl();

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);

protected:
    DECLARE_MESSAGE_MAP()
private:
	GradientPositionMode positionMode_;
	CFont* font_;
};

#endif // __GRADIENT_EDITOR_H_INCLUDED__
