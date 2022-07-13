#ifndef __GRADIENT_EDITOR_H_INCLUDED__
#define __GRADIENT_EDITOR_H_INCLUDED__

#include "..\..\render\inc\Umath.h"
#include "..\..\render\src\NParticleKey.h"
#include "Rect.h"

#include "Functor.h"

class CGradientEditorView : public CWnd{
public:
    CGradientEditorView(CWnd* = 0);
    virtual ~CGradientEditorView ();

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);

	static const char* className() { return "VistaEngineGradientEditor"; }
	static void DrawGradient(CDC* dc, const CRect& rect, const CKeyColor& gradient);

	void nextPeg();
	void previousPeg();
	void setCycled(bool cycled) { cycled_ = cycled; }

	void setGradient(const CKeyColor& gradient);
	void setSelection(int index);
	void moveSelection(float position);
	int getSelection() const;
	void setCurrentColor(const sColor4f& color);
	CKeyColor& getGradient();

	void setFixedPointsCount(bool fixed) { fixedPointsCount_ = fixed; }

	Functor0<void>& signalPositionChanged() { return signalPositionChanged_; }
	Functor0<void>& signalColorChanged() { return signalColorChanged_; }

	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT flags, short delta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
protected:
	static void DrawColorBlend (CDC* dc, const Recti& rect, const sColor4f& color1, const sColor4f& color2);
	static void DrawPegs (CDC* dc, const Recti& rect, const CKeyColor& gradient, int selection);

	int GetPegAt (float pos, float precision);
	void OnDraw (CDC*);

	CKeyColor gradient_;
	int selection_;
	bool moving_;
	float lastMousePosition_;
	Vect2i lastMousePoint_;
	bool cycled_;
	bool fixedPointsCount_;

	Functor0<void> signalPositionChanged_;
	Functor0<void> signalColorChanged_;

	virtual void PreSubclassWindow();
    DECLARE_MESSAGE_MAP()
};

#endif // __GRADIENT_EDITOR_H_INCLUDED__
