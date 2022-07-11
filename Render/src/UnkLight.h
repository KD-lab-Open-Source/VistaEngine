#ifndef __UNK_LIGHT_H_INCLUDED__
#define __UNK_LIGHT_H_INCLUDED__

#include "UnkObj.h"

enum eBlendMode;

struct sLightKey
{
	Color4c	diffuse;
	float		radius;
};

class cUnkLight : public cUnkObj
{
	Vect3f			Direction;
	float			CurrentTime,TimeLife;
	vector<sLightKey> Key;
	eBlendMode		blendingMode;

public:
	cUnkLight();
	virtual ~cUnkLight();
	
	virtual void Animate(float dt);
	virtual void PreDraw(Camera* camera);
	virtual void Draw(Camera* camera);
	
	void SetDirection(const Vect3f& direction);
	const Vect3f& GetDirection() const { return Direction; }
	
	void SetAnimationPeriod(float Period);
	void SetAnimKeys(sLightKey *AnimKeys,int size);
	
	Vect3f& GetPos()										{ return GlobalMatrix.trans(); }
	
	void SetRadius(float radius){ scale_ = radius; }
	float GetRadius()									{ return scale_; }
	
	eBlendMode& GetBlending() {return blendingMode;}
	void SetBlending(eBlendMode mode) {blendingMode = mode;}
};

#endif
