#ifndef __CLIPPING_MESH_H_INCLUDED__
#define __CLIPPING_MESH_H_INCLUDED__

//Довольно тупой класс, не вызывает конструкторов деструкторов для T
template<class T,int N>
class fixed_set
{
	T ptr[N];
	int size;

public:
	typedef T* iterator;
	typedef const T* const_iterator;

	fixed_set():size(0){}
	iterator begin(){return ptr;}
	iterator end(){return ptr+size;}
	const_iterator begin()const{return ptr;}
	const_iterator end()const{return ptr+size;}

	void insert(const T& data)
	{
		for(iterator cur=begin(),iend=end();cur<iend;++cur)
		{
			if(*cur==data)
				return;
		}

		xassert(size<N);
		ptr[size]=data;
		size++;
	}

	void erase(iterator it)
	{
		xassert(it>=begin() && it<end());

		for(iterator end1=end()-1;it<end1;++it)
			it[0]=it[1];
		size--;
	}
	void erase(const T& data)
	{
		for(iterator cur=begin(),iend=end();cur<iend;++cur)
		{
			if(*cur==data)
			{
				erase(cur);
				return;
			}
		}

		xassert(0);
	}

	bool empty()const {return size==0;}
};

struct CVertex
{
	Vect3f point;
	float distance;
	int occurs;
	bool visible;

	CVertex(){	distance=0;occurs=0;visible=true;}
};

struct CEdge
{
	int vertex[2];

	typedef fixed_set<int,12> FACE;
	FACE face;
	bool visible;
	CEdge(){visible=true;}
};

struct CFace
{
	typedef fixed_set<int,12> EDGE;
	EDGE edge;
	bool visible;
	CFace(){visible=true;}
};
/*
struct Plane
{
	float A,B,C,D;

	inline float distance(const Vect3f& p)
	{
		return A*p.x+B*p.y+C*p.z+D;
	}
};
*/
struct APolygons
{
	vector<Vect3f> points;

	//формат такой сначала идёт один int - количество элементов в полигоне (N).
	//потом N элементов - индексы точек в points
	vector<int> faces_flat;
};

///Класс для усечения выпуклого техмерного полигона плоскостями.
struct CMesh
{
	vector<CVertex> V;
	vector<CEdge> E;
	vector<CFace> F;
public:
	CMesh();	
	void CreateABB(Vect3f& vmin,Vect3f& vmax);

	int Clip(sPlane4f clipplane);

	void BuildPolygon(APolygons& p);
protected:
	float epsilon;
	bool GetOpenPolyline(const CFace& face,int& start,int& final);
};

#endif
