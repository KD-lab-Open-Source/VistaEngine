#ifndef __RENDER_MT_H_INCLUDED__
#define __RENDER_MT_H_INCLUDED__

extern __declspec( thread ) DWORD tls_is_graph;//!!! Ќе выставл€ть где попало!!!!!!!
enum
{
	MT_GRAPH_THREAD=1,
	MT_LOGIC_THREAD=2,
};

// акой из потоков сейчас выполн€етс€.
#define MTG() xassert(tls_is_graph&MT_GRAPH_THREAD)
#define MTL() xassert(tls_is_graph&MT_LOGIC_THREAD)
#define MT_IS_GRAPH()  (tls_is_graph&MT_GRAPH_THREAD)
#define MT_IS_LOGIC()  (tls_is_graph&MT_LOGIC_THREAD)

/*
	ќтключает на области видимости assert`ы св€занные с многопоточностью.
	ƒебаговый класс, пользоватьс€ только из под локов, останавливающих другой поток.
*/
class MTAutoSkipAssert
{
	DWORD real_tls;
public:
	MTAutoSkipAssert()
	{
		real_tls=tls_is_graph;
		tls_is_graph=MT_GRAPH_THREAD|MT_LOGIC_THREAD;
	}

	~MTAutoSkipAssert()
	{
		tls_is_graph=real_tls;
	}
};

#endif
