#include "StdAfxRD.h"
#include "VideoHeap.h"


VideoHeapPool::VideoHeapPool()
{
	xassert(sizeof(BlockList::iterator)==sizeof(void*));
}

void VideoHeapPool::init(void* user_heap_ptr_,int all_size_)
{
	user_heap_ptr=user_heap_ptr_;
	all_size=all_size_;
	
	int num_free_list=1;
	while(((all_size_/min_size)>>num_free_list)>2)
		num_free_list++;

	free_block.resize(num_free_list);
	for(int i=0;i<num_free_list;i++)
		free_block[i].max_size=min_size<<i;

	Block b;
	b.offset=0;
	b.size=all_size;
	b.free=false;
	block.push_back(b);
	addFreeBlock(--block.end());
}

void VideoHeapPool::addFreeBlock(BlockList::iterator it)
{
	it->free=true;
	for(int i=0,size=free_block.size();i<size;i++)
	{
		if(it->size<=free_block[i].max_size)
		{
			it->index_free_block=i;
			free_block[i].free.push_back(it);
			return;
		}
	}

	free_block.back().free.push_back(it);
}

VideoHeapObject VideoHeapPool::malloc(int block_size)
{
	for(int iblock=0,size=free_block.size();iblock<size;iblock++)
	{
		FreeBlock& f=free_block[iblock];
		if(block_size<=f.max_size)
		{
			for(FreeList::iterator it=f.free.begin();it!=f.free.end();++it)
			{
				BlockList::iterator itb=*it;
				Block& fb=*itb;
				if(fb.size>=block_size)
				{
					xassert(fb.free);
					xassert(iblock==fb.index_free_block);
					f.free.erase(it);

					if(fb.size>block_size)//Потом написать чтобы минимальный кусок искала в который блок может поместиться.
					{
						//Разбиваем на 2 блока.
						Block free_block;
						free_block.offset=fb.offset+fb.size;
						free_block.size=fb.size-block_size;
						free_block.free=true;
						fb.size=block_size;
						BlockList::iterator itb_free=itb;
						++itb_free;
						addFreeBlock(block.insert(itb_free,free_block));

					}

					fb.free=false;
					VideoHeapObject good;
					good.offset=fb.offset;
					good.size=fb.size;
					good.pool=this;
					good.it_block=*(void**)&itb;
					return good;
				}
			}
		}
	}
	VideoHeapObject empty;
	return empty;
}

void VideoHeapPool::free(const VideoHeapObject& obj)
{
	BlockList::iterator it=*(BlockList::iterator*)(&obj.it_block);
	xassert(!it->free);
	//merge free block
	if(it!=block.begin())
	{
		BlockList::iterator it_prev=it;
		--it_prev;
		if(it_prev->free)
		{
			xassert(it_prev->offset+it_prev->size==it->offset);
			it->offset=it_prev->offset;
			it->size+=it_prev->size;
			free_block[it_prev->index_free_block].free.erase(it_prev->it_free);
			block.erase(it_prev);
		}
	}

	if(it!=block.end())
	{
		BlockList::iterator it_next=it;
		++it_next;
		if(it_next->free)
		{
			xassert(it->offset+it->size==it_next->offset);
			it->size+=it_next->size;
			free_block[it_next->index_free_block].free.erase(it_next->it_free);
			block.erase(it_next);
		}
	
	}

	addFreeBlock(it);
}


VideoHeapObject VideoHeap::malloc(int size)
{
	VideoHeapObject h;
	if(size==0)
		return h;
	//Потом сделать оптимизацию, чтобы из последнего пула выделяло, из которого предыдущий выделился.
	list<VideoHeapPool>::iterator it;
	FOR_EACH(pool,it)
	{
		h=it->malloc(size);
		if(h.size)
			return h;
	}

	void* user_heap_ptr;
	int heap_size;
	createPoolData(user_heap_ptr,heap_size);
	VideoHeapPool p;
	p.init(user_heap_ptr,heap_size);
	pool.push_back(p);
	return pool.back().malloc(size);
}

void VideoHeap::free(const VideoHeapObject& obj)
{
	obj.pool->free(obj);
}

VideoHeap::~VideoHeap()
{
/*
	list<VideoHeapPool>::iterator it;
	FOR_EACH(pool,it)
	{
		deletePoolData(it->user_heap_ptr,it->all_size);
	}
*/
}
