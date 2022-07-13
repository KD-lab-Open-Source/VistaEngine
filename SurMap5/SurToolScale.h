#ifndef __SUR_TOOL_SCALE_H_INCLUDED__
#define __SUR_TOOL_SCALE_H_INCLUDED__

#include "SurToolAux.h"
#include "SurToolTransform.h"

class CSurToolScale : public CSurToolTransform {
public:
    CSurToolScale(CWnd* parent = NULL);
    bool onTrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord);
    bool onDrawAuxData();

protected:
	void onTransformAxisChanged(int index);
    void beginTransformation();
};

#endif
