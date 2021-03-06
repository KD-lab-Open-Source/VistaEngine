#ifndef __SUR_TOOL_CAMERA_RESTRICTION_H_INCLUDED__
#define __SUR_TOOL_CAMERA_RESTRICTION_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"
#include "SurToolEditable.h"
#include "XTL\Rect.h"
#include "mfc\LayoutMFC.h"

class CameraSpline;
class CameraCoordinate;
class UI_Minimap;

class CSurToolCameraRestriction : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolCameraRestriction)
public:
	CSurToolCameraRestriction(CWnd* parent = 0);
	virtual ~CSurToolCameraRestriction();

	bool onOperationOnMap(int x, int y);


	void onLMBDown(const Vect2f& worldPosition);
	void onLMBUp(const Vect2f& worldPosition);
	void onMouseMove(const Vect2f& worldPosition);

	bool onLMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool onLMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool onTrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool onDrawAuxData();

	bool onPreviewLMBDown(const Vect2f& point);
	bool onPreviewLMBUp(const Vect2f& point);
	bool onPreviewTrackingMouse(const Vect2f& point);
	bool onDrawPreview(int width, int height);


	Recti cameraBorder();
	void setCameraBorder(const Recti& cameraBorder);

	Vect2f buttonStep() const;

	enum{
		IDD = IDD_BARDLG_CAMERA_RESTRICTION
	};

	int getIDD() const { return IDD; }

	afx_msg void OnTopLessButton();
	afx_msg void OnTopMoreButton();

	afx_msg void OnBottomLessButton();
	afx_msg void OnBottomMoreButton();

	afx_msg void OnLeftLessButton();
	afx_msg void OnLeftMoreButton();

	afx_msg void OnRightLessButton();
	afx_msg void OnRightMoreButton();

	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL OnInitDialog();
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
protected:
	void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
private:
	//CSizeLayoutManager layout_;

	LayoutWindow layout_;

	void move(const Vect2f& delta);

	CSliderCtrl zoomSlider_;
	bool ownCameraRestriction_;
	Recti selectedEdges_;
	Vect2f startPoint_;
	Vect2f lastPoint_;
};

#endif
