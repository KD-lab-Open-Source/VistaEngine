#ifndef __SOURCES_H_INCLUDED__
#define __SOURCES_H_INCLUDED__

#include "SourceEffect.h"

class SourceWater : public SourceEffect
{
public:
	SourceWater(const SourceWater& src)
	: SourceEffect(src)
	, deltaHeight_(src.deltaHeight_)
	{
	}
	SourceBase* clone() const {
		return new SourceWater(*this);
	}

	SourceWater()
	: SourceEffect()
	, deltaHeight_(3)
	{
	}

	~SourceWater() {
		if (enabled_) {
			effectStop();
		}
	}
	void quant();
	void serialize(Archive& ar);
	SourceType type()const{return SOURCE_WATER;}

protected:
	void start();
	void stop();

private:
	float deltaHeight_;
};

class SourceIce : public SourceDamage
{
public:
	SourceIce()
	: SourceDamage()
	, deltaTemperature_(3.0f)
	{
	}
	SourceIce(const SourceIce& src)
	: SourceDamage(src) {
		deltaTemperature_ = src.deltaTemperature_;
	}
	SourceBase* clone() const {
		return new SourceIce(*this);
	}
	void serialize(Archive& ar);
	void quant();
	SourceType type()const{return SOURCE_ICE;}
private:
	float deltaTemperature_;
};

class SourceWaterWave : public SourceBase
{
public:
	SourceWaterWave();
	SourceWaterWave(const SourceWaterWave& sww);
	SourceBase* clone() const{
		return new SourceWaterWave(*this);
	}
	~SourceWaterWave();

	void serialize(Archive& ar);
	void quant();
	SourceType type()const {return SOURCE_WATER_WAVE;}
protected:
	bool autoKill_;
	Vect2i gridSize_;
	int waveLenght_;
	int curRadius_;
	float amplitude_;
	cWater* water_;
	float speed_;
	int beginRadius_;
	int maxRadius_;
	int fadeRadius_;
	int coordShift_;
	bool flatWave_;
	float sizeWave_;

protected:
	void start();
	void stop();

};

class cWaterBubbleCenter;
class SourceBubble : public SourceBase
{
	cWaterBubbleCenter& waterBubbleCenter;
public:
	SourceBubble();
	SourceBubble(const SourceBubble& sb);
	SourceBase* clone() const {
		return new SourceBubble(*this);
	}
	~SourceBubble();

	void AddCenter();
	void DeleteCenter();

	void serialize(Archive& ar);
	void setPose(const Se3f& pos, bool init);
	void setRadius(float radius);
	SourceType type()const {return SOURCE_BUBBLE;}

protected:
	void start();
	void stop();

};

#endif

