#ifndef __SLOT_MANAGER_H_INCLUDED__
#define __SLOT_MANAGER_H_INCLUDED__
#include <stack>
#include <memory>
using std::stack;
using std::allocator;

/*
	ѕоддерживает кучу слотов.
	≈сли слот уже не нужен, удал€ет его.
*/

template<class Slot>
class cSlotManager
{
public://—делать так, что-бы operator[] мог работать в нескольких потоках при использовании NewSlot,DeleteSlot в другом потоке
	allocator<Slot> alloc;
	typedef vector<Slot*> array;
	typedef typename array::iterator iterator;
protected:
	array all_slot;
	stack<Slot*> free_slot;
public:
	cSlotManager()
	{
	}

	~cSlotManager()
	{
		clear();
	}

	Slot* NewSlot();
	void DeleteSlot(Slot* slot);

	void clear();
	inline int size(){return all_slot.size();};
	inline Slot* operator[](int slot);
};

//ќчищает/устанавливает переменную Slot::init в DeleteSlot/NewSlot
template<class Slot>
class cSlotManagerInit : public cSlotManager<Slot>
{
public:
	inline Slot* NewSlot(){Slot* p=cSlotManager<Slot>::NewSlot(); p->init=1; return p;};
	inline void DeleteSlot(Slot* slot){slot->init=false;cSlotManager<Slot>::DeleteSlot(slot);};
};

//////////implementation////////////////
template<class Slot>
Slot* cSlotManager<Slot>::NewSlot()
{
	if(free_slot.empty())
	{
		Slot* slot=alloc.allocate(sizeof(Slot));
		all_slot.push_back(slot);
		return slot;
	}

	Slot* top=free_slot.top();
	free_slot.pop();
	return top;
}

template<class Slot>
void cSlotManager<Slot>::DeleteSlot(Slot* slot)
{
	free_slot.push(slot);
}

template<class Slot>
Slot* cSlotManager<Slot>::operator[](int slot)
{
	xassert(slot>=0 && slot<all_slot.size());
	return all_slot[slot];
}

template<class Slot>
void cSlotManager<Slot>::clear()
{
	iterator it;
	FOR_EACH(all_slot,it)
	{
		Slot* cur_slot=*it;
		alloc.deallocate(cur_slot,sizeof(Slot));
	}
	all_slot.clear();
	while(!free_slot.empty())
		free_slot.pop();
}

#endif
