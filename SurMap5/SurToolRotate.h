#ifndef __SUR_TOOL_ROTATE_H_INCLUDED__
#define __SUR_TOOL_ROTATE_H_INCLUDED__

#include "SurToolAux.h"
#include "SurToolTransform.h"

class CSurToolRotate : public CSurToolTransform {
public:
    CSurToolRotate(CWnd* parent = NULL);
    bool CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool CallBack_LMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord);
    bool CallBack_DrawAuxData();

protected:
	void onTransformAxisChanged(int index);
    void beginTransformation();

private:
	bool stepRotationMode();
	float angleStep_;
};

#endif
