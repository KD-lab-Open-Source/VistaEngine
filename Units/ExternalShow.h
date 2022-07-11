#ifndef __EXTERNAL_SHOW_H__
#define __EXTERNAL_SHOW_H__

class cBaseGraphObject;
#include "CircleManagerParam.h"

void terCircleShowGraph(const Vect3f& pos,float r, const struct CircleColor& circleColor);
void terCircleShow(const Vect3f& pos0,const Vect3f& pos1,float r, const struct CircleColor& circleColor);
void terCircleShow(const Vect3f& pos0,const Vect3f& pos1,float r0, float r1, const struct CircleColor& circleColor);

class cCircleShow
{
public:
	cCircleShow();
	~cCircleShow();

	void Circle(const Vect3f& pos0,const Vect3f& pos1,float r0,float r1, const CircleColor& attr);
	void Circle(const Vect3f& pos,float r,const CircleColor& attr);
	void Show(int dotted);
	void Quant(float dt);

	void Lock();
	void Unlock();
	void Clear();

	//Функция terCircleShow вызывается без интерполяции и в графическом потоке
	void SetNoInterpolationLock(){Lock();no_interpolation=true;};
	void SetNoInterpolationUnlock(){no_interpolation=false;Unlock();};
protected:
	struct sCircle
	{
		Vect3f pos[2];
		float r[2];
		CircleColor attr;
	};
	struct sCircleGraph
	{
		Vect3f pos;
		float r;
		CircleColor attr;
	};

	struct sCircleType
	{
		vector<sCircle> circles;
		vector<sCircleGraph> circles_graph;
		cBaseGraphObject* external_show;
	};

	vector<sCircleType> types;
	float u_begin;

	float energyTextureStart_;
	float energyTextureEnd_;

	void CircleShow(const Vect3f& pos,float r, const CircleColor& circleColor);

	bool no_interpolation;
	MTSection lock;
};

extern cCircleShow* gbCircleShow;


#endif //__EXTERNAL_SHOW_H__
