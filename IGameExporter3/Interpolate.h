#pragma once

#include "Render\3dx\Interpolator3dx.h"
#include "InvertMatrix.h"

template<int _size>
struct VectTemplate
{
	enum 
	{
		size = _size,
	};

	float data[size];

	float& operator[](int i){return data[i];}
	const float& operator[](int i)const{return data[i];}

	float distance(const VectTemplate& p) const
	{
		float d=0;
		for(int i=0;i<size;i++)
		{
			float x=data[i]-p[i];
			d+=x*x;
		}
		return sqrtf(d);
	}
};

template<class T>
struct DefaultCycleOp
{
	static void cycle(const T& example, T& target) {}
};

template<class T, class CycleOp = DefaultCycleOp<T> >
class Refiner
{
	typedef vector<T> Positions;
public:
	struct Data
	{
		ITPL itpl;
		int interval_begin;//потом во float будет переводиться.
		int interval_size;
		T a0,a1,a2,a3;//y=a0+a1*t+a2*t*t+a3*t*t*t;

		Data() {}

		Data(ITPL _itpl, int begin, int size)
		{
			itpl = _itpl;
			interval_begin = begin;
			interval_size = size;
			for(int i = 0; i < T::size; i++)
				a0[i] = a1[i] = a2[i] = a3[i] = 0;
		}

		Data(ITPL _itpl, int begin, int size, const Positions& positions)
		{
			itpl = _itpl;
			interval_begin = begin;
			interval_size = size;
			for(int i = 0; i < T::size; i++){
				a0[i] = a1[i] = a2[i] = a3[i] = 0;

				if(itpl == ITPL_CONSTANT){
					a0[i] = positions[begin][i];
					continue;
				}
				
				int dataSize = size + 1;
				vector<float> data(dataSize, 0);
				for(int j = 0; j < dataSize; j++)
					data[j] = positions[begin + j][i];

				if(itpl == ITPL_LINEAR)
					calcLinear(data, a0[i], a1[i]);
				else if(itpl == ITPL_SPLINE){
					if(dataSize > 3)
						calcCubic(data, a0[i], a1[i], a2[i], a3[i]);
					else
						calcQuadrix(data, a0[i], a1[i], a2[i]);
				}
				else 
					xassert(0);
			}
		}

		T calc(float t) const 
		{
			float t2=t*t;
			float t3=t2*t;
			T out;
			for(int i = 0; i < T::size; i++)
				out[i]=a0[i]+a1[i]*t+a2[i]*t2+a3[i]*t3;
			return out;
		}

		float delta(const Positions& positions) const
		{
			float delta = 0;
			for(int i = 0; i < interval_size; i++){
				if(itpl != ITPL_CONSTANT){
					float t = i/(float)(interval_size) - 0.5f; //  - 1
					delta = max(calc(t).distance(positions[i + interval_begin]), delta);
				}
				else
					delta = max(a0.distance(positions[i + interval_begin]), delta);
			}
			return delta;
		}

		void serialize(Archive& ar)
		{
			ar.serialize((int&)itpl, "itpl", "itpl");
			ar.serialize(interval_begin, "interval_begin", "interval_begin");
			ar.serialize(interval_size, "interval_size", "interval_size");
			ar.serialize(delta_, "delta_", "delta_");
		}
	};

	vector<Data> out_data;
	int interval_size;

	Refiner();

	void addValue(T value){ 
		if(!positions.empty())
			CycleOp::cycle(positions.back(), value);
		positions.push_back(value); 
	}

	void refine(float delta, bool cycled_);

	template<class InterpolatorData>
	void export(Interpolator3dx<InterpolatorData>& interpolator) const;

	bool constant() const { return out_data.size() == 1 && out_data[0].itpl == ITPL_CONSTANT; }

protected:
	Positions positions;
};

template<class T, class CycleOp>
Refiner<T, CycleOp>::Refiner()
{
	interval_size=0;
}

template<class T, class CycleOp>
void Refiner<T, CycleOp>::refine(float delta, bool cycled)
{
	if(positions.empty()){
		xassert(!positions.empty());
		return;
	}
	
	interval_size = positions.size();

	Data constant(ITPL_CONSTANT, 0, interval_size, positions);
	if(constant.delta(positions) < delta)
		out_data.push_back(constant);
	else{
		for(int begin = 0; begin < interval_size - 1;){
			ITPL itpl = ITPL_LINEAR;
			int size = 1;
			if(begin < interval_size - 3){
				size = 3;
				itpl = ITPL_SPLINE;
			}
			else if(begin == interval_size - 3){
				size = 2;
				itpl = ITPL_SPLINE;
			}
			while(1){
				if(begin + size == interval_size - 1)
					break;
				if(Data(itpl, begin, size + 1, positions).delta(positions) < delta)
					size++;
				else if(itpl < ITPL_SPLINE && Data(ITPL(itpl + 1), begin, size + 1, positions).delta(positions) < delta){
					itpl = ITPL(itpl + 1);
					size++;
				}
				else
					break;
			}
			xassert(Data(itpl, begin, size, positions).delta(positions) < delta);
			out_data.push_back(Data(itpl, begin, size, positions));
			begin += size;
		}
		out_data.back().interval_size++;
	}

	int begin=0;
	for(int i=0;i<out_data.size();i++){
		Data& cur = out_data[i];
		xassert(begin == cur.interval_begin);
		xassert(cur.interval_size>0);
		begin += cur.interval_size;
	}
	xassert(begin == positions.size());
}

template<class T, class CycleOp>
template<class InterpolatorData>
void Refiner<T, CycleOp>::export(Interpolator3dx<InterpolatorData>& interpolator) const
{
	interpolator.values.resize(out_data.size());
	for(int i = 0; i < out_data.size(); i++){
		InterpolatorData& data = interpolator.values[i];
		const Data& o = out_data[i];
		data.itpl = o.itpl;
		data.tbegin = float(o.interval_begin)/float(interval_size);
		data.inv_tsize = 1.0f / (float(o.interval_size) / float(interval_size));

		const int size = T::size;
		xassert(size == InterpolatorData::size);
		memset(data.a, 0, sizeof(data.a));
		switch(o.itpl){
		case ITPL_CONSTANT:
			for(int j = 0; j < size; ++j)
				data.a[j] = o.a0[j];
			break;
		case ITPL_LINEAR:
			for(int j = 0; j < size; ++j){
				data.a[2*j] = o.a0[j];
				data.a[2*j + 1] = o.a1[j];
			}
			break;
		case ITPL_SPLINE:
			for(int j = 0; j < size; ++j){
				data.a[4*j] = o.a0[j];
				data.a[4*j + 1] = o.a1[j];
				data.a[4*j + 2] = o.a2[j];
				data.a[4*j + 3] = o.a3[j];
			}
			break;
		}
	}
}

class RefinerBool
{
public:
	void addValue(bool value) { visibility_.push_back(value); }
	void refine(int interval_size, bool cycled);
	void export(Interpolator3dxBool& interpolator) const;

private:
	struct Node{
		float interval_begin;
		float interval_size;
		bool value;
	};

	typedef vector<Node> Nodes;

	Nodes nodes_;
	vector<bool> visibility_;
};

struct QuatCycleOp
{
	static void cycle(const VectTemplate<4>& example,VectTemplate<4>& target)
	{
		const nsub=4;
		VectTemplate<4> rot_prev = example;
		VectTemplate<4> cur_inv;
		for(int isub=0;isub<nsub;isub++)
			cur_inv[isub]=-target[isub];
		float d=rot_prev.distance(target);
		float d_inv=rot_prev.distance(cur_inv);
		if(d_inv<d)
			target=cur_inv;
	}
};

typedef VectTemplate<1> VectScale;
typedef VectTemplate<1> VectOpacity;
typedef VectTemplate<3> VectPosition;
typedef VectTemplate<4> VectRotation;
typedef VectTemplate<4> VectColor;
typedef VectTemplate<6> VectUV;

typedef Refiner<VectScale> RefinerScale;
typedef Refiner<VectOpacity> RefinerOpacity;
typedef Refiner<VectPosition> RefinerPosition;
typedef Refiner<VectRotation, QuatCycleOp> RefinerRotation;
typedef Refiner<VectColor> RefinerColor;
typedef Refiner<VectUV>      RefinerUV;
