#ifndef __MINI_MAP_WINDOW_H_INCLUDED__
#define __MINI_MAP_WINDOW_H_INCLUDED__

#include "Render\Inc\IRenderDevice.h"
#include "EventListeners.h"

class CSurToolBase;
class CMainFrame;
class cScene;

class CMiniMapWindow : public CWnd, public WorldObserver, public sigslot::has_slots{
public:
    static const char* className() { return "VistaEngineMiniMap"; }
	BOOL Create (DWORD dwStyle, const CRect& rect, CWnd* pParentWnd);
	CMiniMapWindow(CMainFrame* mainFrame);
	virtual ~CMiniMapWindow();

	cScene* getScene(){
		return scene_;
	}

	void onWorldChanged(WorldObserver* changer);
	
	void updateCameraFrustum (int width, int height);
	void updateMinimapPosition (int width, int height);

	void redraw(CDC& dc);

	void setZoom(float zoom);
	float zoom() const{ return zoom_; }

	void initRenderDevice ();
	void doneRenderDevice ();

	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
protected:
	CSurToolBase* currentToolzer();

	Vect2i windowSize_;
	cRenderWindow* renderWindow_;
    Camera* camera_;
	cScene* scene_;
	bool sizing_;
	float zoom_;

	virtual void PreSubclassWindow();

	DECLARE_MESSAGE_MAP()
private:
    void drawMiniMap ();
	
	int flag_CameraRestrictionDrag_;
};

#endif
