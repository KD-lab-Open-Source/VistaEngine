#ifndef __GENERALVIEW_H__
#define __GENERALVIEW_H__

#include "IRenderDevice.h"
#include "..\Game\RenderObjects.h"
#include "Handle.h"
class MissionDescription;

// CGeneralView window
class CToolsTreeWindow;
class CSurToolBase;

//Создание полей в статус баре
enum { NUMBERS_PARTS_STATUSBAR = 8 + 2 };//8-полей 2-промежутка

void drawLineTerrain(const Vect2f& point1, const Vect2f& point2, const sColor4c& color, float segmentLength = 16.0);

class CGeneralView : public CWnd
{
public:
	CGeneralView();
	void init(CToolsTreeWindow* toolsWindow){
		toolsWindow_ = toolsWindow;
	};

	CToolsTreeWindow* toolsWindow_;

	CSurToolBase* getCurCtrl(void);

	void initRenderDevice();
	void doneRenderDevice();

	string lastWorldName;
	void setGraphicsParameters();
	bool isWorldOpen() const;

	void quant();

	Vect3f pointOnMouse;
	void UpdateStatusBar();

	// ID таймера повтора инструмента
	UINT m_Timer4RepetitionID;

	class cFont* pFont;

	double prevtime;
    Vect3f	vPosition;
	Vect2f	AnglePosition;
	bool wireframe;
	bool flag_restrictionOnPaint;

	virtual ~CGeneralView();

	void drawGrid();

	void InitAux();
	void graphQuant();
	void Animate(float dt);
	void CameraQuant(float dt);

	void createScene();
	void doneScene();
	void reInitWorld();
	void doneUniverse();
	void updateSurface();
	
	bool isSceneInitialized () const { return sceneInited_; }
	bool CoordScr2vMap(const Vect2i& inMouse_pos, Vect3f& outWorld_pos);
	MissionDescription& currentMission() { return *currentMission_; }

	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL DestroyWindow();
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg void OnPaint();
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
private:
	PtrHandle<MissionDescription> currentMission_;
	cRenderWindow* renderWindow_;
	bool renderDeviceInited_;
	bool sceneInited_;
	int draw_num_polygon;
	int draw_num_tilemappolygon;
};

#endif //__GENERALVIEW_H__
