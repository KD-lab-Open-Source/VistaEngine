#ifndef __COLOR_RAMP_CTRL__
#define __COLOR_RAMP_CTRL__

#include "Umath.h"
#include "Functor.h"

struct HBSColor{
    HBSColor(float _hue = 0.0f, float _brightness = 1.0f, float _saturation = 0.0f)
    : hue(_hue)
    , brightness(_brightness)
    , saturation(_saturation)
    {}

    HBSColor(const sColor4f& color);

    void set(const sColor4f& color);

    void toRGB(sColor4f& result) const;

    float hue;
    float brightness;
    float saturation;
};

class CColorRampCtrl : public CWnd{
	DECLARE_DYNAMIC(CColorRampCtrl)
public:
	enum {
		HUE_WIDTH = 20,
		BORDER_WIDTH = 4
	};

    CColorRampCtrl();

    BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);
    static const char* className() { return "VistaEngineColorRampCtrl"; }

	Functor0<void>& signalColorChanged() { return signalColorChanged_; };

    void setColor(const sColor4f& color);
    const sColor4f& getColor() const{ return color_; }
	void redraw(CDC& dc);

    afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* wnd, UINT hitTest, UINT message);

    DECLARE_MESSAGE_MAP()
protected:
	void createRampBitmap();
	void handleMouse(CPoint point);
	void handleMouseHue(CPoint point);

	int mouseArea_;
    Functor0<void> signalColorChanged_;
	sColor4f color_;

	HBSColor hlsColor_;
    
    CBitmap rampBitmap_;
};

#endif
