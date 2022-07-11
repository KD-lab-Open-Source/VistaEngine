#ifndef _VIDEOHEAP_H_
#define _VIDEOHEAP_H_
/*
	Для Vertex и Index буферов реализация хипа разноразмерных объектов.
	Будем действовать как в dlmalloc - несколько списков свободных блоков,
	по размеру собранных и один список блоков по порядку следования.

*/

class VideoHeapPool;
struct VideoHeapObject
{
	int offset;
	int size;
	VideoHeapPool* pool;
	void* it_block;
	void* user_heap_ptr;

	VideoHeapObject():offset(0),size(0),pool(0),it_block(0),user_heap_ptr(0){}
};

class VideoHeapPool
{
public:
	VideoHeapPool();
	void init(void* user_heap_ptr,int all_size);
	VideoHeapObject malloc(int size);
	void free(const VideoHeapObject& obj);
	int sizeMaxFreeBlock();
protected:
	void* user_heap_ptr;
	int all_size;
	struct Block;
	typedef list<Block> BlockList;
	typedef list<BlockList::iterator> FreeList;
	struct Block
	{
		int offset;
		int size;
		bool free;
		int index_free_block;
		FreeList::iterator it_free;
	};

	struct FreeBlock
	{
		int max_size;
		FreeList free;
	};
	enum {min_size=16};
	
	BlockList block;//Отсортированны по порядку. block[i+1].offset==block[i].offset+block[i].size
	vector<FreeBlock> free_block;

	void addFreeBlock(BlockList::iterator it);
	friend class VideoHeap;
};

class VideoHeap
{
public:
	~VideoHeap();
	VideoHeapObject malloc(int size);
	void free(const VideoHeapObject& obj);
protected:
	virtual void createPoolData(void*& user_heap_ptr,int& size)=0;
	virtual void deletePoolData(void* user_heap_ptr,int size)=0;
	list<VideoHeapPool> pool;
	
};

#endif _VIDEOHEAP_H_