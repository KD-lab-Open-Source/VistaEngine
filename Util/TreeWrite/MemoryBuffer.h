#pragma once
//base , free
//Предполагается 2 применения.
//1 - для записи файлов с разметкой.
//2 - для записи структур с внутренними ссылками, и последующего их восстановления.

class MemoryBuffer
{
public:
	inline MemoryBuffer();
	inline ~MemoryBuffer();

	inline size_t size(){return size_;};
	inline void write(const void* data,size_t data_size);
	inline size_t tell(){return cur_offset_;};//Текущее положение куда пишем.
	inline bool seek(size_t pos);//Переместить указатель записи (pos<size())

	inline void* data();//Дорогая функция, собирает данные во flat массив, лучше пользоваться num_block,get_block_ptr
	inline int num_block();
	inline bool get_block_ptr(size_t num,void*& ptr,size_t& size);//num<num_block()

	inline void align(int align_size=4);//после этого size()%align_size==0, а ненужные байтики будут нулями заполнены.

	inline void copy(MemoryBuffer& from,size_t from_offset,size_t from_size);
protected:
	struct buffer
	{
		size_t size;
		size_t offset;
		char* ptr;
	};

	struct less
	{
		bool operator ()(buffer& b,size_t second_offset)
		{
			if(b.offset+b.size<=second_offset)
				return true;//Меньше
			return false;
		}
	};

	std::vector<buffer> buffers;
	size_t size_;
	int cur_buffer_;
	size_t cur_offset_;

	inline void next_buffer();
	inline void free_all_buffers();
};

MemoryBuffer::MemoryBuffer()
:size_(0),cur_buffer_(-1),cur_offset_(0)
{
}

MemoryBuffer::~MemoryBuffer()
{
	free_all_buffers();
}

void MemoryBuffer::free_all_buffers()
{
	for(std::vector<buffer>::iterator it=buffers.begin();it!=buffers.end();it++)
		free(it->ptr);
	buffers.clear();
}

int MemoryBuffer::num_block()
{
	return (int)buffers.size();
}

bool MemoryBuffer::get_block_ptr(size_t num,void*& ptr,size_t& size)
{
	if(num>buffers.size())
	{
		ptr=0;
		size=0;
		return false;
	}

	ptr=buffers[num].ptr;

	if(num==buffers.size()-1)
	{
		size=size_-buffers.back().offset;
	}else
	{
		size=buffers[num].size;
	}

	return true;
}

void MemoryBuffer::write(const void* data,size_t data_size)
{
	if(data_size==0)
		return;
	if(buffers.empty())
		next_buffer();

	const char* cdata=(const char*)data;
	size_t cdata_size=data_size;

	while(cdata_size)
	{
		buffer& p=buffers[cur_buffer_];
		size_t end_size=p.size+p.offset-cur_offset_;
		size_t copy_size=min(end_size,cdata_size);
		memcpy(p.ptr+cur_offset_-p.offset,cdata,copy_size);

		if(end_size<cdata_size)
			next_buffer();

		cdata+=copy_size;
		cdata_size-=copy_size;
		cur_offset_+=copy_size;
	}

	if(size_<cur_offset_)
		size_=cur_offset_;
}

void MemoryBuffer::next_buffer()
{
	cur_buffer_++;
	if(cur_buffer_!=(int)buffers.size())
		return;

	buffer p;
	p.size=65536;
	p.ptr=(char*)malloc(p.size);
	if(buffers.empty())
		p.offset=0;
	else
		p.offset=buffers.back().offset+buffers.back().size;
	buffers.push_back(p);
}


void* MemoryBuffer::data()
{
	buffer p;
	p.size=size();
	p.offset=0;
	p.ptr=(char*)malloc(p.size);
	size_t offset=0;
	for(int i=0;i<num_block();i++)
	{
		void* ptr; size_t size;
		get_block_ptr(i,ptr,size);
		memcpy(offset+p.ptr,ptr,size);
		offset+=size;
	}
	free_all_buffers();
	buffers.push_back(p);
	size_=p.size;
	cur_buffer_=0;
	return p.ptr;
}

void MemoryBuffer::align(int align_size)
{
	size_t num_write=align_size-cur_offset_%align_size;
	int data=0;
	while(num_write)
	{
		size_t real_write=min((size_t)sizeof(data),num_write);
		write(&data,real_write);
		num_write-=real_write;
	}
}

bool MemoryBuffer::seek(size_t pos)
{
	if(pos>size_)
		return false;
	if(buffers.empty())
	{
		cur_buffer_=0;
		cur_offset_=0;
		return true;
	}

	std::vector<buffer>::iterator it;
	if(pos==size_)
	{
		it=buffers.end();
		it--;
	}else
		it=lower_bound(buffers.begin(),buffers.end(),pos,less());
	xassert(it!=buffers.end());
	xassert(it->offset<=pos && it->offset+it->size>=pos);
	cur_buffer_=int(it-buffers.begin());
	cur_offset_=pos;
	if(it->offset+it->size==pos)
		xassert(cur_buffer_==buffers.size()-1);

	return true;
}

void MemoryBuffer::copy(MemoryBuffer& from,size_t from_offset,size_t from_size)
{
	xassert(from_offset+from_size<=from.size());
	std::vector<buffer>::iterator it=lower_bound(from.buffers.begin(),from.buffers.end(),from_offset,less());
	xassert(it!=from.buffers.end());
	xassert(it->offset<=from_offset && it->offset+it->size>=from_offset);

	//Ещё не проверили на несколько циклов
	while(from_size && it!=from.buffers.end())
	{
		buffer& p=*it;
		size_t end_size=p.size+p.offset-from_offset;
		size_t copy_size=min(end_size,from_size);
		write(p.ptr+from_offset-p.offset,copy_size);

		from_offset+=copy_size;
		from_size-=copy_size;
		it++;

	}

	xassert(from_size==0);
}
