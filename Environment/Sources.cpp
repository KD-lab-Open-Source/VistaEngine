#include "StdAfx.h"

#include "Universe.h"
#include "Environment.h"

#include "Sources.h"
#include "SourceFlash.h"
#include "SourceTornado.h"
#include "SourceLightning.h"
#include "SourceBlast.h"
#include "SourceShield.h"
#include "SourceZone.h"
#include "SourceTerTool.h"
#include "..\Water\Ice.h"
#include "..\Water\WaterGarbage.h"
#include "SourceCameraShaking.h"
#include "SourceTeleport.h"
#include "SourceDeleteGrass.h"
#include "SourceFlock.h"
#include "SourceLight.h"

#include "Serialization.h"
#include "RangedWrapper.h"
#include "TypeLibraryImpl.h"

#pragma warning(disable: 4355)

WRAP_LIBRARY(SourcesLibrary, "SourcesLibrary", "Источники", "Scripts\\Content\\SourcesLibrary", 0, true);

REGISTER_CLASS(SourceBase, SourceZone, "Зона на мире");
REGISTER_CLASS(SourceBase, SourceLightning, "Зона с молниями");
REGISTER_CLASS(SourceBase, SourceWater, "Источник воды");
REGISTER_CLASS(SourceBase, SourceBubble, "Источник пузырьков");
REGISTER_CLASS(SourceBase, SourceIce, "Источник льда");
REGISTER_CLASS(SourceBase, SourceFreeze, "Источник заморозки");
REGISTER_CLASS(SourceBase, SourceLight, "Источник света");
REGISTER_CLASS(SourceBase, SourceTerTool, "Источник тулзеров");
REGISTER_CLASS(SourceBase, SourceImpulse, "Источник импульса");
REGISTER_CLASS(SourceBase, SourceTornado, "Источник торнадо");
REGISTER_CLASS(SourceBase, SourceBlast, "Источник взрывной волны");
REGISTER_CLASS(SourceBase, SourceCameraShaking, "Источник тряски камеры");
REGISTER_CLASS(SourceBase, SourceFlash, "Вспышка экрана");
REGISTER_CLASS(SourceBase, SourceDetector, "Детектор");
REGISTER_CLASS(SourceBase, SourceWaterWave, "Источник волны на воде");
REGISTER_CLASS(SourceBase, SourceShield, "Источник защитного поля");
REGISTER_CLASS(SourceBase, SourceTeleport, "Источник телепортатор");
REGISTER_CLASS(SourceBase, SourceDeleteGrass, "Источник удаления травы");
REGISTER_CLASS(SourceBase, SourceFlock, "Истоник - стая на мире");

void SourceIce::quant()
{
	if(environment->temperature() && active()) {
		environment->temperature()->SetT(position().xi(), position().yi(), deltaTemperature_, radius());
	}
	__super::quant();
}

void SourceIce::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(RangedWrapperf(deltaTemperature_, -10.0f, 10.0f, 0.1f), "deltaTemparature", "Разница температуры");
	serializationApply(ar);
}

//---------------------------------------------------------------------------------

void SourceWater::quant()
{
	if(active() && environment->water()) {
		bool is_on_ice=environment->temperature() && environment->temperature()->isOnIce(position());
		if(!is_on_ice)
		{
			environment->water()->AddWaterRect(position().xi(), position().yi(), deltaHeight_, radius());
			effectPause(false);
		}else
		{
			effectPause(true);
		}
	}
	__super::quant();
}

void SourceWater::start()
{
	__super::start();

	if (environment->temperature() && environment->temperature()->isOnIce(position()))
	{
		effectPause(true);
	}
}

void SourceWater::stop()
{
	__super::stop();
}

void SourceWater::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(RangedWrapperf (deltaHeight_, -10.0f, 10.0f, 0.1f), "deltaHeight", "Разница высоты");
	serializationApply(ar);
}

//////////////////////////////////////////////////////////////////////////////
void SourceEffect::serialize(Archive& ar)
{
    __super::serialize(ar);
	bool enableEffect = true;
	if(universe() && universe()->userSave()){ 
		int effectTime = 0;
		if(ar.isOutput())
			effectTime = effectController_.getTime();
		ar.serialize(effectTime, "effectTime", 0);
		if(ar.isInput())
			effectTime_ = effectTime;
	}
	ar.serialize(effectAttribute_, "effectAttribute", "Эффект");
}

void SourceEffect::quant()
{
	__super::quant();
	if(active_ && !effectPause_)
		effectController_.logicQuant();
}

void SourceEffect::showDebug() const
{
	__super::showDebug();
	effectController_.showDebugInfo();
}

void SourceEffect::effectStart()
{
	xassert(enabled() && "Создание эффекта для источника не на мире");
	effectStop();
	if (!effectPause_){
		start_timer_auto();
		effectController_.effectStart(&effectAttribute_);
		effectController_.moveToTime(effectTime_);
	}
}

void SourceEffect::effectStop()
{
	effectController_.release();
}

void SourceEffect::effectPause(bool pause)
{
	if (pause != effectPause_)
	{
		if (pause)
			effectStop();
		else
			effectStart();
		effectPause_ = pause;
	}
}


SourceEffect::SourceEffect () 
: SourceBase()
, effectController_(this)
, effectPause_(false)
, effectTime_(0)
{
	setScanEnvironment(false);
}

SourceEffect::SourceEffect (const SourceEffect& original)
: SourceBase(original)
, effectController_(this)
,effectPause_(original.effectPause_)
, effectTime_(original.effectTime_)
{
	effectAttribute_ = original.effectAttribute_;
	setScanEnvironment(false);
}
SourceEffect::~SourceEffect () {
	effectController_.release();
}


//---------------------------------------------------------------------------------

void SourceDamage::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(damage_, "Damage", "повреждения");
	ar.serialize(abnormalState_, "abnormalState", "воздействие на юниты");
	if(enabled() && ar.isInput())
		setScanEnvironment(active());
}

bool SourceDamage::getParameters(WeaponSourcePrm& prm) const
{
	prm.setDamage(damage_);
	prm.setAbnormalState(abnormalState_);

	return true;
}

bool SourceDamage::setParameters(const WeaponSourcePrm& prm, const WeaponTarget* target)
{
	damage_ = prm.damage();
	abnormalState_ = prm.abnormalState();

	return true;
}

void SourceDamage::showDebug() const
{
	__super::showDebug();

	if(showDebugSource.zoneDamage)
		damage_.showDebug(position(), MAGENTA);

	if(showDebugSource.zoneStateDamage)
		abnormalState_.damage().showDebug(position(), MAGENTA);
}

void SourceDamage::applyDamage(UnitBase* target) const
{
	int time = (lifeTime_ > 0) ? lifeTime_ : MAX_DEFAULT_LIFETIME;

	float dt = (time >= 200) ? (float(logicTimePeriod) / float(time)) : 1.f;
	ParameterSet dmg = damage();
	dmg *= dt;

	target->setDamage(dmg, owner());
}

void SourceDamage::apply(UnitBase* target)
{
	__super::apply(target);

	if(active() && !isUnderEditor()) {
		xassert(!breakWhenApply_);
		applyDamage(target);
		if(abnormalState().isEnabled())
			target->setAbnormalState(abnormalState(), owner());
		return;
	}	
}

//---------------------------------------------------------------------------------

SourceBubble::SourceBubble()
: SourceBase()
, waterBubbleCenter(*new cWaterBubbleCenter)
{
}

SourceBubble::SourceBubble(const SourceBubble& sb)
: SourceBase(sb)
, waterBubbleCenter(*new cWaterBubbleCenter(sb.waterBubbleCenter))
{
}

void SourceBubble::serialize(Archive& ar)
{
    bool wasActive = active();
    __super::serialize(ar);
    
    waterBubbleCenter.serialize(ar);
    if (ar.isInput()) {
        if(enabled() && ar.isEdit() && wasActive != active()){
            active_ = wasActive;
            setActivity(!wasActive);
        }
        waterBubbleCenter.SetPos(Vect2f(pose_.trans().x,pose_.trans().y));
        waterBubbleCenter.SetRadius(radius_);
    }

}

void SourceBubble::start()
{
	__super::start();
	AddCenter();
	waterBubbleCenter.active() = true;
}

void SourceBubble::stop()
{
	__super::stop();
	DeleteCenter();
	waterBubbleCenter.active() = false;
}

void SourceBubble::AddCenter()
{
    //if (environment)
        if(environment->waterBubble())
            environment->waterBubble()->AddCenter(&waterBubbleCenter);
}

void SourceBubble::DeleteCenter()
{
    if (environment)
        if (environment->waterBubble() && waterBubbleCenter.GetParent())
            environment->waterBubble()->DeleteCenter(&waterBubbleCenter);
}

SourceBubble::~SourceBubble()
{
	delete &waterBubbleCenter;
}

void SourceBubble::setPose(const Se3f& pos, bool init)
{
    SourceBase::setPose(pos,init);
    waterBubbleCenter.SetPos(Vect2f(pos.trans().x,pos.trans().y));
}
void SourceBubble::setRadius(float radius)
{
    SourceBase::setRadius(radius);
    waterBubbleCenter.SetRadius(radius);
}

//---------------------------------------------------------------------------------

SourceWaterWave::SourceWaterWave() : SourceBase()
{
    gridSize_ = Vect2i(0,0);
    waveLenght_ = 50;
    curRadius_ =0;
    amplitude_ = 5;
    water_ = NULL;
    speed_ = 1;
    beginRadius_ =  0;
    maxRadius_ = 100;
    fadeRadius_ = 50;
    coordShift_ = 0;
    flatWave_ = false;
    sizeWave_ = 100;
    autoKill_ = false;
}
SourceWaterWave::SourceWaterWave(const SourceWaterWave& sww):
    SourceBase(sww),
    gridSize_(sww.gridSize_),
    waveLenght_(sww.waveLenght_),
    amplitude_(sww.amplitude_),
    water_(sww.water_),
    speed_(sww.speed_),
    coordShift_(sww.coordShift_),
    beginRadius_(sww.beginRadius_),
    maxRadius_(sww.maxRadius_),
    fadeRadius_(sww.fadeRadius_),
    flatWave_(sww.flatWave_),
    sizeWave_(sww.sizeWave_),
    curRadius_(sww.curRadius_),
    autoKill_(sww.autoKill_)
{
}
SourceWaterWave::~SourceWaterWave()
{
}

void SourceWaterWave::serialize(Archive& ar)
{
    __super::serialize(ar);
    ar.serialize(flatWave_, "flatWave", "Прямая волна");
    if (flatWave_)
        ar.serialize(sizeWave_, "sizeWave", "Размер волны");
    ar.serialize(beginRadius_, "beginRadius", "Начальный радиус");
    ar.serialize(maxRadius_, "maxRadius", "Максимальный Радиус");
    ar.serialize(fadeRadius_, "fadeRadius", "Начальный радиус затухания");
    ar.serialize(RangedWrapperi(waveLenght_,32,maxRadius_/2), "waveLenght", "Длинна волны");
    ar.serialize(RangedWrapperf(amplitude_,1,waveLenght_/10,1.0f), "amplitude", "Амплитуда волны");
    ar.serialize(speed_, "speed", "Скорость волны");
    ar.serialize(autoKill_, "autokill", "Автоудаление");
    curRadius_ = beginRadius_;
}

void SourceWaterWave::start()
{
	__super::start();

	if (environment->water())
	{
		water_ = environment->water();
		gridSize_ = Vect2i(water_->GetGridSizeX(),water_->GetGridSizeY());
		coordShift_ = water_->GetCoordShift();
		curRadius_ = beginRadius_;
	}
}

void SourceWaterWave::stop()
{
	__super::stop();
}

void SourceWaterWave::quant()
{
    __super::quant();
    if (!active_&&!water_)
        return;

    float fade = 1;
    if (curRadius_>fadeRadius_)
        fade = 1-(float(curRadius_-fadeRadius_)/float(maxRadius_-fadeRadius_));
    if (flatWave_)
    {
        Vect3f dir(0,1,0);
        orientation().xform(dir);
        Vect3f len = dir%Vect3f::K;
        len.Normalize();
        Vect3f v1 = (dir*curRadius_)-len*sizeWave_+position();
        Vect3f v2 = (dir*(curRadius_+waveLenght_))-len*sizeWave_+position();
        Vect3f v3 = (dir*curRadius_)+len*sizeWave_+position();
        Vect3f v4 = (dir*(curRadius_+waveLenght_))+len*sizeWave_+position();
        float x1 = min(v1.x,min(v2.x,min(v3.x,v4.x)));
        float x2 = max(v1.x,max(v2.x,max(v3.x,v4.x)));
        float y1 = min(v1.y,min(v2.y,min(v3.y,v4.y)));
        float y2 = max(v1.y,max(v2.y,max(v3.y,v4.y)));
        for (int i = round(x1)>>coordShift_; i<round(x2)>>coordShift_; i++)
            for (int j = round(y1)>>coordShift_; j<round(y2)>>coordShift_; j++)
            {
                if (i < 0 || i>=gridSize_.x ||
                    j < 0 || j>=gridSize_.y)
                    continue;
                float x = (i<<coordShift_)-position().x;
                float y = (j<<coordShift_)-position().y;
                Vect2f vec(x,y);
                float len = vec.norm();
                if (len>=curRadius_&& len < curRadius_+waveLenght_)
                {
                    float s= float(len-curRadius_)/float(waveLenght_);
                    //zbuffer_[p.y*gridSize_.x+p.x] += (amplitude_*(1-curRadius_/radius()))*sin(M_PI*s);
                    if (water_->Get(i,j).type == cWater::type_filled)
					{
                        float z = (amplitude_*fade*(-sin(2*M_PI*s)));
						water_->AddWater(i,j,z);
					}
                }
            }
    }else
    {
        int nRadius = ((curRadius_+waveLenght_)>>coordShift_)+1;
        for (int i = 0; i<nRadius*2; i++)
            for (int j = 0; j<nRadius*2; j++)
            {
                int xx = (round(position().x)>>coordShift_);
                int yy = (round(position().y)>>coordShift_);
                Vect2i p(xx-nRadius+i,yy-nRadius+j);
                if (p.x < 0 || p.x>=gridSize_.x ||
                    p.y < 0 || p.y>=gridSize_.y)
                    continue;
                float x = (p.x<<coordShift_)-position().x;
                float y = (p.y<<coordShift_)-position().y;
                Vect2f vec(x,y);
                float len = vec.norm();
                if (len>=curRadius_&& len < curRadius_+waveLenght_)
                {
                    float s= float(len-curRadius_)/float(waveLenght_);
                    //zbuffer_[p.y*gridSize_.x+p.x] += (amplitude_*(1-curRadius_/radius()))*sin(M_PI*s);
                    //if (water_->Get(p.x,p.y).type == cWater::type_filled)
                        water_->AddWater(p.x,p.y,(amplitude_*fade*(-sin(2*M_PI*s))));
                }
            }
    }
    curRadius_+= speed_;
    if (curRadius_>maxRadius_)
    {
        if (autoKill_)
            kill();
        else
            curRadius_ = beginRadius_;
    }
    
    //InterpolationTimer timer_;
    //timer_.start(seconds * 1000);
    //
    //curRadius_ = waveLenght_ + timer_() * (radius() - waveLenght_);
    //if(timer_() >= 1.f - FLT_EPS)
    //	timer_.start(seconds * 1000);

}
