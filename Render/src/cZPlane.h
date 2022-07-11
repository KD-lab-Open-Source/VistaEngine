#ifndef _CZPLANE_H_
#define _CZPLANE_H_

class cPlane:public cUnkObj
{
	float umin,vmin,umax,vmax;
	Vect3f size;
public:
	cPlane();
	/*  Что можно применять
		SetTexture(0,...)
		SetAttr ATTRUNKOBJ_IGNORE
		SetScale
		SetPosition
	*/
	virtual void SetUV(float umin,float vmin,float umax,float vmax);
	virtual void PreDraw(cCamera *UClass);
	virtual void Draw(cCamera *UClass);

	virtual Vect3f GetCenterObject();

	void SetSize(const Vect3f& size_){size=size_;};
};

#endif  _CZPLANE_H_
