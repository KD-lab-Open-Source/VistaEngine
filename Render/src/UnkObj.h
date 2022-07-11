#ifndef _UNKOBJ_H_
#define _UNKOBJ_H_

#include "Render\inc\IUnkObj.h"

class cTexture;

class RENDER_API cUnkObj : public cIUnkObj
{ /// базовый класс объектов растеризации
public:
	cUnkObj(int kind);
	virtual ~cUnkObj();

	const float GetScale() const 			{ return scale_; }
	void SetScale(const float scale);

	/// Функции несклько хинтовые, нужно будет тщательно посмотреть, и если нигде не используются - стереть.
	virtual float GetBoundRadius() const {return scale_;}
	virtual void GetBoundBox(sBox6f& Box) const {Box.min.set(-scale_,-scale_,-scale_);Box.max.set(scale_,scale_,scale_);}

	const Color4f& GetDiffuse()	const 										{ return diffuse; }
	void SetDiffuse(const Color4f& color)
	{
		xassert(color.r>=0 && color.r<100.0f);
		xassert(color.g>=0 && color.g<100.0f);
		xassert(color.b>=0 && color.b<100.0f);
		xassert(color.a>=0 && color.a<2.0f);
		diffuse=color;
	}

	cTexture* GetTexture(int n=0)											{ return Texture[n]; }
	void SetTexture(int n,cTexture *pTexture);

private:
	enum { NUMBER_OBJTEXTURE = 3 };
	cTexture* Texture[NUMBER_OBJTEXTURE];

protected:
	float	scale_;	
	Color4f	diffuse;
};

#endif // _UNKOBJ_H_
