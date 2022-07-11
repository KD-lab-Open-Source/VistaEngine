#pragma once

#include "XMath\Colors.h"
#include "Timers.h"

class PostEffectManager;

class Flash
{
public:
	Flash(PostEffectManager* manager);
	~Flash();
	void init(float _intensity);
	void setActive(bool _active);
	
	void setColor(Color4c _color);
	
	void setIntensity();
	void draw();
	void addIntensity(float _intensity);
	void addFlash() {count_++;}
	void decFlash() {count_--; if(count_<0)count_=0;}

private:
	PostEffectManager* manager_;
	bool active_;
	Color4c color_;
	
	bool inited_;
	float intensity_[2];
	int count_;
	float intensitySum_;
	InterpolationLogicTimer timer_;
};

