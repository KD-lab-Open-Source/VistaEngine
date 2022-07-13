#ifndef __SUR_TOOL_MOVE_H_INCLUDED__
#define __SUR_TOOL_MOVE_H_INCLUDED__

#include "SurToolAux.h"
#include "SurToolTransform.h"

class CSurToolMove : public CSurToolTransform {
public:
    CSurToolMove(CWnd* parent = NULL);
    bool onTrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool onLMBDown(const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool onLMBUp(const Vect3f& coord, const Vect2i& screenCoord);
    bool onDrawAuxData();
	bool onRMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord) { return false; }

protected:
	void onTransformAxisChanged(int index);
    void beginTransformation();

	bool cloneOnMove_;
};

#endif
