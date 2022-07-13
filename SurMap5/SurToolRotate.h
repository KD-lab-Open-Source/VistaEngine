#ifndef __SUR_TOOL_ROTATE_H_INCLUDED__
#define __SUR_TOOL_ROTATE_H_INCLUDED__

#include "SurToolAux.h"
#include "SurToolTransform.h"

class CSurToolRotate : public CSurToolTransform {
public:
    CSurToolRotate(CWnd* parent = NULL);
    bool onTrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool onLMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord);
    bool onDrawAuxData();

protected:
	void onTransformAxisChanged(int index);
    void beginTransformation();

private:
	bool stepRotationMode();
	float angleStep_;
};

#endif
