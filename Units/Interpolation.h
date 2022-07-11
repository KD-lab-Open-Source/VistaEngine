#ifndef __INTERPOLATION_
#define __INTERPOLATION_

#include "SafeMath.h"
#include "..\ht\StreamInterpolation.h"

template<class T ,class InterpolationOp = DefaultInterpolationOp<T> >
class Interpolator
{
protected:
	T x_[2];
	bool inited_;
	bool update_;
				 
public:
	Interpolator() { initialize(); }

	void initialize() { inited_ = false; update_ = false;}
	
	void operator=(const T& x) 
	{ 
		if(inited_){ 
			x_[0] = x_[1]; 
			x_[1] = x; 
		} 
		else{ 
			inited_ = true; 
			x_[0] = x_[1] = x; 
		} 
		update_ = true;
	}
	
	const T& prevValue() const { return x_[0]; } //Устаревшее роложение
	const T& currValue() const { return x_[1]; } //Положение на текущий логический квант

	void operator()(cBaseGraphObject* cur)
	{ 
		if(update_)
			InterpolationOp()(cur,x_);
		update_ = false;
	}
};


class Se3fInterpolationOp
{
public:
	void operator()(cBaseGraphObject* cur,Se3f p[2])
	{
		streamLogicInterpolator.set(fSe3fInterpolation,cur) << p[0] << p[1];
	}
};

class NodeTransformInterpolationOp
{
public:
	void operator()(cBaseGraphObject* cur, Se3f p[2])
	{
		xassert(0);
	}
};

class PhaseInterpolationOp
{
public:
	void operator()(cBaseGraphObject* cur,float p[2])
	{
		xassert(0);
	}
};

class PhaseFadeInterpolationOp
{
public:
	void operator()(cBaseGraphObject* cur,Vect2f p[2])
	{
		xassert(0);
	}
};

class AngleInterpolationOp
{
public:
	void operator()(cBaseGraphObject* cur,float p[2])
	{
		xassert(0);
	}
};

class ColorInterpolationOp
{
public:
	void operator()(cBaseGraphObject* cur,sColorInterpolate p[2])
	{
		streamLogicInterpolator.set(fColorInterpolation,cur)<<p[0] << p[1];
	}
};

class ParticleRateInterpolationOp
{
public:
	void operator()(cBaseGraphObject* cur,float p[2])
	{
		streamLogicInterpolator.set(fParticleRateInterpolation,cur) << p[0] << p[1];
	}
};

typedef Interpolator<Se3f, Se3fInterpolationOp> InterpolatorPose;
typedef Interpolator<sColorInterpolate, ColorInterpolationOp> InterpolatorColor;
typedef Interpolator<float, ParticleRateInterpolationOp> InterpolatorParticleRate;

class InterpolatorNodeTransform : public Interpolator<Se3f, NodeTransformInterpolationOp>
{
public:
	void operator=(const Se3f& x) 
	{
		(*(Interpolator<Se3f, NodeTransformInterpolationOp>*)this)=x;
	}

	void operator()(cBaseGraphObject* cur, int node_index)
	{ 
		if(update_)
		{
			streamLogicInterpolator.set(fNodeTransformInterpolation, cur) << node_index << x_[0] << x_[1];
		}

		update_=false;
	}
};

class InterpolatorAngle:public Interpolator<float, AngleInterpolationOp>
{
public:
	void operator=(const float& x) 
	{
		(*(Interpolator<float, AngleInterpolationOp>*)this)=x;
	}

	void operator()(cBaseGraphObject* cur,eAxis axis)
	{ 
		if(update_)
		{
			streamLogicInterpolator.set(fAngleInterpolation,cur) << x_[0] << x_[1] << axis;
		}
		update_=false;
	}
};

class InterpolatorPhase:public Interpolator<float, PhaseInterpolationOp>
{
public:
	void operator=(const float& x) 
	{
		(*(Interpolator<float, PhaseInterpolationOp>*)this)=x;
	}

	void operator()(cBaseGraphObject* cur,int animationGroup)
	{ 
		if(update_)
		{
			streamLogicInterpolator.set(fPhaseInterpolation,cur) << x_[0] << x_[1] << animationGroup;
		}
		update_=false;
	}
};

class InterpolatorPhaseFade:public Interpolator<Vect2f, PhaseFadeInterpolationOp>
{
public:
	void operator=(const Vect2f& x) 
	{
		(*(Interpolator<Vect2f, PhaseInterpolationOp>*)this)=x;
	}

	void operator()(cBaseGraphObject* cur,int animationGroup)
	{ 
		if(update_)
		{
			streamLogicInterpolator.set(fPhaseInterpolationFade,cur) << x_[0] << x_[1] << animationGroup;
		}
		update_=false;
	}
};

#endif
