#ifndef __I_UNK_OBJ_H_INCLUDED__
#define __I_UNK_OBJ_H_INCLUDED__

#include "IVisGenericInternal.h"

inline void cBaseGraphObject::MTAccess()
{
	xassert(MT_IS_GRAPH() || !GetAttr(ATTRUNKOBJ_ATTACHED));
}

class cIUnkObj : public cBaseGraphObject
{ // базовый класс объектов
public:
	cIUnkObj(int kind);
	virtual ~cIUnkObj();

	virtual void Animate(float dt)=0;
	virtual void PreDraw(cCamera *UClass)=0;
	virtual void Draw(cCamera *UClass)=0;
	virtual void SetPosition(const MatXf& Matrix);
	virtual void SetPosition(const Se3f& pos){MatXf m(pos);SetPosition(m);}
	virtual const MatXf& GetPosition() const						{ return GlobalMatrix; }

	// инлайновые функции доступа к переменным
	inline const MatXf& GetGlobalMatrix() const						{ return GlobalMatrix; }

	virtual Vect3f GetCenterObject()								{return GetGlobalMatrix().trans();}

protected:
	// глобальная матрица объекта, относительно мировых координат
	MatXf			GlobalMatrix;	

};

class cIUnkObjScale : public cIUnkObj
{
public:
	cIUnkObjScale(int kind);
	virtual ~cIUnkObjScale();

	virtual const float GetScale() const 			{ return scale_; }
	virtual void SetScale(const float scale);
protected:
	float	scale_;	// масштаб
};


#endif
