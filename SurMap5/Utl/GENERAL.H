
#pragma warning( disable : 4554 )

#define _video_page	VS(_video)->_video

#define ALLOC(b,c)	new c[b]
#define FREE(p) 	delete p;

#include "xmath.h"
typedef Vect3f DBV;
typedef Vect3f Vector;
typedef Mat3f DBM;
typedef Mat3f Matrix;
#include "3dgraph.h"
#include "3dobject.h"
#include "heap.h"


extern MemoryHeap load_heap;
#define HEAP_BEGIN(sz)	    load_heap.alloc(sz);
#define HEAP_ALLOC(b,c)     ((c*)(load_heap.get((b)*sizeof(c))))
#define HEAP_ALLOC1(c)	    ((c*)(load_heap.get(sizeof(c))))
#define HEAP_END	    load_heap.end_alloc();


extern XBuffer msg_buf;




