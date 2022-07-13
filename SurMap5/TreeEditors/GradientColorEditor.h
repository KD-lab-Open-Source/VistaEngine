#ifndef __GRADIENT_EDITOR_H_INCLUDED__
#define __GRADIENT_EDITOR_H_INCLUDED__

#define GRADIENT_EDITOR_CLASSNAME "VistaEngineGradientEditor"

#include "IVisGeneric.h"
#include "Rect.h"

class CGradientColorEditor : public CWnd{
public:
    CGradientColorEditor(CWnd* = 0);
    virtual ~CGradientColorEditor();

	static void DrawGradient(CDC* dc, const CRect& rect, const CKeyColor& gradient);

	void nextPeg();
	void previousPeg();
	void setCycled(bool cycled) { cycled_ = cycled; }

	void setGradient(const CKeyColor& gradient);
	void moveSelection(float position);
	void setSelection(int index);
	int getSelection() const;
	void setCurrentColor(const sColor4f& color);
	CKeyColor& getGradient();

	void setFixedPointsCount(bool fixed) { fixedPointsCount_ = fixed; }
protected:
	static void DrawColorBlend (CDC* dc, const Recti& rect, const sColor4f& color1, const sColor4f& color2);
	static void DrawPegs (CDC* dc, const Recti& rect, const CKeyColor& gradient, int selection);

	int GetPegAt(float pos, float precision);
	void OnDraw(CDC*);

	CKeyColor gradient_;
	int selection_;
	bool moving_;
	float lastMousePosition_;
	Vect2i lastMousePoint_;
	bool cycled_;
	bool fixedPointsCount_;

    DECLARE_MESSAGE_MAP()
protected:
	virtual void PreSubclassWindow();
};

#endif // __GRADIENT_EDITOR_H_INCLUDED__
