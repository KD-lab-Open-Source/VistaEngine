#ifndef __EDITOR_VIEW_H_INCLUDED__
#define __EDITOR_VIEW_H_INCLUDED__

#include "Handle.h"
#include "Render\3dx\Umath.h"
#include "SubRectEditor.h"
#include "SelectionCorner.h"
#include "MaskEditor.h"

#include "Render\inc\IRenderDevice.h"


class cRenderWindow;
class Camera;
class cScene;
class cTexture;
struct IDirect3DSurface9;

class UI_Screen;

class ViewDefaultResources : public ManagedResource {
public:
	ViewDefaultResources(Camera* camera);
	~ViewDefaultResources();

	void initialize();
	void free();

	void deleteManagedResource();
	void restoreManagedResource();

	cTexture* screenTexture() { return screenTexture_; }
	IDirect3DSurface9* screenDepth() { return screenDepth_; }
	bool reseted() const{ return reseted_; }
protected:
	bool reseted_;
	cTexture* screenTexture_;
	IDirect3DSurface9* screenDepth_;
	Camera* camera_;
};

class UI_ControlContainer;

class CUIEditorView : public CWnd
{
	DECLARE_DYNCREATE(CUIEditorView)
public:
	CUIEditorView();
	virtual ~CUIEditorView();

	void redraw();
	void quant(float dt);

	void onSelectionChanged();
	void updateProperties();
	void updateHoverControl();
    
	///рисует дополнительные ручки, рамки, уголки, поверх стандартного интерфаейса
	void drawEditorOverlay();
	void drawEditorUnderlay(const UI_ControlContainer* container = 0);
	float pixelWidth() const;
	float pixelHeight() const;

	///установить текущий(редактируемый) экран
	void setCurrentScreen(UI_Screen* screen);
	///текущий(редактируемый) экран
    UI_Screen* currentScreen(){ return current_screen_; }

	bool isEditingControls() const;
	///редактируем ли сейчас SubRect-ы
	bool isEditingSubRect() const;
	///редактируем ли сейчас маску прозрачности
	bool isEditingMask() const;
    
    void eraseSelection();

	void updateWindowPosition();

	void setViewZoom(float zoom);
	void zoom(bool in = true);
	float viewZoom() const;

	void setViewCenter(const Vect2f& viewCenter);
	const Vect2f& viewCenter() const;


	Vect2f transform(const Vect2f& coord);
	Vect2f untransform(const Vect2f& coord);
	Rectf transform(const Rectf& coord);
	Rectf untransform(const Rectf& coord);
	//Rectf untransform(const Rectf& coord);

	Vect2f relativeCoords(const Vect2f& screenCoords);
	Vect2f screenCoords(const Vect2f& relativeCoords);
	Rectf screenCoords(const Rectf& relativeCoords);

	void drawBlackBars();

	void drawRect(const Rectf& _rect, float _depth, const Color4f& _color, eBlendMode _blend_mode = ALPHA_BLEND, bool outline = false);
	void drawRectFrame(const Rectf& _rect, float _depth, const Color4f& _color, const Vect2f& weight, eBlendMode _blend_mode = ALPHA_BLEND);
	void drawLine(const Vect2f& start, const Vect2f& end, const Color4f& color);
	void drawHandle(const Vect2f& pos, const Vect2f& corner_size, bool active);

	Color4c backgroundColor() const{ return backgroundColor_; }

    static const float      SCREEN_WIDTH;
    static const float      SCREEN_HEIGHT;
    static const float      ASPECT;
    static const Vect2f     CORNER_SIZE;
    static const float      CORNER_DISTANCE;
    static const Color4f   INACTIVE_CORNER_COLOR;
    static const Color4f   ACTIVE_CORNER_COLOR;

	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDestroy();
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

protected:
	void afterRedraw();

	bool initRenderDevice(int width, int height);
	void finitRenderDevice();
    void updateCameraFrustum(int width, int height);


	SubRectEditor			sub_rect_editor_;
	MaskEditor			    mask_editor_;
	UI_Screen*              current_screen_;
	Color4c				backgroundColor_;

	bool					renderDeviceInitialized_;

	float					viewZoom_;
	Vect2f					viewCenter_;

	PtrHandle<ViewDefaultResources> defaultResources_;

	cRenderWindow*			render_window_;
    Camera*                camera_;
    cScene*                 scene_;
	Vect2f					selection_start_;
	bool					selecting_rect_;

	int						view_width_;
	int						view_height_;
    Vect2f                  cursor_position_;
	int						selected_guide_;

	Vect2i					scrollStart_;
	Vect2f					scrollStartRelative_;
protected:
	DECLARE_MESSAGE_MAP()
};

#endif
