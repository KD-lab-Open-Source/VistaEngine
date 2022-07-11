#ifndef __I_UNK_OBJ_H_INCLUDED__
#define __I_UNK_OBJ_H_INCLUDED__

#include "IVisGenericInternal.h"
#include "VisGenericDefine.h"

inline void BaseGraphObject::MTAccess()
{
	xassert(MT_IS_GRAPH() || !getAttribute(ATTRUNKOBJ_ATTACHED));
}

class cIUnkObj : public BaseGraphObject
{ // базовый класс объектов
public:
	cIUnkObj(int kind);
	virtual ~cIUnkObj();

	void SetPosition(const MatXf& Matrix);
	void SetPosition(const Se3f& pos){MatXf m(pos);SetPosition(m);}
	const MatXf& GetPosition() const { return GlobalMatrix; }

	// инлайновые функции доступа к переменным
	const MatXf& GetGlobalMatrix() const						{ return GlobalMatrix; }

protected:
	// глобальная матрица объекта, относительно мировых координат
	MatXf			GlobalMatrix;	

};

#endif
