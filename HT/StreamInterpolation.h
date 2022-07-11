#ifndef __STREAM_INTERPOLATION_H_INCLUDED__
#define __STREAM_INTERPOLATION_H_INCLUDED__
/*
Потоковая интерполяция данных.
Каждая команда - указатель на функцию.
int command(void* data);
Функция возвращает величину данных data,
которые ей нужны.
*/

#include "..\Render\inc\IVisGeneric.h"

typedef void (*InterpolateFunction)(XBuffer& buffer);

struct sColorInterpolate
{
	sColorInterpolate(){ colors[0] = colors[2] = sColor4f(1,1,1,0); colors[1] = sColor4f(1,1,1,1); }
	sColorInterpolate(const sColor4f& ambient, const sColor4f& diffuse, const sColor4f& specular) 
		{ colors[0] = ambient; colors[1] = diffuse; colors[2] = specular; }

	sColor4f colors[3]; // ambient, diffuse, specular
};

struct sAngleInterpolate
{
	float angle;
	eAxis axis;
};

class StreamInterpolator
{
public:
	StreamInterpolator();
	~StreamInterpolator();
	
	void process(float factor);
	void put(StreamInterpolator& s);
	void clear();

	StreamInterpolator& set(InterpolateFunction func);
	StreamInterpolator& set(InterpolateFunction func,cBaseGraphObject* obj); // для совместимости, будет удаляться

	template<class T>
	StreamInterpolator& operator<<(const T& p) {
		MTL();
		stream_.write(p);
		return *this;
	}

	static float factor() { return factor_; }

private:
	enum { flagValue = 21 };

	XBuffer stream_;

	static float factor_; 
};

extern MTSection streamLock; 
extern StreamInterpolator streamLogicInterpolator; // Можно писать из любого места логики, всегда залочен
extern StreamInterpolator streamLogicCommand; // Можно писать из любого места логики, всегда залочен
extern StreamInterpolator streamLogicPostCommand; // Можно писать из любого места логики, всегда залочен

void fSe3fInterpolation(XBuffer& stream);
void fNodeTransformInterpolation(XBuffer& stream);
void fPhaseInterpolation(XBuffer& stream);
void fChainInterpolation(XBuffer& stream);
void fVisibilityGroupInterpolation(XBuffer& stream);
void fAngleInterpolation(XBuffer& stream);
void fColorInterpolation(XBuffer& stream);
void fParticleRateInterpolation(XBuffer& stream);
void fPhaseInterpolationFade(XBuffer& stream);
void fChainInterpolationFade(XBuffer& stream);
void fHideByFactor(XBuffer& stream);
void fBeamInterpolation(XBuffer& stream);

void fCommandSetVisibilityGroupOfSet(XBuffer& stream);
void fCommandRelease(XBuffer& stream);
void fCommandSetCycle(XBuffer& stream);
void fCommandSetAutoDeleteAfterLife(XBuffer& stream);
void fCommandSimplyOpacity(XBuffer& stream);
void fCommandSetParticleRate(XBuffer& stream);
void fCommandSetTarget(XBuffer& stream);
void fCommandSetIgnored(XBuffer& stream);
void fCommandSetScale(XBuffer& stream);
void fCommandSetUserTransform(XBuffer& stream);
void fCommandResetIdleUnits(XBuffer& stream);
void fCommandAddIdleUnits(XBuffer& stream);
void fCommandHideByFactor(XBuffer& stream);
void fCommandSetUnderwaterSiluet(XBuffer& stream);

#endif
