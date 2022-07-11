#ifndef __INTERPOLATOR3DX_H_INCLUDED__
#define __INTERPOLATOR3DX_H_INCLUDED__

#include "..\..\IGameExporter2\3dx.h"


template<class sInterpolateType>
class BlockPointer{
public:
	BlockPointer(vector<sInterpolateType>& array)
	: start_(&array[0])
	, count_(array.size())
    , justIndex_(false)
	{
	}
	BlockPointer(const sInterpolateType* start = 0, size_t length = 0)
	: start_(start)
	, count_(length)
	, justIndex_(false)
	{
	}

    ~BlockPointer(){
        //xassert(justIndex_ == false);
    }

    // CONVERSION
    void setIndex(int index, int length){
        start_ = reinterpret_cast<sInterpolateType*>(index);
        count_ = length;
        justIndex_ = true;
    }
	int index() const { return reinterpret_cast<int>(start_); }
	bool isJustIndex() const { return justIndex_; };
    // ^^^

	const sInterpolateType& operator*() const{ return *start_; }
	const sInterpolateType& operator[](size_t index) const{ return *(start_ + index); }

	size_t size() const{ return count_; }
	bool empty() const{ return size() == 0; }

	const sInterpolateType* get() const{ return start_; }
private:
    // CONVERSION
    bool justIndex_;
    // ^^^
	const sInterpolateType* start_;
	size_t count_;
};

template<int template_size>
struct sInterpolate3dx
{
	ITPL type;
	float tbegin;
	float inv_tsize;
	float a[template_size*4];

	bool operator!=(const sInterpolate3dx& rhs) const{
		return !operator==(rhs);
	}
	bool operator==(const sInterpolate3dx& rhs) const{
		if(type != rhs.type ||
			(fabsf(tbegin - rhs.tbegin) > FLT_COMPARE_TOLERANCE) ||
			(fabsf(inv_tsize - rhs.inv_tsize) > FLT_COMPARE_TOLERANCE))
			return false;
		int count = 4;
		switch(type){
		case ITPL_CONSTANT:
			count = 1;
			break;
		case ITPL_LINEAR:
			count = 2;
			break;
		case ITPL_SPLINE:
			count = 4;
			break;
		default:
			xassert(0);
		}
		for(int i = 0; i < count * 4; ++i)
			if(fabsf(a[i] - rhs.a[i]) > FLT_COMPARE_TOLERANCE)
				return false;
		return true;
	}
};

struct sInterpolate3dxBool
{
	float tbegin;
	float inv_tsize;
	int value; // bool a
};

class StaticChainsBlock;

class Interpolator3dxBool
{
public:
	typedef BlockPointer< sInterpolate3dxBool > ValuesType;
	typedef vector<sInterpolate3dxBool > darray;
	darray data_;
	ValuesType values;

	inline void Interpolate(float globalt,bool* out,int index) const;
	inline int FindIndex(float globalt) const {return Next(globalt,0);};
	inline int FindIndexRelative(float globalt,int cur) const;
	
	//cur - значение, которое вернул FindIndex или Next.
	// предполагается, что globalt постоянно растёт.
	inline int Next(float globalt,int cur) const;

	// предполагается, что globalt постоянно уменьшается.
	inline int Prev(float globalt,int cur) const;

	inline void LoadIndex(CLoadIterator ld, const StaticChainsBlock& chains_block);
	inline void SaveIndex(Saver& saver, const StaticChainsBlock& chains_block);
};

template<int tsize>
class Interpolator3dx
{
public:
	typedef vector<sInterpolate3dx<tsize> > darray;
//	darray data_;

	typedef BlockPointer< sInterpolate3dx<tsize> > ValuesType;
	ValuesType values;

	Interpolator3dx(){
	}

	Interpolator3dx(const Interpolator3dx& original){
		//data_ = original.data_;
		//if(!data_.empty())
		//	values = BlockPointer<sInterpolate3dx<tsize> >(data_);
		//else
			values = original.values;
	}

	void Interpolate(float globalt,float* out,int index);
	inline void InterpolateSlow(float globalt,float* out);
	int FindIndex(float globalt);
	int FindIndexRelative(float globalt,int cur) const;

	//cur - значение, которое вернул FindIndex или Next.
	// предполагается, что globalt постоянно растёт.
	int Next(float globalt,int cur) const;

	// предполагается, что globalt постоянно уменьшается.
	int Prev(float globalt,int cur) const;


	void LoadIndex(CLoadIterator ld, const StaticChainsBlock& chains_block);
	void SaveIndex(Saver& saver, const StaticChainsBlock& chains_block);
};

typedef Interpolator3dx<1> Interpolator3dxScale;
typedef Interpolator3dx<3> Interpolator3dxPosition;
typedef Interpolator3dx<4> Interpolator3dxRotation;
typedef Interpolator3dx<6> Interpolator3dxUV;

typedef sInterpolate3dx<1> sInterpolate3dxScale;
typedef sInterpolate3dx<3> sInterpolate3dxPosition;
typedef sInterpolate3dx<4> sInterpolate3dxRotation;
typedef sInterpolate3dx<6> sInterpolate3dxUV;

template<class T> struct Type2Type{ typedef T Type; };

struct sInterpolate3dxBool;
class Interpolator3dxBool;

class ChainConverter;
class StaticChainsBlock{
	friend ChainConverter;
public:
	void load(CLoadData* load_data);
	void save(Saver& saver) const;
    void free();

	StaticChainsBlock();
	~StaticChainsBlock();

	int indexOf(const Interpolator3dxPosition& interpolator) const{ return int(interpolator.values.get() - positions_); }
	int indexOf(const Interpolator3dxRotation& interpolator) const{ return int(interpolator.values.get() - rotations_); }
	int indexOf(const Interpolator3dxScale& interpolator) const{ return int(interpolator.values.get() - scales_); }
	int indexOf(const Interpolator3dxBool& interpolator) const{ return int(interpolator.values.get() - bools_); }
	int indexOf(const Interpolator3dxUV& interpolator) const{ return int(interpolator.values.get() - uvs_); }

	inline void setValues(Interpolator3dxBool& interpolator, int index, int length) const;
	void setValues(Interpolator3dxPosition& interpolator, int index, int length) const{ interpolator.values = BlockPointer<sInterpolate3dxPosition>(positions_ + index, length); }
	void setValues(Interpolator3dxRotation& interpolator, int index, int length) const{ interpolator.values = BlockPointer<sInterpolate3dxRotation>(rotations_ + index, length); }
	void setValues(Interpolator3dxScale& interpolator, int index, int length) const{ interpolator.values = BlockPointer<sInterpolate3dxScale>(scales_ + index, length); }
	void setValues(Interpolator3dxUV& interpolator, int index, int length) const{ interpolator.values = BlockPointer<sInterpolate3dxUV>(uvs_ + index, length); }

	ChainConverter& converter();
	
	bool isEmpty() const{ return block_ == 0; }
	int GetBlockSize(){return block_size_;}

private:
	ChainConverter* converter_;

    void* block_;
 	size_t block_size_;

	sInterpolate3dxPosition*    positions_; int num_positions_;
	sInterpolate3dxRotation*    rotations_; int num_rotations_;
    sInterpolate3dxScale*       scales_;    int num_scales_;
	sInterpolate3dxBool*        bools_;     int num_bools_;
	sInterpolate3dxUV*          uvs_;       int num_uvs_;
};

inline
void InterpolateX(float t,float* out,const sInterpolate3dx<1>& in)
{
	switch(in.type)
	{
	case 0:
		out[0]=in.a[0];
		break;
	case 1:
		out[0]=in.a[0]+in.a[1]*t;
		break;
	case 2:
		{
			float t2=t*t;
			float t3=t2*t;
			out[0]=in.a[0]+in.a[1]*t+in.a[2]*t2+in.a[3]*t3;
		}
		break;
	default:
		xassert(0);
	}
}

inline
void InterpolateX(float t,float* out,const sInterpolate3dx<2>& in)
{
	switch(in.type)
	{
	case 0:
		out[0]=in.a[0];
		out[1]=in.a[1];
		break;
	case 1:
		out[0]=in.a[0]+in.a[1]*t;
		out[1]=in.a[2]+in.a[3]*t;
		break;
	case 2:
		{
			float t2=t*t;
			float t3=t2*t;
			out[0]=in.a[0]+in.a[1]*t+in.a[2]*t2+in.a[3]*t3;
			out[1]=in.a[4]+in.a[5]*t+in.a[6]*t2+in.a[7]*t3;
		}
		break;
	default:
		xassert(0);
	}
}

inline
void InterpolateX(float t,float* out,const sInterpolate3dx<3>& in)
{
	switch(in.type)
	{
	case 0:
		out[0]=in.a[0];
		out[1]=in.a[1];
		out[2]=in.a[2];
		break;
	case 1:
		out[0]=in.a[0]+in.a[1]*t;
		out[1]=in.a[2]+in.a[3]*t;
		out[2]=in.a[4]+in.a[5]*t;
		break;
	case 2:
		{
			float t2=t*t;
			float t3=t2*t;
			out[0]=in.a[0]+in.a[1]*t+in.a[2]*t2+in.a[3]*t3;
			out[1]=in.a[4]+in.a[5]*t+in.a[6]*t2+in.a[7]*t3;
			out[2]=in.a[8]+in.a[9]*t+in.a[10]*t2+in.a[11]*t3;
		}
		break;
	default:
		xassert(0);
	}
}

inline
void InterpolateX(float t,float* out,const sInterpolate3dx<4>& in)
{
	switch(in.type)
	{
	case 0:
		out[0]=in.a[0];
		out[1]=in.a[1];
		out[2]=in.a[2];
		out[3]=in.a[3];
		break;
	case 1:
		out[0]=in.a[0]+in.a[1]*t;
		out[1]=in.a[2]+in.a[3]*t;
		out[2]=in.a[4]+in.a[5]*t;
		out[3]=in.a[6]+in.a[7]*t;
		break;
	case 2:
		{
			float t2=t*t;
			float t3=t2*t;
			out[0]=in.a[0]+in.a[1]*t+in.a[2]*t2+in.a[3]*t3;
			out[1]=in.a[4]+in.a[5]*t+in.a[6]*t2+in.a[7]*t3;
			out[2]=in.a[8]+in.a[9]*t+in.a[10]*t2+in.a[11]*t3;
			out[3]=in.a[12]+in.a[13]*t+in.a[14]*t2+in.a[15]*t3;
		}
		break;
	default:
		xassert(0);
	}
}

inline
void InterpolateX(float t,float* out,const sInterpolate3dx<6>& in)
{
	switch(in.type)
	{
	case 0:
		out[0]=in.a[0];
		out[1]=in.a[1];
		out[2]=in.a[2];
		out[3]=in.a[3];
		out[4]=in.a[4];
		out[5]=in.a[5];
		break;
	case 1:
		out[0]=in.a[0]+in.a[1]*t;
		out[1]=in.a[2]+in.a[3]*t;
		out[2]=in.a[4]+in.a[5]*t;
		out[3]=in.a[6]+in.a[7]*t;
		out[4]=in.a[8]+in.a[9]*t;
		out[5]=in.a[10]+in.a[11]*t;
		break;
	case 2:
		{
			float t2=t*t;
			float t3=t2*t;
			out[0]=in.a[0]+in.a[1]*t+in.a[2]*t2+in.a[3]*t3;
			out[1]=in.a[4]+in.a[5]*t+in.a[6]*t2+in.a[7]*t3;
			out[2]=in.a[8]+in.a[9]*t+in.a[10]*t2+in.a[11]*t3;
			out[3]=in.a[12]+in.a[13]*t+in.a[14]*t2+in.a[15]*t3;
			out[4]=in.a[16]+in.a[17]*t+in.a[18]*t2+in.a[19]*t3;
			out[5]=in.a[20]+in.a[21]*t+in.a[22]*t2+in.a[23]*t3;
		}
		break;
	default:
		xassert(0);
	}
}

template<int tsize>
void Interpolator3dx<tsize>::Interpolate(float globalt,float* out,int index)
{
	xassert(index>=0 && index<values.size());
	const sInterpolate3dx<tsize>& in=values[index];
	float localt=(globalt-in.tbegin)*in.inv_tsize-0.5f;
	InterpolateX(localt,out,in);
}

template<int tsize>
int Interpolator3dx<tsize>::FindIndex(float globalt)
{
	//потом возможно заменить на binary find
	return Next(globalt,0);
}

template<int tsize>
void Interpolator3dx<tsize>::InterpolateSlow(float globalt,float* out)
{
	int idx=FindIndex(globalt);
	Interpolate(globalt,out,idx);
}

template<int tsize>
int Interpolator3dx<tsize>::FindIndexRelative(float globalt,int cur) const
{
	xassert(cur>=0 && cur<values.size());
	const sInterpolate3dx<tsize>& p=values[cur];
	if(globalt>=p.tbegin)
		return Next(globalt,cur);
	return Prev(globalt,cur);
}

template<int tsize>
int Interpolator3dx<tsize>::Next(float globalt,int cur) const
{
	int size=values.size();
	for(int i=cur;i<size;i++)
	{
		const sInterpolate3dx<tsize>& p=values[i];
		float t=(globalt-p.tbegin)*p.inv_tsize;
		if(t>=0 && t<=1)
		{
			return i;
		}
	}

	return size-1;
}


template<int tsize>
int Interpolator3dx<tsize>::Prev(float globalt,int cur) const
{
	int size=values.size();
	for(int i=cur;i>=0;i--)
	{
		const sInterpolate3dx<tsize>& p=values[i];
		float t=(globalt-p.tbegin)*p.inv_tsize;
		if(t>=0 && t<=1)
		{
			return i;
		}
	}

	return 0;
}

template<int tsize>
void Interpolator3dx<tsize>::LoadIndex(CLoadIterator ld, const StaticChainsBlock& chains_block)
{
	int index = -1;
	int length = 0;
	ld >> index;
	ld >> length;
	xassert(index >= 0);
	xassert(length > 0);

	chains_block.setValues(*this, index, length);

    /*
	if(!data_.empty()){
		xassert(values.size() == data_.size());
		for(int i = 0; i < data_.size(); ++i)
			xassert(values[i] == data_[i]);

		data_.clear();
	}
	*/
}

template<int tsize>
void Interpolator3dx<tsize>::SaveIndex(Saver& saver, const StaticChainsBlock& chains_block)
{
    int index = chains_block.indexOf(*this);
	xassert(index >= 0 && index < 65536);
	int length = values.size();
	xassert(length > 0 && length < 65536);
	saver << index;
	saver << length;
}

/*
template<int tsize>
void Interpolator3dx<tsize>::Save(Saver &s)
{
	s<<values.size();
	for(int i=0; i<values.size(); i++)
	{
		const sInterpolate3dx<tsize>& one=values[i];
		s<<(int)one.type;
		s<<one.tbegin;
		s<<(1/one.inv_tsize);
		int one_size=0;
		switch(one.type)
		{
		case ITPL_CONSTANT:
			one_size=1;
			break;
		case ITPL_LINEAR:
			one_size=2;
			break;
		case ITPL_SPLINE:
			one_size=4;
			break;
		default:
			assert(0);
		}

		for(int j=0;j<one_size*tsize;j++)
		{
			s<<one.a[j];
		}
	}
}
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void Interpolator3dxBool::Interpolate(float globalt,bool* out,int index) const
{
	xassert(index>=0 && index<values.size());
	const sInterpolate3dxBool& in=values[index];
	*out=bool(in.value);
}

int Interpolator3dxBool::FindIndexRelative(float globalt,int cur) const
{
	xassert(cur>=0 && cur<values.size());
	const sInterpolate3dxBool& p=values[cur];
	if(globalt>=p.tbegin)
		return Next(globalt,cur);
	return Prev(globalt,cur);
}

int Interpolator3dxBool::Next(float globalt,int cur) const
{
	int size=values.size();
	for(int i=cur;i<size;i++)
	{
		const sInterpolate3dxBool& p=values[i];
		float t=(globalt-p.tbegin)*p.inv_tsize;
		if(t>=0 && t<=1)
		{
			return i;
		}
	}

	return size-1;
}


int Interpolator3dxBool::Prev(float globalt,int cur) const
{
	int size=values.size();
	for(int i=cur;i>=0;i--)
	{
		const sInterpolate3dxBool& p=values[i];
		float t=(globalt-p.tbegin)*p.inv_tsize;
		if(t>=0 && t<=1)
		{
			return i;
		}
	}

	return 0;
}

void Interpolator3dxBool::LoadIndex(CLoadIterator ld, const StaticChainsBlock& chains_block)
{
	int index = -1;
	int length = 0;
	ld >> index;
	ld >> length;
 	xassert(index >= 0);
	xassert(length >= 0);
	data_.clear();
	Interpolator3dxBool& self = *this;
	chains_block.setValues(self, index, length);
}

/*
void Interpolator3dxBool::Save(Saver& s) const
{
	s<<values.size();
	for(int i=0;i<values.size();i++)
	{
		const sInterpolate3dxBool& one=values[i];
		s<<one.tbegin;
		float tsize=0;
		s<<(1/one.inv_tsize);
		s<<bool(one.value);
	}
}
*/
void Interpolator3dxBool::SaveIndex(Saver& saver, const StaticChainsBlock& chains_block)
{
    int index = chains_block.indexOf(*this);
	xassert(index >= 0 && index < 65536);
	int length = values.size();
	xassert(length > 0 && length < 65536);
	saver << index;
	saver << length;
}

inline void StaticChainsBlock::setValues(Interpolator3dxBool& interpolator, int index, int length) const
{
    interpolator.values = BlockPointer<sInterpolate3dxBool>(bools_ + index, length);
}

#endif
