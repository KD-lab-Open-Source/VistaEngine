#pragma once
#include "Colors.h"

class Archive;
class Saver;
class CLoadIterator;

struct KeyBase
{
	float time;

	void serialize(Archive& ar);

	static float time_delta;
};

template<class Key, class Derived>
class KeysBase : public vector<Key>
{
public:
	typedef typename vector<Key>::iterator iterator;
	typedef typename Key::value value;

	value Get(float t) const;
	Key& InsertKey(float t);
	Key* GetOrCreateKey(float t,float life_time=1,float create_time=0,bool* create=0);

	virtual bool serialize(Archive& ar, const char* name, const char* nameAlt){
		if(ar.isEdit()){
			if(ar.openStruct(*this, name, nameAlt, typeid(Derived).name())){
				ar.serialize(static_cast<vector<Key>&>(*this), "values", "��������");
				ar.closeStruct(name);
			}
			return true;
		}
		else{
			return ar.serialize(static_cast<vector<Key>&>(*this), name, nameAlt);
		}
	}
};

template<class Key, class Derived>
typename KeysBase<Key, Derived>::value KeysBase<Key, Derived>::Get(float t) const
{
	if(this->empty())return Key::none;
	if(this->size()==1)return (*this)[0].Val();

	if(t<(*this)[0].time)
		return (*this)[0].Val();

	for(int i=1;i<(int)this->size();i++)
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

	return this->back().Val();
}

template<class Key, class Derived>
Key& KeysBase<Key, Derived>::InsertKey(float t)
{
	Key p;
	p.Val()=Get(t);
	p.time=t;

	if(!this->empty())
	{
		if(t<this->front().time)
		{
			this->insert(this->begin(),p);
			return this->front();
		}

		for(int i=1;i<(int)this->size();i++)
			if(t<(*this)[i].time)
			{
				return *this->insert(this->begin()+i,p);
			}
	}

	this->push_back(p);
	return this->back();
}

template<class Key, class Derived>
Key* KeysBase<Key, Derived>::GetOrCreateKey(float t,float life_time,float create_time,bool* create)
{
	if(create)
		*create=false;
	if(life_time < KeyBase::time_delta)
		return &(*this)[0];

	iterator it;
	FOR_EACH((*this),it)
	{
		Key& p=*it;
		float tp=p.time*life_time+create_time;
		if(fabsf(tp-t)<=KeyBase::time_delta)
			return &p;
	}

	if(create)
		*create=true;
	return &InsertKey((t-create_time)/life_time);
}


/////////////////////////////////////////////////

struct KeyColor : public KeyBase, public Color4f
{
	typedef Color4f value;
	static value none;
	value& Val(){return (value&)*this;};
	const value& Val()const{return (value&)*this;};
	inline void interpolate(value& out,const value& in1,const value& in2,float tx) const
	{
		out=in1*(1-tx)+in2*tx;
	}

	void serialize(Archive& ar);
};

class KeysColor : public KeysBase<KeyColor, KeysColor>
{
public:
	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
	void serialize(Archive& ar);

	void MulToColor(Color4f color);
};
