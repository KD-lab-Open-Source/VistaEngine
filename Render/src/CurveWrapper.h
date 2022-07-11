#ifndef __CURVE_WRAPPER_H_INCLUDED__
#define __CURVE_WRAPPER_H_INCLUDED__

#include <string>
#include "Handle.h"

class CurveWrapperBase : public ShareHandleBase{
public:
	virtual CurveWrapperBase* clone() = 0;
	virtual std::size_t size() const = 0;
	virtual float value(int index) const = 0;
	virtual void setValue(int index, float value) = 0;
	virtual float time(int index) const = 0;
	virtual void setPoint(int index, float time, float value) = 0;
	virtual bool keyPerGeneration() const = 0;
	const char* name() const{ return name_.c_str(); }
	void setName(const char* name){
		name_ = name;
	}
protected:
	std::string name_;
	static class CurveCollector* currentCollector_;
	friend class CurveCollector;
};

class CurveCollector{
public:
	CurveCollector();
	~CurveCollector();
	typedef std::vector<ShareHandle<CurveWrapperBase > > Curves;
	Curves& curves(){ return curves_; }
	void collectMe(CurveWrapperBase* wrapper){
		curves_.push_back(wrapper->clone());
	}
protected:
	Curves curves_;
};

inline float& keyValue(float& value){
	return value;
}

struct EffectBeginSpeed;
inline float& keyValue(EffectBeginSpeed& value);

struct KeyFloat;
inline float keyValue(const KeyFloat& keyFloat);

template<class KeyType>
class CurveWrapper: public CurveWrapperBase{
public:
	CurveWrapper()
	: keys_(0)
	, scale_(1.0f)
	{
	}
	CurveWrapper(KeyType& keys, float scale, bool keyPerGeneration, const char* name = "")
	: keys_(&keys)
	, scale_(scale)
	, keyPerGeneration_(keyPerGeneration)
	{
		name_ = name;
	}
	bool serialize(Archive& ar, const char* name, const char* nameAlt){
		if(ar.isEdit()){
			setName(name);
			if(ar.isOutput() && currentCollector_)
				currentCollector_->collectMe(this);
			if(ar.openStruct(*this, name, nameAlt)){
				ar.serialize(*keys_, "keys", "Ключи");
				ar.closeStruct(name);
			}
			return true;
		}
		else
			return ar.serialize(*keys_, name, nameAlt);
	}
	CurveWrapperBase* clone(){
		return new CurveWrapper((*keys_), scale_, keyPerGeneration_, name_.c_str());
	}	
	float value(int index) const{
		return keyValue((*keys_)[index]) / scale_;
	}
	void setValue(int index, float value){
		keyValue((*keys_)[index]) = value * scale_;
	}
	std::size_t size() const{
		return keys_->size();
	}
	float time(int index) const{
		return (*keys_)[index].time;
	}
	void setPoint(int index, float time, float value){
		(*keys_)[index].time = time;
		keyValue((*keys_)[index]) = value * scale_;
	}
	virtual bool keyPerGeneration() const{ return keyPerGeneration_; }
protected:
	KeyType* keys_;
	float scale_;
	bool keyPerGeneration_;
};

template<class KeyType>
CurveWrapper<KeyType> wrapCurve(KeyType& keys, bool keyPerGeneration = false, float scale = 1.0f){
	return CurveWrapper<KeyType>(keys, scale, keyPerGeneration);
}

#endif
