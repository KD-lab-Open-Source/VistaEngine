#ifndef __TOOL_PLACE_H_INCLUDED__
#define __TOOL_PLACE_H_INCLUDED__

#include "kdw/Tool.h"

class ToolPlace : public kdw::Tool{
public:
	ToolPlace();
	void render(kdw::ToolViewport* viewport, kdw::SandboxRenderer* renderer);
	bool onMouseButtonDown(kdw::ToolViewport* viewport, kdw::MouseButton button);
	bool onMouseButtonUp(kdw::ToolViewport* viewport, kdw::MouseButton button);
	bool onMouseMove(kdw::ToolViewport* viewport);
protected:
	Vect3f lastPosition_;
	bool tracking_;
	bool pressed_;
};

#endif
