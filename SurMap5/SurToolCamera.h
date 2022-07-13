#ifndef __SUR_TOOL_CAMERA_H_INCLUDED__
#define __SUR_TOOL_CAMERA_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"
#include "SurToolEditable.h"

class CameraSpline;
class CameraCoordinate;

class CSurToolCamera : public CSurToolEditable
{
	DECLARE_DYNAMIC(CSurToolCamera)
public:
	CSurToolCamera(CWnd* parent = 0);
	virtual ~CSurToolCamera();

	bool CallBack_OperationOnMap(int x, int y);

	virtual BOOL OnInitDialog();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
private:
    CameraSpline& cameraSpline_;
};

#endif
