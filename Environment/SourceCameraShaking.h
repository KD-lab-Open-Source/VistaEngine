#ifndef __CAMERA_SHAKING_SOURCE_H__
#define __CAMERA_SHAKING_SOURCE_H__

#include "SourceBase.h"

class SourceCameraShaking : public SourceBase
{
public:
	SourceCameraShaking();
	SourceType type() const { return SOURCE_CAMERA_SHAKING; }
	SourceBase* clone() const { return new SourceCameraShaking(*this); }

	void quant();
	void serialize(Archive& ar);

private:
	float time;
	float factor;
	bool instantaneous;
};

#endif //__CAMERA_SHAKING_SOURCE_H__

