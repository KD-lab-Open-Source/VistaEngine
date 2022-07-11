#ifndef __UNK_LIGHT_H_INCLUDED__
#define __UNK_LIGHT_H_INCLUDED__
enum eBlendMode;
struct sLightKey
{
	sColor4c	diffuse;
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
	// общие интерфейсные функции унаследованы от cUnkObj
	virtual void Animate(float dt);
	virtual void PreDraw(cCamera *UCamera);
	virtual void Draw(cCamera *UCamera);
	virtual const MatXf& GetPosition()const; 
	virtual void SetPosition(const MatXf& Matrix);
	virtual void SetDirection(const Vect3f& direction);
	virtual const Vect3f& GetDirection() const;
	virtual void SetAnimation(float Period=2000.f,float StartPhase=0.f,float FinishPhase=-1.f,bool recursive=true);
	virtual void SetAnimKeys(sLightKey *AnimKeys,int size);
	// интерфейсные функции cUnkLight
	void SetRadius(float radius){scale_=radius;}

	inline Vect3f& GetPos()										{ return GlobalMatrix.trans(); }
	inline float& GetRadius()									{ return scale_; }
	inline float& GetRealRadius()								{ return scale_; }
	inline float& GetPeriod(){return TimeLife;}
	inline eBlendMode& GetBlending() {return blendingMode;}
	void SetBlending(eBlendMode mode) {blendingMode = mode;}
};

#endif
