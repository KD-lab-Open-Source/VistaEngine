#ifndef __SUR_TOOL_CAMERA_H_INCLUDED__
#define __SUR_TOOL_CAMERA_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"
#include "SurToolEditable.h"
#include "EventListeners.h"

class CameraSpline;
class CameraCoordinate;

class CSurToolCamera : public CSurToolEditable, public ObjectObserver, public sigslot::has_slots
{
	DECLARE_DYNAMIC(CSurToolCamera)
public:
	CSurToolCamera(CWnd* parent = 0);
	virtual ~CSurToolCamera();

	bool onOperationOnMap(int x, int y);

	virtual BOOL OnInitDialog();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
private:
    CameraSpline& cameraSpline_;
};

#endif
