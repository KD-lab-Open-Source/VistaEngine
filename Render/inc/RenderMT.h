#ifndef __RENDER_MT_H_INCLUDED__
#define __RENDER_MT_H_INCLUDED__

#include "rd.h"

enum
{
	MT_GRAPH_THREAD=1,
	MT_LOGIC_THREAD=2,
};

//Какой из потоков сейчас выполняется.
RENDER_API bool MT_IS_GRAPH();
RENDER_API bool MT_IS_LOGIC();
RENDER_API void MT_SET_TLS(unsigned int t);
#define MTG() xassert(MT_IS_GRAPH())
#define MTL() xassert(MT_IS_LOGIC())

/*
	Отключает на области видимости assert`ы связанные с многопоточностью.
	Дебаговый класс, пользоваться только из под локов, останавливающих другой поток.
*/
class RENDER_API MTAutoSkipAssert
{
	unsigned int real_tls;
public:
	MTAutoSkipAssert();
	~MTAutoSkipAssert();
};

#endif
