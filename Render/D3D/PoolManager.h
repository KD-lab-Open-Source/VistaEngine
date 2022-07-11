#ifndef __POOL_MANAGER_H_INCLUDED__
#define __POOL_MANAGER_H_INCLUDED__

#include "Render\inc\rd.h"

//Классы для хранения и выделения одинаковых по размеру объектов.

struct PoolParameter
{
	virtual bool operator==(const PoolParameter& p) const=0;
	virtual ~PoolParameter() {}
};

struct PoolPage
{
	int pool;
	int page;

	PoolPage(){pool=page=-1;}

	inline int IsInit() { return pool >= 0; }
};

class Pool
{
protected:
	int total_pages;
	int free_pages;
	int *free_pages_list;

	PoolParameter* parameter;
	friend class PoolManager;
public:
	Pool();
	virtual ~Pool();

	int AllocPage();
	void FreePage(int page);
	bool IsEmpty()const{return total_pages==free_pages;};
	bool IsFreePages()const{return free_pages>0;}

	PoolParameter* GetParam()const{return parameter;}

	virtual void Create(const PoolParameter* p)=0;
	virtual void Select(int page)=0;
	virtual void* LockPage(int page)=0;
	virtual void UnlockPage(int page)=0;

	virtual void GetUsedMemory(int& total,int& free)=0;
protected:
	void SetTotal(int total_pages);
};

class RENDER_API PoolManager
{
public:
	PoolManager();
	~PoolManager();

	void CreatePage(PoolPage& new_page,const PoolParameter& param);
	void DeletePage(PoolPage& page);
	void Clear();
	void Select(const PoolPage& page);

	void GetUsedMemory(int& total,int& free);
protected:
	vector<Pool*> pools;
	virtual Pool* NewPool()=0;

	inline void CheckGood(const PoolPage& page)
	{
		xassert(page.pool >=0 && page.pool < int(pools.size()));
		xassert(pools[page.pool]);
		xassert(page.page >= 0 && page.page < pools[page.pool]->total_pages);
	}
};

/////////////////////////vertex/////////////////////
struct VertexPoolParameter : public PoolParameter
{
	int number_vertex;
	IDirect3DVertexDeclaration9* vertex_declaration;

	VertexPoolParameter(const VertexPoolParameter& p)
		:number_vertex(p.number_vertex),vertex_declaration(p.vertex_declaration){}

	VertexPoolParameter(int number_vertex_,	IDirect3DVertexDeclaration9* vertex_declaration_)
		:number_vertex(number_vertex_),vertex_declaration(vertex_declaration_)  {}

	virtual bool operator==(const PoolParameter& p) const
	{
		VertexPoolParameter* vp=(VertexPoolParameter*)&p;
		return number_vertex==vp->number_vertex && vertex_declaration==vp->vertex_declaration;
	}
};

struct VertexPoolPage : public PoolPage
{
};

//#define VERTEXPOOL_CHECKMEM

class VertexPool : public Pool
{
protected:
	enum{VPOOL_MAX_VTX_PER_POOL = 65536/4,};

	IDirect3DVertexBuffer9* vb;
	int page_size;
	IDirect3DVertexDeclaration9* vertex_declaration;
	int vertex_size;
#ifdef VERTEXPOOL_CHECKMEM
	vector<DWORD*> check_array;
#endif 
public:
	VertexPool();
	virtual ~VertexPool();

	virtual void Create(const PoolParameter* p);
	virtual void Select(int page);

	virtual void* LockPage(int page);
	virtual void UnlockPage(int page);

	virtual void GetUsedMemory(int& total,int& free);
	inline int GetPageSize(){return page_size;}
protected:
	virtual void* InternalLockPage(int page);
};

class VertexPoolManager : public PoolManager
{
public:
	void* LockPage(const VertexPoolPage& page);
	void UnlockPage(const VertexPoolPage& page);

	int GetPageSize(const VertexPoolPage& page);
protected:
	virtual Pool* NewPool(){return new VertexPool;}
};


////////////////////////////index/////////////////////////////////
struct IndexPoolParameter : public PoolParameter
{
	int number_index;

	IndexPoolParameter(const IndexPoolParameter& p)
		:number_index(p.number_index){}

	IndexPoolParameter(int number_index_)
		:number_index(number_index_)  {}

	virtual bool operator==(const PoolParameter& p) const
	{
		IndexPoolParameter* vp=(IndexPoolParameter*)&p;
		return number_index==vp->number_index;
	}
};

struct IndexPoolPage : public PoolPage
{
};

class IndexPool : public Pool
{
protected:
	enum{VPOOL_MAX_VTX_PER_POOL = 65536/4,};

	IDirect3DIndexBuffer9* ib;
	int page_size;
#ifdef VERTEXPOOL_CHECKMEM
	vector<DWORD*> check_array;
#endif 
public:
	IndexPool();
	virtual ~IndexPool();

	virtual void Create(const PoolParameter* p);
	virtual void Select(int page);

	virtual void* LockPage(int page);
	virtual void UnlockPage(int page);

	virtual void GetUsedMemory(int& total,int& free);
	inline int GetPageSize(){return page_size;}
};

class IndexPoolManager : public PoolManager
{
public:
	void CreatePage(PoolPage& new_page,int num_index)
	{
		PoolManager::CreatePage(new_page,IndexPoolParameter(num_index));
	}

	sPolygon* LockPage(const IndexPoolPage& page);
	void UnlockPage(const IndexPoolPage& page);

	int GetPageSize(const IndexPoolPage& page);
	int GetBaseIndex(const IndexPoolPage& page);
protected:
	virtual Pool* NewPool(){return new IndexPool;}
};

#endif
