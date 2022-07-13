#ifndef __U_I_SPRITE_EDITOR_H_INCLUDED__
#define __U_I_SPRITE_EDITOR_H_INCLUDED__

#include "IRenderDevice.h"
#include "IVisGeneric.h"
#include "Rect.h"

#include "..\UserInterface\UI_Types.h"
#include "..\..\Render\inc\umath.h"

#define UITEXTUREEDITOR_CLASSNAME "UITextureEditor"
// CUISpriteEditor

class CUISpriteEditor : public CWnd
{
	friend class CUISpriteEditorDlg;
	DECLARE_DYNAMIC(CUISpriteEditor)
public:
	CUISpriteEditor();
	virtual ~CUISpriteEditor();
    void initRenderDevice(int cx, int cy);
    void finitRenderDevice();

	void redraw();
    
    void drawRect(const Rectf& _rect, float _depth, const sColor4c& _color, eBlendMode _blend_mode = ALPHA_BLEND);
	void drawRectTextured(const Rectf& _rect, float _depth, const sColor4c& _color, cTexture* _texture, const Rectf& _uv_rect,  eBlendMode _blend_mode = ALPHA_BLEND, float phase = 0.0f, float saturation = 1.0f);

	void nextFrame();
	void prevFrame();
	void enableAnimation(bool enable);
	
	void setSprite (const UI_Sprite& sprite);
	UI_Sprite getSprite () const;
	void drawEdgesOverlay(const Rectf& rect);

	void clipRectByView(Rectf* rect, Rectf* uv);

	Rectf coordsToWindow (const Rectf& rect);
	Rectf coordsFromWindow (const Rectf& rect);
	
	Vect2f coordsToWindow (const Vect2f& point);
	Vect2f coordsFromWindow (const Vect2f& point);

    Rectf getCoords () const {
		if (!sprite_.isEmpty () && sprite_.texture()) {
			const Rectf& coords = sprite_.textureCoords ();
			return Rectf(round (coords.left ()   * sprite_.texture ()->GetWidth ()),
						 round (coords.top ()    * sprite_.texture ()->GetHeight ()),
						 round (coords.width ()  * sprite_.texture ()->GetWidth ()),
						 round (coords.height () * sprite_.texture ()->GetHeight ()));
		}
        return Rectf ();
    }
	void setCoords (const Rectf& coords) {
		if (!sprite_.isEmpty () && sprite_.texture()) {
			sprite_.setTextureCoords (coords / Vect2f (sprite_.texture ()->GetWidth (), sprite_.texture ()->GetHeight ()));
			redraw ();
		}
    }
	void setTexture (const char* name);
	void scroll (const Vect2f& delta);
    void move (const Vect2f& pos, const Vect2i& delta);

    const Vect2f& viewOffset() const { return view_offset_; }
    const Vect2i& viewSize() const { return view_size_; }

	void setBackgroundColor(sColor4c color) { backgroundColor_ = color; }
	sColor4c backgroundColor() const{ return backgroundColor_; }

	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
protected:
    void updateCameraFrustum(int cx, int cy);

	UI_Sprite sprite_;

    cCamera* camera_;
	cScene* scene_;
	cRenderWindow* renderWindow_;

	int zoom_;
    bool scrolling_;
    bool moving_;
    Recti selected_edges_;
    
	sColor4c backgroundColor_;

    Vect2f view_offset_;
    Vect2i view_size_;

	bool animationEnabled_;
	float animationPhase_;
	float animationTime_;

	void PreSubclassWindow();
	DECLARE_MESSAGE_MAP()
};



#endif
