#ifndef __SUR_TOOL_MOVE_H_INCLUDED__
#define __SUR_TOOL_MOVE_H_INCLUDED__

#include "SurToolAux.h"
#include "SurToolTransform.h"

class CSurToolMove : public CSurToolTransform {
public:
    CSurToolMove(CWnd* parent = NULL);
    bool CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool CallBack_LMBDown(const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool CallBack_LMBUp(const Vect3f& coord, const Vect2i& screenCoord);
    bool CallBack_DrawAuxData();

protected:
	void onTransformAxisChanged(int index);
    void beginTransformation();

	bool cloneOnMove_;
};

#endif
