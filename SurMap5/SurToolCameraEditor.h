#ifndef __SUR_TOOL_CAMERA_EDITOR_H_INCLUDED__
#define __SUR_TOOL_CAMERA_EDITOR_H_INCLUDED__

#include "SurToolAux.h"
#include "MFC\SizeLayoutManager.h"

class CameraSpline;
class BaseUniverseObject;

class CSurToolCameraEditor : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolCameraEditor)
public:
	CSurToolCameraEditor(BaseUniverseObject* spline, bool playAndQuit = false);
	virtual ~CSurToolCameraEditor();

	bool onTrackingMouse (const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool onLMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool onLMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool onRMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	void onSelectionChanged ();

	bool onDelete();
    bool onKeyDown(unsigned int keyCode, bool shift, bool control, bool alt);

	bool onDrawAuxData();
	void quant();

	int getIDD() const{ return IDD_BARDLG_CAMERA; }
protected:
	void updateControls();
	void addPoint(const Vect3f& worldCoord);
	int nodeUnderPoint(const Vect2f& point);

	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();

	afx_msg void OnSize(UINT nType, int cx, int cy);

	afx_msg void OnPlayCameraButton();
	afx_msg void OnViewToPointButton();
	afx_msg void OnPointToViewButton();
	afx_msg void OnMoveOriginToViewButton();

	afx_msg void OnAddButton();
	afx_msg void OnDeleteButton();

	afx_msg void OnPrevButton();
	afx_msg void OnNextButton();

	afx_msg void OnDoneButton();

	DECLARE_MESSAGE_MAP()
private:
    CSizeLayoutManager layout_;
	CameraSpline* spline_;

	int selectedPoint_;
	bool playAndQuit_;
	bool moving_;
};

#endif
