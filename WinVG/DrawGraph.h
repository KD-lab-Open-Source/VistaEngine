#pragma once

class DrawFunctor
{
public:
	virtual float get(float f,sColor4c* color)=0;
};

class DrawGraph
{
public:
	DrawGraph();
	void SetWindow(float xmin=0,float xmax=1,float ymin=0, float ymax=1);
	void SetArgumentRange(float xmin,float xmax,float ymin,float ymax);
	void SetCycleHalfShift(bool b=true){half_shift=b;};
	void Draw(DrawFunctor& f);
	void DrawXPosition(float x,sColor4c color);
protected:
	float wxmin,wxmax,wymin,wymax;
	float xmin,xmax,ymin,ymax;
	bool half_shift;
};
