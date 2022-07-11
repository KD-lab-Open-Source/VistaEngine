#include "StdAfxRD.h"
#include "PoolManager.h"

Pool::Pool()
{
	total_pages=free_pages=0;
	free_pages_list=NULL;
	parameter=NULL;
}

Pool::~Pool()
{
	delete[] free_pages_list;
	delete parameter;
}

int Pool::AllocPage()
{
	VISASSERT(free_pages);
	if (!free_pages) return -1;
	free_pages--;
	return free_pages_list[free_pages];
}

void Pool::FreePage(int page)
{
	VISASSERT(page >= 0 && page < total_pages && free_pages < total_pages);
	free_pages_list[free_pages] = page;
	free_pages++;
}

void Pool::SetTotal(int total_pages_)
{
	free_pages = total_pages = total_pages_;
	free_pages_list=new int[total_pages];
	for (int i = 0; i < total_pages; i++)
		free_pages_list[i] = i;
}


/////////////////////////PoolManager///////////////////////

PoolManager::PoolManager()
{
}

PoolManager::~PoolManager()
{
}

void PoolManager::CreatePage(PoolPage& page,const PoolParameter& param)
{
	int i;
	for(i=0;i<pools.size();i++)
	if(pools[i])
	{
		if(param==*pools[i]->GetParam() && pools[i]->IsFreePages())
			break;
	}

	if(i>=pools.size())
	{
		for (i=0;i<pools.size();i++)
		if(pools[i]==NULL)
				break;
		if (i>=pools.size())
		{
			i = pools.size();
			pools.push_back(NULL);
		}
		pools[i] = NewPool();
		pools[i]->Create(&param);
	}

	page.pool = i;
	page.page = pools[i]->AllocPage();
}

void PoolManager::DeletePage(PoolPage& page)
{
	if(page.pool==-1)
		return;
	CheckGood(page);
	pools[page.pool]->FreePage(page.page);
	page.pool=-1;
	page.page=-1;
}

void PoolManager::Clear()
{
	vector<Pool*>::iterator it;
	FOR_EACH(pools,it)
		delete *it;

	pools.clear();
}

void PoolManager::Select(const PoolPage& page)
{
	CheckGood(page);
	pools[page.pool]->Select(page.page);
}

void PoolManager::GetUsedMemory(int& total,int& free)
{
	total=free=0;

	vector<Pool*>::iterator it;
	FOR_EACH(pools,it)
	{
		int cur_total,cur_free;
		(*it)->GetUsedMemory(cur_total,cur_free);
		total+=cur_total;
		free+=cur_free;
	}
}

////////////////////////////vertex//////////////////////
VertexPool::VertexPool()
{
	vb=NULL;
	page_size=0;
	vertex_declaration=0;
	vertex_size=0;
}

VertexPool::~VertexPool()
{
	RELEASE(vb);
}

void VertexPool::Create(const PoolParameter* p)
{
	VertexPoolParameter* vp=(VertexPoolParameter*)p;
	parameter=new VertexPoolParameter(*vp);

	page_size = vp->number_vertex;
	vertex_declaration = vp->vertex_declaration;
	vertex_size = gb_RenderDevice3D->GetSizeFromDeclaration(vertex_declaration);
	SetTotal(VPOOL_MAX_VTX_PER_POOL / page_size);

	RDCALL(gb_RenderDevice3D->lpD3DDevice->CreateVertexBuffer(
		total_pages * page_size * vertex_size,
		//D3DUSAGE_DYNAMIC | 
		D3DUSAGE_WRITEONLY,
		0,
		D3DPOOL_DEFAULT,
		&vb,NULL ) );
#ifdef VERTEXPOOL_CHECKMEM
	check_array.resize(total_pages,0);
#endif
}

void VertexPool::Select(int page)
{
	RDCALL(gb_RenderDevice3D->lpD3DDevice->SetStreamSource(0, vb, 0, vertex_size));
}

void* VertexPoolManager::LockPage(const VertexPoolPage& page)
{
	CheckGood(page);
	return pools[page.pool]->LockPage(page.page);
}

void VertexPoolManager::UnlockPage(const VertexPoolPage& page)
{
	CheckGood(page);
	pools[page.pool]->UnlockPage(page.page);
}

#ifdef VERTEXPOOL_CHECKMEM
inline int VERTEXPOOL_DWORDSIZE(int x) {return (x+3)/4;}
static DWORD vertex_mask=0x7f865431L;
const vertex_additional=64;
#endif

void* VertexPool::InternalLockPage(int page)
{
	VISASSERT(page >= 0 && page < total_pages);
	void *ptr;
	RDCALL(vb->Lock(
		vertex_size * page_size * page, // offset, bytes
		vertex_size * page_size, // size, bytes
		&ptr,
		D3DLOCK_NOSYSLOCK
	));
	return ptr;
}

void* VertexPool::LockPage(int page)
{
	VISASSERT(page >= 0 && page < total_pages);
#ifdef VERTEXPOOL_CHECKMEM
	int size=VERTEXPOOL_DWORDSIZE(vertex_size*page_size);
	xassert(check_array[page]==NULL);
	DWORD* ptr=new DWORD[size+vertex_additional];
	check_array[page]=ptr;
	for(int i=0;i<size+vertex_additional;i++)
		ptr[i]=vertex_mask;
	return ptr;
#else
	return InternalLockPage(page);
#endif
}

void VertexPool::UnlockPage(int page)
{
	VISASSERT(page >= 0 && page < total_pages);
#ifdef VERTEXPOOL_CHECKMEM
	void *ptr_out=InternalLockPage(page);
	DWORD* data=check_array[page];
	int size=VERTEXPOOL_DWORDSIZE(vertex_size*page_size);
	for(int i=0;i<size;i++)
		xassert(data[i]!=vertex_mask);
	for(int i=size;i<size+vertex_additional;i++)
		xassert(data[i]==vertex_mask);

	memcpy(ptr_out,data,vertex_size*page_size);
	delete data;
	check_array[page]=NULL;
#endif
	RDCALL(vb->Unlock());
}

void VertexPool::GetUsedMemory(int& total,int& free)
{
	total=page_size*total_pages*vertex_size;
	free=page_size*free_pages*vertex_size;
}


int VertexPoolManager::GetPageSize(const VertexPoolPage& page)
{
	CheckGood(page);
	return ((VertexPool*)pools[page.pool])->GetPageSize();
}


//////////////////////////////index///////////////
IndexPool::IndexPool()
{
	ib=NULL;
	page_size=0;
}

IndexPool::~IndexPool()
{
	RELEASE(ib);
}

void IndexPool::Create(const PoolParameter* p)
{
	IndexPoolParameter* vp=(IndexPoolParameter*)p;
	parameter=new IndexPoolParameter(*vp);

	page_size = vp->number_index;
	SetTotal(VPOOL_MAX_VTX_PER_POOL / page_size);

	RDCALL(gb_RenderDevice3D->lpD3DDevice->CreateIndexBuffer(
		total_pages * page_size * sizeof(WORD),
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_DEFAULT,
		&ib,NULL
	));

#ifdef VERTEXPOOL_CHECKMEM
	check_array.resize(total_pages,0);
#endif 
}

void IndexPool::Select(int page)
{
	gb_RenderDevice3D->SetIndices(ib);
}

sPolygon* IndexPoolManager::LockPage(const IndexPoolPage& page)
{
	CheckGood(page);
	return (sPolygon*)pools[page.pool]->LockPage(page.page);
}

void IndexPoolManager::UnlockPage(const IndexPoolPage& page)
{
	CheckGood(page);
	pools[page.pool]->UnlockPage(page.page);
}

void* IndexPool::LockPage(int page)
{
#ifdef VERTEXPOOL_CHECKMEM
	int size=VERTEXPOOL_DWORDSIZE(sizeof(WORD)* page_size);
	xassert(check_array[page]==NULL);
	DWORD* ptr=new DWORD[size+vertex_additional];
	check_array[page]=ptr;
	for(int i=0;i<size+vertex_additional;i++)
		ptr[i]=vertex_mask;
	return ptr;
#else
	void *ptr;
	VISASSERT(page >= 0 && page < total_pages);

	RDCALL(ib->Lock(
		sizeof(WORD)* page_size * page, // offset, bytes
		sizeof(WORD)* page_size, // size, bytes
		&ptr,
		D3DLOCK_NOSYSLOCK
	));
	return ptr;
#endif
}

void IndexPool::UnlockPage(int page)
{
	VISASSERT(page >= 0 && page < total_pages);
#ifdef VERTEXPOOL_CHECKMEM
	void *ptr_out;
	RDCALL(ib->Lock(
		sizeof(WORD)* page_size * page, // offset, bytes
		sizeof(WORD)* page_size, // size, bytes
		&ptr_out,
		D3DLOCK_NOSYSLOCK
	));

	DWORD* data=check_array[page];
	int size=VERTEXPOOL_DWORDSIZE(sizeof(WORD)* page_size);
	for(int i=0;i<size;i++)
		xassert(data[i]!=vertex_mask);
	for(int i=size;i<size+vertex_additional;i++)
		xassert(data[i]==vertex_mask);

	memcpy(ptr_out,data,sizeof(WORD)* page_size);
	delete data;
	check_array[page]=NULL;
#endif
	RDCALL(ib->Unlock());
}

void IndexPool::GetUsedMemory(int& total,int& free)
{
	total=page_size*total_pages*sizeof(WORD);
	free=page_size*free_pages*sizeof(WORD);
}


int IndexPoolManager::GetPageSize(const IndexPoolPage& page)
{
	CheckGood(page);
	return ((IndexPool*)pools[page.pool])->GetPageSize();
}

int IndexPoolManager::GetBaseIndex(const IndexPoolPage& page)
{
	CheckGood(page);
	return ((IndexPool*)pools[page.pool])->GetPageSize()*page.page;
}

