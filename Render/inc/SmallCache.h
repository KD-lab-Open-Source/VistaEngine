#ifndef _SMALL_CACHE_
#define _SMALL_CACHE_

//Интрузивный список без типизации.
struct VoidIntrusiveListNode
{
	VoidIntrusiveListNode *prev,*next;
};

template<class T, int offset>//Передавать offsetof(T,node_name)
class VoidIntrusiveList
{
public:
	typedef VoidIntrusiveListNode Node;
protected:
	Node* root;
public:
	VoidIntrusiveList()
		:root((Node*)(void*)offset)//Чтобы begin,next возвращала 0
	{
	}

	void push_front(T* object)
	{
		Node* cur=(Node*)(offset+(char*)object);
		if(root!=(Node*)(void*)offset)
			root->prev=cur;
		cur->prev=(Node*)(void*)offset;
		cur->next=root;
		root=cur;
	}

	void remove(T* object)
	{
		Node* cur=(Node*)(offset+(char*)object);
		if(cur->next!=(Node*)(void*)offset)
			cur->next->prev=cur->prev;

		if(cur->prev!=(Node*)(void*)offset)
			cur->prev->next=cur->next;
		else
			root=cur->next;
	}

	bool empty(){return root==(Node*)(void*)offset;};
	T* begin(){return (T*)(-offset+(char*)root);}
	T* next(void* object)
	{
		Node* cur=(Node*)(offset+(char*)object);
		return (T*)(-offset+(char*)cur->next);
	}
};

template<class T, int offset>
class VoidIntrusiveListCycled
{
public:
	typedef VoidIntrusiveListNode Node;
protected:
	Node* root;
public:
	VoidIntrusiveListCycled()
		:root((Node*)(void*)offset)
	{
	}

	void push_front(void* object)
	{
		Node* cur=(Node*)(offset+(char*)object);
		if(root==(Node*)(void*)offset)
		{
			root=cur->next=cur->prev=cur;
			return;
		}

		cur->prev=root->prev;
		root->prev->next=cur;
		cur->next=root;
		root->prev=cur;

		root=cur;
	}

	T* move_root_back()
	{
		root=root->prev;
		return begin();
	}
	void remove(T* object)
	{
		Node* cur=(Node*)(offset+(char*)object);
		if(cur->next==cur)
		{
			xassert(cur->prev==cur);
			root=(Node*)(void*)offset;
			return;
		}

		if(root==cur)
			root=cur->next;

		cur->prev->next=cur->next;
		cur->next->prev=cur->prev;
	}

	bool empty(){return root==(Node*)(void*)offset;};
	T* begin(){return (T*)(-offset+(char*)root);}
	T* next(T* object)
	{
		Node* cur=(Node*)(offset+(char*)object);
		return (T*)(-offset+(char*)cur->next);
	}
};

//Основная идея - использовать цикличесмкие списки.
//Хранение - циклический список пустых ячеек.
//Кэширование - циклический список для этого числа в кэше.
//Удаление - циклический список всех объектов по времени доступа.
//SHIFT - количество элементов в кеше.
template<class T,int SHIFT=4>
class SmallCache
{
	enum {
		KEY_SIZE=1<<(SHIFT+1),
		KEY_AND=KEY_SIZE-1,
		DATA_SIZE=1<<SHIFT,
	};

	struct Data
	{
		VoidIntrusiveListNode use_node;
		VoidIntrusiveListNode key_node;
		int key;
		T data;
	};

	typedef VoidIntrusiveList<Data,offsetof(Data,use_node)> VoidIntrusiveListUse;
	typedef VoidIntrusiveList<Data,offsetof(Data,key_node)> VoidIntrusiveListKey;
	VoidIntrusiveListUse empty_list;
	VoidIntrusiveListCycled<Data,offsetof(Data,use_node)> used_list;
	VoidIntrusiveListKey key_list[KEY_SIZE];
	Data data_array[DATA_SIZE];
public:
	SmallCache()
	{
		for(int i=0;i<DATA_SIZE;++i)
			empty_list.push_front(&data_array[i]);
	}

	T* get(int key)
	{
		VoidIntrusiveListKey& kl=key_list[key&KEY_AND];
		for(Data* p=kl.begin();p;p=kl.next(p))
		{
			if(p->key==key)
			{
				used_list.remove(p);
				used_list.push_front(p);
				return &p->data;
			}
		}

		return NULL;
	}

	T* add(int key,T& value)
	{
		int key_and=key&KEY_AND;
		Data* data;
		if(!empty_list.empty())
		{
			data=empty_list.begin();
			empty_list.remove(data);
			used_list.push_front(data);
		}else
		{
			data=used_list.move_root_back();
			key_list[data->key&KEY_AND].remove(data);
		}

		data->key=key;
		data->data=value;
		key_list[key_and].push_front(data);
		return &data->data;
	}
};

//Очень маленький кэш для 3х элементов. Стереть потом.
template<class T>
class SmallCache3
{
	struct Data
	{
		int key;
		T data;
		int time;
	};

	Data data_array[3];
	int time;
public:
	SmallCache3()
	{
		time=0;
		for(int i=0;i<3;++i)
		{
			Data& d=data_array[i];
			d.key=INT_MAX;
			d.time=-1;
		}
	}

	T* get(int key)
	{
		for(int i=0;i<3;i++)
		{
			Data& d=data_array[i];
			if(d.key==key)
			{
				d.time=time++;
				return &d.data;
			}
		}

		return NULL;
	}

	T* add(int key,T& value)
	{
		int min_time=data_array[0].time;
		int min_time_index=0;
		for(int i=1;i<3;i++)
		{
			Data& d=data_array[i];
			if(d.time<min_time)
			{
				min_time_index=i;
				min_time=d.time;
			}
		}

		Data& data=data_array[min_time_index];
		data.key=key;
		data.data=value;
		data.time=time++;
		return &data.data;
	}
};

#endif  _SMALL_CACHE_
