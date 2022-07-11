#include "stdafx.h"
#include "SourceCameraShaking.h"
#include "CameraManager.h"

SourceCameraShaking::SourceCameraShaking()
{
	time = 30;
	factor = 1;
	instantaneous = true;
}

void SourceCameraShaking::serialize(Archive& ar)
{
	SourceBase::serialize(ar);

	ar.serialize(time, "time", "Время тряски камеры");
	ar.serialize(factor, "factor", "Сила тряски камеры");
	ar.serialize(instantaneous, "instantaneous", "Мгновенный источник");
}

void SourceCameraShaking::quant()
{
	SourceBase::quant();

	if(active()) {

		if(cameraManager->eyePosition().distance2(position()) < sqr(radius())){
			Vect3f cameraDir = cameraManager->eyePosition() - position();
			cameraDir.Normalize();
			cameraManager->startOscillation(round(time*1000), factor, cameraDir);
		}
			
		if(instantaneous && !isUnderEditor())
			kill();
	}
}
