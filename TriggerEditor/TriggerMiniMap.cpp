#include "stdafx.h"
#include "TriggerMiniMap.h"
#include "TriggerView.h"

TriggerMiniMap::TriggerMiniMap(TriggerChain& triggerChain)
: kdw::Viewport2D(0, 12)
, triggerChain_(triggerChain)
, triggerView_(0)
{
	debug_ = false;
}

void TriggerMiniMap::onRedraw(HDC dc)
{
	xassert(triggerView_);

	__super::onRedraw(dc);

	Rectf rt = visibleArea();
	fillRectangle(dc, rt, Color4c::WHITE);

	if(triggerChain_.triggers.empty())
		return;

	Rectf area(triggerChain_.triggers.front().leftTop(), Trigger::gridSize());
	Vect2f border(10, 10);

	TriggerList::iterator ti;
	FOR_EACH(triggerChain_.triggers, ti)
		area.addBound(Rectf(ti->leftTop() - border, Trigger::gridSize() + border*2));

	setVisibleArea(area);

	FOR_EACH(triggerChain_.triggers, ti)
		drawTrigger(dc, *ti);

	drawRectangle(dc, triggerView_->visibleArea(), Color4c::BLACK);
}	

void TriggerMiniMap::drawTrigger(HDC dc, Trigger& trigger) 
{
	Color4c color = debug_ ? Trigger::debugColors[trigger.state()] : trigger.color();
	if(color == Color4c::WHITE)
		drawRectangle(dc, Rectf(trigger.leftTop(), Trigger::gridSize()), Color4c::BLACK);
	else
		fillRectangle(dc, Rectf(trigger.leftTop(), Trigger::gridSize()), color);
	if(trigger.selected())
		drawRectangle(dc, Rectf(trigger.leftTop(), Trigger::gridSize()), Color4c::RED);
}

void TriggerMiniMap::onMouseButtonDown(kdw::MouseButton button)
{
	__super::onMouseButtonDown(button);

	if(button == kdw::MOUSE_BUTTON_LEFT || button == kdw::MOUSE_BUTTON_RIGHT){
		triggerView_->centerOn(clickPoint());
		triggerView_->redraw();
	}

	if(button == kdw::MOUSE_BUTTON_WHEEL_DOWN || button == kdw::MOUSE_BUTTON_WHEEL_UP)
		triggerView_->onMouseButtonDown(button);
}

void TriggerMiniMap::onMouseMove(const Vect2i& delta)
{
	if(lmbPressed() || rmbPressed()){
		triggerView_->centerOn(clickPoint());
		triggerView_->redraw();
		redraw();
	}
}


