#ifndef __INTERPOLATOR3DX_H_INCLUDED__
#define __INTERPOLATOR3DX_H_INCLUDED__

#include "Render\Inc\3dx.h"
#include "Render\3dx\Saver.h"
#include "Serialization\Serialization.h"

template<int template_size>
struct SplineData
{
	typedef float Type;
	enum { size = template_size };

	ITPL itpl;
	float tbegin;
	float inv_tsize;
	float a[size*4];

	void interpolate(float t, float* out) const
	{
		switch(itpl){
		case ITPL_CONSTANT: {
			for(int i = 0; i < size; i++)
				out[i] = a[i];
			} break;
		case ITPL_LINEAR: {
			for(int i = 0; i < size; i++)
				out[i] = a[2*i] + a[2*i + 1]*t;
			} break;
		case ITPL_SPLINE: {
			float t2=t*t;
			float t3=t2*t;
			for(int i = 0; i < size; i++){
				int i4 = i*4;
				out[i] = a[i4] + a[i4 + 1]*t + a[i4 + 2]*t2 + a[i4 + 3]*t3;
			}
			} break;
		default:
			xassert(0);
		}
	}

	void serialize(Archive& ar)
	{
		MergeBlocksAuto merge(ar);
		ar.serialize((int&)itpl, "type", "type");
		ar.serialize(tbegin, "tbegin", "tbegin");
		ar.serialize(inv_tsize, "inv_tsize", "inv_tsize");
		ar.serializeArray(a, "a", "a");
	}
};

struct SplineDataBool
{
	typedef bool Type;
	enum { size = 1 };

	float tbegin;
	float inv_tsize;
	int value; // bool a

	void interpolate(float t, bool* out) const
	{
		*out = value != 0;
	}

	void serialize(Archive& ar);
};

class StaticChainsBlock;

template<class _Data>
class Interpolator3dx
{
public:
	typedef _Data Data;
	typedef typename Data::Type Type;
	typedef vector<Data> Values;
	Values values;

	void Interpolate(float globalt, Type* out,int index) const
	{
		xassert(index>=0 && index<values.size());
		const Data& in = values[index];
		float localt=(globalt-in.tbegin)*in.inv_tsize-0.5f;
		in.interpolate(localt,out);
	}

	void InterpolateSlow(float globalt,float* out) const
	{
		int idx=FindIndex(globalt);
		Interpolate(globalt,out,idx);
	}

	int FindIndex(float globalt) const
	{
		//потом возможно заменить на binary find
		return Next(globalt,0);
	}

	int FindIndexRelative(float globalt,int cur) const
	{
		xassert(cur>=0 && cur<values.size());
		const Data& p=values[cur];
		if(globalt>=p.tbegin)
			return Next(globalt,cur);
		return Prev(globalt,cur);
	}

	//cur - значение, которое вернул FindIndex или Next.
	// предполагается, что globalt постоянно растёт.
	int Next(float globalt,int cur) const
	{
		int size=values.size();
		for(int i=cur;i<size;i++)
		{
			const Data& p=values[i];
			float t=(globalt-p.tbegin)*p.inv_tsize;
			if(t>=0 && t<=1)
			{
				return i;
			}
		}

		return size-1;
	}

	// предполагается, что globalt постоянно уменьшается.
	int Prev(float globalt,int cur) const
	{
		int size=values.size();
		for(int i=cur;i>=0;i--)
		{
			const Data& p=values[i];
			float t=(globalt-p.tbegin)*p.inv_tsize;
			if(t>=0 && t<=1)
			{
				return i;
			}
		}

		return 0;
	}

	bool serialize(Archive& ar, const char* name, const char* nameAlt)
	{
		return ar.serialize(values, name, nameAlt);
	}

	void LoadIndex(CLoadIterator ld, const StaticChainsBlock& chains_block)
	{
		int index = -1;
		int length = 0;
		ld >> index;
		ld >> length;
		xassert(index >= 0);
		xassert(length > 0);

		chains_block.setValues(*this, index, length);
	}
};

typedef Interpolator3dx<SplineDataBool> Interpolator3dxBool;

typedef SplineData<1> SplineDataScale;
typedef Interpolator3dx<SplineDataScale> Interpolator3dxScale;

typedef SplineData<3> SplineDataPosition;
typedef Interpolator3dx<SplineDataPosition> Interpolator3dxPosition;

typedef SplineData<4> SplineDataRotation;
typedef Interpolator3dx<SplineDataRotation> Interpolator3dxRotation;

typedef SplineData<6> SplineDataUV;
typedef Interpolator3dx<SplineDataUV> Interpolator3dxUV;

class ChainConverter;

class StaticChainsBlock{
public:
	StaticChainsBlock();
	~StaticChainsBlock();

	void load(CLoadData* load_data);
    void free();

	void setValues(Interpolator3dxBool& interpolator, int index, int length) const { interpolator.values.insert(interpolator.values.begin(), bools_ + index, bools_ + index + length); }
	void setValues(Interpolator3dxPosition& interpolator, int index, int length) const{ interpolator.values.insert(interpolator.values.begin(), positions_ + index, positions_ + index + length); }
	void setValues(Interpolator3dxRotation& interpolator, int index, int length) const{ interpolator.values.insert(interpolator.values.begin(), rotations_ + index, rotations_ + index + length); }
	void setValues(Interpolator3dxScale& interpolator, int index, int length) const{ interpolator.values.insert(interpolator.values.begin(), scales_ + index, scales_ + index + length); }
	void setValues(Interpolator3dxUV& interpolator, int index, int length) const{ interpolator.values.insert(interpolator.values.begin(), uvs_ + index, uvs_ + index + length); }

	ChainConverter& converter();
	
	bool isEmpty() const{ return block_.buffer() == 0; }
	int GetBlockSize(){return block_.size();}

private:
	ChainConverter* converter_;

    MemoryBlock block_;

	SplineDataPosition*    positions_; int num_positions_;
	SplineDataRotation*    rotations_; int num_rotations_;
    SplineDataScale*       scales_;    int num_scales_;
	SplineDataBool*        bools_;     int num_bools_;
	SplineDataUV*          uvs_;       int num_uvs_;

	friend ChainConverter;
};

#endif
