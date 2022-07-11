#ifndef __N_PARTICLE_KEY_H_INCLUDED__
#define __N_PARTICLE_KEY_H_INCLUDED__

#include "Render\Inc\rd.h"
#include "XMath\Colors.h"
#include "XMath\KeysBase.h"
#include "Serialization\Serialization.h"

struct RENDER_API KeyFloat : KeyBase
{
	KeyFloat(){}
	KeyFloat(float time_,float f_){time=time_;f=f_;}
	float f;

	typedef float value;
	static value none;
	value& Val(){return f;};
	const value& Val() const{return f;};
	inline void interpolate(value& out,const value& in1,const value& in2,float tx) const
	{
		out=in1*(1-tx)+in2*tx;
	}
	void serialize(Archive& ar){
		KeyBase::serialize(ar);
		ar.serialize(f, "value", "Значение");		
	}
};

inline float& keyValue(KeyFloat& keyFloat){
	return keyFloat.f;
}

class RENDER_API KeysFloat : public KeysBase<KeyFloat, KeysFloat>
{
public:
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
};

struct KeyPos : public KeyBase
{
	Vect3f pos;

	typedef Vect3f value;
	static value none;
	value& Val(){return pos;};
	const value& Val() const{return pos;};
	inline void interpolate(value& out,const value& in1,const value& in2,float tx) const
	{
		out=in1*(1-tx)+in2*tx;
	}
};

class RENDER_API KeysPos : public KeysBase<KeyPos, KeysPos>
{
public:
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
protected:
	void SaveInternal(Saver& s);
};

class RENDER_API KeysPosHermit : public KeysPos
{
public:
	enum CLOSING
	{
		T_CLOSE,
		T_FREE,
		T_CYCLE,
	};

	CLOSING cbegin,cend;

	KeysPosHermit();
	value Get(float t);

	Vect3f Clamp(int i);
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
};

struct RENDER_API KeyRotate : public KeyBase
{
	QuatF pos;

	typedef QuatF value;
	static value none;
	value& Val(){return pos;};
	const value& Val() const{return pos;};
	inline void interpolate(value& out,const value& in1,const value& in2,float tx) const
	{
		out.slerp(in1,in2,tx);
	}
};

class RENDER_API KeysRotate : public KeysBase<KeyRotate, KeysRotate>
{
public:
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
};

///////////////////////////////////////////

class CVectVect3f : public vector<Vect3f>
{
public:
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);

	bool serialize(Archive& ar, const char* name, const char* nameAlt){ return ar.serialize(static_cast<vector<Vect3f>&>(*this), name, nameAlt); }
};

/////////////////////////////
//BackVector Несортированые одинакового типа элементы. Заменить для прозрачности обращение к key, 
//на vector<bool> stopped_map, написать IsFree.
template <class type>
class BackVector : public vector<type>
{
	vector<int>	stopped;
	vector<bool> bempty;
public:
	bool is_empty(){return size()<=stopped.size();}
	type& GetFree();
	int GetIndexFree();
	void SetFree(int n);
	bool IsFree(int n){return bempty[n];}
	void clear();
	void Compress();
	void resize(size_type __n);//Все новые вертексы считаются уже использованными.
};


template <class type> int BackVector<type>::GetIndexFree()
{
	int FreeParticle=-1;
	if(stopped.empty())
	{
		FreeParticle=size();
		push_back(type());
		bempty.push_back(false);
	}else
	{
		FreeParticle=stopped.back();
		stopped.pop_back();
		xassert(bempty[FreeParticle]);
	}

	bempty[FreeParticle]=false;
//	(*this)[FreeParticle].key=0;
	return FreeParticle;
}

template <class type> type& BackVector<type>::GetFree()
{
	int FreeParticle=GetIndexFree();
	return (*this)[FreeParticle];
}

template <class type>
void BackVector<type>::SetFree(int n)
{
//	type &p=(*this)[n];
//	p.key=-1;
	xassert(n>=0 && n<bempty.size());
	xassert(!bempty[n]);
	bempty[n]=true;
	stopped.push_back(n);
}

template <class type>
void BackVector<type>::Compress()
{
	if(size()<6)
		return;
	if(stopped.size()*2<=size())
		return;
	stopped.clear();
	int curi=0;
	int i;
	for(i=0;i<size();i++)
	{
		//if((*this)[i].key!=-1)
		if(!bempty[i])
		{
			if(i!=curi)
			{
				(*this)[curi]=(*this)[i];
			}
			curi++;
		}
	}

	__super::resize(curi);
	bempty.resize(curi);
	for(i=0;i<curi;i++)
		bempty[i]=false;
}

template <class type>
void BackVector<type>::clear()
{
	stopped.clear();
	__super::clear();
	bempty.clear();
}

template <class type>
void BackVector<type>::resize(size_type n)
{
	int prev_size=size();
	xassert(prev_size<=n);
	xassert(prev_size==bempty.size());
	__super::resize(n);
	bempty.resize(n,false);

}

#endif
