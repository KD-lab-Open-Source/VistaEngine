#ifndef __SUR_TOOL_CAMERA_RESTRICTION_H_INCLUDED__
#define __SUR_TOOL_CAMERA_RESTRICTION_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"
#include "SurToolEditable.h"
#include "Rect.h"
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

	bool CallBack_OperationOnMap(int x, int y);


	void onLMBDown(const Vect2f& worldPosition);
	void onLMBUp(const Vect2f& worldPosition);
	void onMouseMove(const Vect2f& worldPosition);

	bool CallBack_LMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool CallBack_LMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool CallBack_DrawAuxData();

	bool CallBack_PreviewLMBDown(const Vect2f& point);
	bool CallBack_PreviewLMBUp(const Vect2f& point);
	bool CallBack_PreviewTrackingMouse(const Vect2f& point);
	bool CallBack_DrawPreview(int width, int height);

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
