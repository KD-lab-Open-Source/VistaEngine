#ifndef _UNKOBJ_H_
#define _UNKOBJ_H_

const int NUMBER_OBJTEXTURE = 3;

#include "material.h"
#include "frame.h"

class cTexture;

class cUnkObj : public cIUnkObjScale
{ /// базовый класс объектов растеризации
public:
	cUnkObj(int kind);
	virtual ~cUnkObj();
	virtual void Animate(float dt)													{ }
	virtual void PreDraw(cCamera *UClass)											{ }
	virtual void Draw(cCamera *UClass)												{ }

	/// Функции несклько хинтовые, нужно будет тщательно посмотреть, и если нигде не используются - стереть.
	virtual float GetBoundRadius() const {return scale_;}
	virtual void GetBoundBox(sBox6f& Box) const {Box.min.set(-scale_,-scale_,-scale_);Box.max.set(scale_,scale_,scale_);}

	/// инлайновые функции доступа к переменным
	inline const sColor4f& GetDiffuse()	const 										{ return diffuse; }
	inline cTexture* GetTexture(int n=0)											{ return Texture[n]; }
	inline cTexture* GetTexture(int n=0) const										{ return Texture[n]; }
	void SetTexture(int n,cTexture *pTexture);
	void SetDiffuse(const sColor4f& color)
	{
		VISASSERT(color.r>=0 && color.r<100.0f);
		VISASSERT(color.g>=0 && color.g<100.0f);
		VISASSERT(color.b>=0 && color.b<100.0f);
		VISASSERT(color.a>=0 && color.a<2.0f);
		diffuse=color;
	}
private:
	cTexture		*Texture[NUMBER_OBJTEXTURE];
protected:
	sColor4f		diffuse;
};

class cAnimUnkObj : public cUnkObj
{ // базовый класс объектов растеризации.
public:
	cAnimUnkObj(int kind) : cUnkObj(kind)					{ }
	virtual void Animate(float dt)							{ GetFrame()->AddPhase(dt);	}
	virtual void SetAnimation(float Period=2000.f,float StartPhase=0.f,float FinishPhase=-1.f,bool recursive=true) { GetFrame()->Set(Period,StartPhase,FinishPhase); }
	virtual void GetAnimation(float *Period,float *Phase,float *Finish,float *StartPhase) { if(Phase) *Phase=GetFrame()->GetPhase(); if(Period) *Period=GetFrame()->GetPeriod(); if(Finish) *Finish=GetFrame()->GetFinish(); if(StartPhase) *StartPhase=GetFrame()->GetPhase(); }
	inline cFrame* GetFrame()								{ return &Frame; }
private:
	cFrame			Frame;
};

#endif // _UNKOBJ_H_
