#ifndef __UI_UNIT_VIEW_H__
#define __UI_UNIT_VIEW_H__

#include "..\Units\Animation.h"

class UI_UnitView
{
public:
	UI_UnitView();
	~UI_UnitView();

	bool init();
	bool release();

	//void preDraw();
	void draw();
	//void postDraw();

	void toggleDraw(bool needDraw) { needDraw_ = needDraw; }

	bool setPosition(const Rectf& pos);
	bool setAttribute(const AttributeBase* attribute);

	bool isEmpty() const { return !attribute_; }

	void logicQuant(float dt);

	static UI_UnitView& instance(){ return Singleton<UI_UnitView>::instance(); }

private:

	bool needDraw_;
	Rectf windowPosition_;
	cScene* scene_;
	cCamera* camera_;

	MatXf modelPosition_;
	cObject3dx* model_;
	
	PhaseController phase_;
	InterpolatorPhase interpolator_phase;
	InterpolatorPose pose_interpolator;

	const AttributeBase* attribute_;
};

#endif /* __UI_UNIT_VIEW_H__ */
