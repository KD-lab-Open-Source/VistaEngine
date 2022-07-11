#ifndef __VERTEX_BUFFER_H_INCLUDED__
#define __VERTEX_BUFFER_H_INCLUDED__

/// Классы для упрощения доступа к динамическому вертекс буферу.
class cVertexBufferInternal
{
protected:
	sPtrVertexBuffer vb;
	int numvertex;
	IDirect3DVertexDeclaration9* declaration;
	int cur_min_vertex,start_vertex;
public:
	cVertexBufferInternal();
	~cVertexBufferInternal();
	void Destroy();

	BYTE* Lock(int minvertex);
	void Unlock(int num_write_vertex);
	inline int GetSize(){return numvertex-cur_min_vertex;};

	void Create(int bytesize,int vertexsize,IDirect3DVertexDeclaration9* declaration);
	void DrawPrimitive(PRIMITIVETYPE Type,UINT Count);
	void DrawIndexedPrimitive(UINT Count);

	void Rewind();//Нужно для PerfHUD, чтобы количество DIP совпадало точнёхонько.
};

/// cVertexBuffer - доступ к динамическому буферу по циклу
/// Типичный вариант использования.
/*
	int nVertex=0;
	cVertexBuffer<sVertexXYZDT1>* pBuf=gb_RenderDevice->GetBufferXYZDT1();
	gb_RenderDevice->SetNoMaterial(...);

	sVertexXYZDT1 *Vertex=pBuf->Lock();
	for(int i=0;i<Particle.size();i++)
	{
		sVertexXYZDT1 *v=&Vertex[nVertex];
		Заполняем v[0]..v[5]
		nVertex+=6;
		if(nVertex+6>=pBuf->GetSize())
		{
			pBuf->Unlock(nVertex);
			pBuf->DrawPrimitive(PT_TRIANGLELIST,nVertex/3,GetGlobalMatrix());
			Vertex=pBuf->Lock();
			nVertex=0;
		}
	}

	pBuf->Unlock(nVertex);
	if(nVertex>0)
		pBuf->DrawPrimitive(PT_TRIANGLELIST,nVertex/3,GetGlobalMatrix());
*/
template <class vertex>
class cVertexBuffer
{
	cVertexBufferInternal buf;
public:

	inline void Destroy(){buf.Destroy();};
	inline vertex* Lock(int minvertex=40){return (vertex*)buf.Lock(minvertex);};
	inline void Unlock(int num_write_vertex){buf.Unlock(num_write_vertex);};
	inline int GetSize(){return buf.GetSize();};

	inline void CreateBytesize(int bytesize)
	{
		buf.Create(bytesize,sizeof(vertex),vertex::declaration);
	}

	inline void CreateVertexNum(int vertexnum)
	{
		buf.Create(vertexnum*sizeof(vertex),sizeof(vertex),vertex::declaration);
	}

	/*
	inline void DrawPrimitive(PRIMITIVETYPE Type,UINT Count,const MatXf &m)
	{
		buf.DrawPrimitive(Type,Count,m);
	}
	*/

	inline void DrawPrimitive(PRIMITIVETYPE Type,UINT Count)
	{
		buf.DrawPrimitive(Type,Count);
	}

	void Rewind(){buf.Rewind();}
};

class cQuadBufferInternal:protected cVertexBufferInternal
{
	BYTE* start_vertex;
	int vertex_index;
public:
	cQuadBufferInternal();
	~cQuadBufferInternal();

	void Destroy();
	void Create(int vertexsize, IDirect3DVertexDeclaration9* declaration);

	void SetMatrix(const MatXf &m);
	void BeginDraw();
	void* Get();
	void EndDraw();
	void Rewind(){cVertexBufferInternal::Rewind();}
};

/// cQuadBuffer Динамический вертекс буффер для случая квадов
/// То есть каждые 4 точки образуют квадрат из двух треугольников.
/// Типичный вариант использования
/*
	cQuadBuffer<sVertexXYZDT1>* quad=GetQuadBufferXYZDT1();
	gb_RenderDevice->SetNoMaterial(...);
	quad->BeginDraw();
	for(int i=0;i<;i++)
	{
		sVertexXYZDT1* p=quad->Get();
		Заполняем здесь 4 точки.
	}
	quad->EndDraw()
*/
template <class vertex>
class cQuadBuffer
{
	cQuadBufferInternal buf;
public:

	inline void Destroy(){buf.Destroy();}
	inline void Create()
	{
		buf.Create(sizeof(vertex),vertex::declaration);
	}

	inline void BeginDraw(const MatXf &m=MatXf::ID){buf.SetMatrix(m);buf.BeginDraw();};
	/// Возвращает указатель на 4 вершины
	inline vertex* Get(){return (vertex*)buf.Get();}
	inline void EndDraw(){buf.EndDraw();};
	void Rewind(){buf.Rewind();}
};

/// Понятие Strip из треугольников не буду описывать, потому как долго и в доках к DirectX хорошо описанно.
/// Вкраце каждые 4 последовательно точки с шагом в 2 точки определяют 2 треугольника расположенных квадом.
class DrawStrip
{
	cVertexBuffer<sVertexXYZDT1>* buf;
	sVertexXYZDT1* pointer;
	int num;
public:
	DrawStrip():buf(NULL){ }

	void Begin();
	void End();
	inline void Set(sVertexXYZDT1& p1,sVertexXYZDT1& p2);
};

void DrawStrip::Set(sVertexXYZDT1& p1,sVertexXYZDT1& p2)
{
	pointer[num++]=p1;
	pointer[num++]=p2;
	if(num+4>buf->GetSize())
	{
		buf->Unlock(num);
		buf->DrawPrimitive(PT_TRIANGLESTRIP,num-2);

		pointer=buf->Lock(8);
		num=0;

		pointer[num++]=p1;
		pointer[num++]=p2;
	}
}

class DrawStripT2
{
	cVertexBuffer<sVertexXYZDT2>* buf;
	sVertexXYZDT2* pointer;
	int num;
public:
	DrawStripT2():buf(NULL){ }

	void Begin(const MatXf& mat=MatXf::ID);
	void End();
	inline void Set(sVertexXYZDT2& p1,sVertexXYZDT2& p2);
};

void DrawStripT2::Set(sVertexXYZDT2& p1,sVertexXYZDT2& p2)
{
	pointer[num++]=p1;
	pointer[num++]=p2;
	if(num+4>buf->GetSize())
	{
		buf->Unlock(num);
		buf->DrawPrimitive(PT_TRIANGLESTRIP,num-2);

		pointer=buf->Lock(8);
		num=0;

		pointer[num++]=p1;
		pointer[num++]=p2;
	}
}

#endif
