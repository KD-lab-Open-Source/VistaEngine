#ifndef __SHOW_CIRCLE_CONTROLLER_H_INCLUDED__
#define __SHOW_CIRCLE_CONTROLLER_H_INCLUDED__

#include "CircleManagerParam.h"

class ShowCircleController
{
	const CircleEffect* circle_;
	cUnkLight* light_;
	bool updated_;

public:
	ShowCircleController(const CircleEffect& circle);

	void release();

	bool isOld() const { return !updated_; }
	void makeOld() { updated_ = false; }
	
	void redraw(const Vect3f& pos, float radius);
};

#endif //#ifndef __SHOW_CIRCLE_CONTROLLER_H_INCLUDED__
