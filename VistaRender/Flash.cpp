#include "stdafx.h"
#include "Flash.h"
#include "VistaRender\postEffects.h"
#include "DebugPrm.h"
#include "Render\inc\IRenderDevice.h"

Flash::Flash(PostEffectManager* manager)
{
	manager_ = manager;
	color_ = Color4c(255,255,255);
	active_ = false;
	
	inited_ = false;
	intensity_[0] = intensity_[1] = 0.0f;
	count_=0;
	intensitySum_ = 0.0f;
}

Flash::~Flash()
{
}

// графический квант
void Flash::draw()
{
	if (!active_ || !inited_)
		return;

	cInterfaceRenderDevice* rd = gb_RenderDevice;
	Color4f color = Color4f(color_);
	float intFactor = timer_.factor();
	float factor = clamp(((1.f - intFactor) * intensity_[0] + intFactor * intensity_[1]),0.0f,1.0f);
	color.a *= factor;
	float l = 0.15f;
	if(PostEffectBloom* bloom = (PostEffectBloom*)manager_->getEffect(PE_BLOOM)){
		if(bloom->isActive())
			l = bloom->defaultLuminance();
		bloom->SetLuminace(l-(l-0.06f)*color.a);
		color*=color.a;
		bloom->SetColor(color);
	}
}

void Flash::init(float _intensity){
	//addIntensity(_intensity);
	intensity_[0] = intensity_[1] = _intensity;
	inited_ = true;
}

void Flash::setActive(bool _active)
{
	if(!_active && count_>0)
		return;
	active_ = _active;

	PostEffectBloom* bloom = (PostEffectBloom*)manager_->getEffect(PE_BLOOM);
	if(!active_)
	{
		inited_ = false;
		if(bloom)
			bloom->RestoreDefaults();
	}
	if(bloom)
		bloom->SetExplode(active_);
}

void Flash::setColor(Color4c _color)
{
	color_ = _color;
}

// вызывать из логического кванта
void Flash::addIntensity(float _intensity)
{
	intensitySum_ += _intensity;
}

void Flash::setIntensity()
{
	if(!active_)
		return;
	if(inited_){
		intensity_[0] = intensity_[1];
		intensity_[1] = intensitySum_;
		timer_.start(logicTimePeriod);
	}else
		init(intensitySum_);
	intensitySum_ = 0.0f;
}
