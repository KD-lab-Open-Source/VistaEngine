#pragma once

#include "kdw/Viewport2D.h"
#include "TriggerEditor\TriggerExport.h"

class TriggerView;

class TriggerMiniMap : public kdw::Viewport2D
{
public:
	TriggerMiniMap(TriggerChain& triggerChain);
	void setTriggerView(TriggerView* triggerView) { triggerView_ = triggerView; }

	void setDebug(bool debug) { debug_ = debug; }

	void onMouseMove(const Vect2i& delta);
	void onMouseButtonDown(kdw::MouseButton button);

	void onRedraw(HDC dc);

private:
	TriggerChain& triggerChain_;
	TriggerView* triggerView_;
	bool debug_;

	void drawTrigger(HDC dc, Trigger& trigger);
};

