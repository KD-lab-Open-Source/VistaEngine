#ifndef __N_PARTICLE_KEY_H_INCLUDED__
#define __N_PARTICLE_KEY_H_INCLUDED__

class Archive;

struct KeyGeneral
{
	float time;
	static float time_delta;

	void serialize(Archive& ar);
};

template<class Key>
class CKeyBase:public vector<Key>
{
public:
	typedef typename vector<Key>::iterator iterator;
	typedef typename Key::value value;

	value Get(float t) const;
	Key& InsertKey(float t);
	Key* GetOrCreateKey(float t,float life_time=1,float create_time=0,bool* create=0);
};

template<class Key>
typename CKeyBase<Key>::value CKeyBase<Key>::Get(float t) const
{
	if(empty())return Key::none;
	if(size()==1)return (*this)[0].Val();

	if(t<(*this)[0].time)
		return (*this)[0].Val();

	for(int i=1;i<size();i++)
	if(t<(*this)[i].time)
	{
		const Key& f0=(*this)[i-1];
		const Key& f1=(*this)[i];
		float dx=f1.time-f0.time;
		xassert(dx>=0);
		xassert(t>=f0.time);
		float tx=(t-f0.time)/dx;

		value out;
		f0.interpolate(out,f0.Val(),f1.Val(),tx);
		return out;
	}

	return back().Val();
}

template<class Key>
Key& CKeyBase<Key>::InsertKey(float t)
{
	Key p;
	p.Val()=Get(t);
	p.time=t;

	if(!empty())
	{
		if(t<front().time)
		{
			insert(begin(),p);
			return front();
		}
		
		for(int i=1;i<size();i++)
		if(t<(*this)[i].time)
		{
			return *insert(begin()+i,p);		
		}
	}

	push_back(p);
	return back();
}

template<class Key>
Key* CKeyBase<Key>::GetOrCreateKey(float t,float life_time,float create_time,bool* create)
{
	if(create)
		*create=false;
	if(life_time<KeyGeneral::time_delta)
		return &(*this)[0];

	iterator it;
	FOR_EACH(*this,it)
	{
		Key& p=*it;
		float tp=p.time*life_time+create_time;
		if(fabsf(tp-t)<=KeyGeneral::time_delta)
			return &p;
	}

	if(create)
		*create=true;
	return &InsertKey((t-create_time)/life_time);
}


struct KeyFloat:KeyGeneral
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
};

class Saver;
class CLoadIterator;

class CVectVect3f : public vector<Vect3f>
{
public:
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
};

class CKey:public CKeyBase<KeyFloat>
{
public:
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
};
struct KeyPos:public KeyGeneral
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

class CKeyPos:public CKeyBase<KeyPos>
{
public:
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
protected:
	void SaveInternal(Saver& s);
};

class CKeyPosHermit:public CKeyPos
{
public:
	enum CLOSING
	{
		T_CLOSE,
		T_FREE,
		T_CYCLE,
	};

	CLOSING cbegin,cend;

	CKeyPosHermit();
	value Get(float t);

	Vect3f Clamp(int i);
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
};

struct KeyRotate:public KeyGeneral
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

class CKeyRotate:public CKeyBase<KeyRotate>
{
public:
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
};

struct KeyColor:public KeyGeneral,public sColor4f
{

	typedef sColor4f value;
	static value none;
	value& Val(){return (value&)*this;};
	const value& Val()const{return (value&)*this;};
	inline void interpolate(value& out,const value& in1,const value& in2,float tx) const
	{
		out=in1*(1-tx)+in2*tx;
	}

	void serialize(Archive& ar);
};

class CKeyColor:public CKeyBase<KeyColor>
{
public:
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
	void serialize(Archive& ar);

	void MulToColor(sColor4f color);
};

/////////////////////////////
//BackVector Ќесортированые одинакового типа элементы. «аменить дл€ прозрачности обращение к key, 
//на vector<bool> stopped_map, написать IsFree.
template <class type>
class BackVector:public vector<type>
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
	void resize(size_type __n);//¬се новые вертексы считаютс€ уже использованными.
};


template <class type> int BackVector<type>::GetIndexFree()
{
	int FreeParticle=-1;
	try {
	if(stopped.empty())
	{
		FreeParticle=size();
		push_back(type());
		bempty.push_back(false);
	}else
	{
		FreeParticle=stopped.back();
		stopped.pop_back();
		VISASSERT(bempty[FreeParticle]);
	}
	} catch (...) {
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
