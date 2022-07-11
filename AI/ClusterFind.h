//Balmer,K-D Lab
#ifndef __CLUSTERFIND_H__
#define __CLUSTERFIND_H__
#include "AIAStar.h"

typedef WORD AStarTile;

#define CF_UP_BIT //Верхний бит в walk_map несёт разделительную функцию

//Ещё большее сглаживание пути
template<class ClusterHeuristic>
float HeuristicLine(int xfrom,int yfrom,int xto,int yto,
		  int dx,int dy,AStarTile* walk_map,ClusterHeuristic& heuristic,
		  bool& debug_xor );

template<class ClusterHeuristic>
void SoftPath2(vector<Vect2i>& out_path,
			int dx,int dy,AStarTile* walk_map,
			ClusterHeuristic& heuristic);

class ClusterFind
{
public:
	struct Cluster
	{
		int x,y;//Левая верхняя точка.
		int xcenter,ycenter;//Не обязательно попадает в кластер
		AStarTile walk;//Сложность продвижения по этому куску
		bool temp_set;//Можно ли писать в link
		DWORD self_id;

		vector<Cluster*> link;//С кем связанны.
		vector<DWORD> index_link;//индекс в массиве all_cluster

		inline Cluster(){temp_set=true;}

		//Для AIAStarGraph
		typedef vector<Cluster*>::iterator iterator;
		inline iterator begin(){return link.begin();}
		inline iterator end(){return link.end();}
		void* AIAStarPointer;

	};

#ifdef CF_UP_BIT
	enum {
		UP_MASK=0x8000,
		DOWN_MASK=0x7FFF,
		GROUND_MASK=0x0100,
		IMPASSABILITY_MASK=0x0800,
		WATER_MASK=0x1000,
	};
#endif 

	enum { 
		max_cluster_size = 8192,
	};


	ClusterFind(int dx,int dy,int max_distance=10);
	~ClusterFind();

	// Доступ к карте для заполнения перед Set/SetLater
	AStarTile* GetWalkMap(){ return walk_map; }

	//Создать сеть кластеров по walk_map
	void Set(bool enable_smooting);
	
	//Так как Set очень длительная операция, то 
	//разбить на несколько квантов
	void SetLater(bool enable_smooting,int quant_of_build);
	bool SetLaterQuant();//true - процесс завершён
	bool ready() const { return cur_quant_build >= quant_of_build; }

	template<class ClusterHeuristic>
	bool FindPath(const Vect2i& from, const Vect2i& to, vector<Vect2i>& out_path, ClusterHeuristic& heuristic)
	{
		heuristic.end = getCluster(to);

		vector<Cluster*> path;
		AIAStarGraph<ClusterHeuristic,Cluster> astar;
		astar.Init(all_cluster);
//		if(!astar.FindPath(getCluster(from), &heuristic, path))
//			return false;

		bool full_path = astar.FindPath(getCluster(from), &heuristic, path);

		SoftPath(path, from, to, out_path);

		SoftPath2(out_path, dx, dy, walk_map, heuristic);

		if(out_path.size()>1){
			xassert(out_path.front() == from);
			out_path.erase(out_path.begin());
		}
		
		return full_path;
	}

	template<class ClusterHeuristic>
	bool FindPathMulti(const Vect2i& from, const vector<Vect2i>& to, vector<Vect2i>& out_path, ClusterHeuristic& heuristic)
	{
		vector<Vect2i>::const_iterator vi;
		FOR_EACH(to, vi)
			heuristic.addEnd(*vi, getCluster(*vi));

		vector<Cluster*> path;
		AIAStarGraph<ClusterHeuristic,Cluster> astar;
		astar.Init(all_cluster);
		if(!astar.FindPath(getCluster(from), &heuristic, path))
			return false;

		xassert(!path.empty());
		SoftPath(path, from, heuristic.to(path.back()), out_path);

		SoftPath2(out_path, dx, dy, walk_map, heuristic);

		if(out_path.size()>1){
			xassert(out_path.front() == from);
			out_path.erase(out_path.begin());
		}
		
		return true;
	}

	int GetNumCluster() { return all_cluster.size(); }

	inline Cluster* getCluster(const Vect2i& point)
	{
		xassert(point.x >= 0 && point.x < dx && point.y >= 0 && point.y < dy);
		xassert(ready());

		unsigned int index = pmap[point.y*dx + point.x] - 1;
		xassert(index < all_cluster.size());
		return &all_cluster[index]; 
	}

protected:
	int dx,dy;
	DWORD* pmap;
	AStarTile* walk_map;

	enum{
		max_cell_in_front=64,
	};

	int max_distance;
	/*Vect2i*/
	typedef Vect2i Front;

	Front *pone,*ptwo;
	int size_one,size_two;

	vector<Cluster> all_cluster;

	BYTE* is_used;//Для SoftPath
	DWORD is_used_size;
	int is_used_xmin,is_used_xmax,is_used_ymin,is_used_ymax;

	//для SetLater
	int quant_of_build;//Сколько квантов необходимо для построения карты
	int cur_quant_build;
	int set_later_cur_num;


	/////////////////////////
	//	Private Members
	void Relink();
	void Smooting();
	//Добавлять, если temp_set==true
	vector<DWORD> vtemp_set;//Для ClusterOne
	void ClusterOne(int x,int y,int id,Cluster& c);

	//Возвращает true, если нашёл путь на два шага вперёд
	bool IterativeFindPath(Vect2i from,
						   Vect2i center,Vect2i to,
						   Vect2i up,Vect2i up_to,
						   vector<Vect2i>& path);
	enum LINE_RET
	{
		L_BAD=0,
		L_GOOD,
		L_COMPLETE
	};

	LINE_RET Line(int xfrom,int yfrom,int xto,int yto,
			DWORD prev,DWORD eq,int& xeq,int& yeq,bool enable_add_one=false);

	bool LineWalk(int xfrom,int yfrom,int xto,int yto,
					   AStarTile max_walk);

	//То-же поиск волной. Ищет ячейки соприкасающиеся с to.
	void FindClusterFront(int x,int y,DWORD to,
		vector<Front>& front);

	void SoftPath(vector<Cluster*>& in_path,Vect2i from,Vect2i to,
				vector<Vect2i>& out_path);

	void BuildSidePath(vector<Vect2i>& in_path,
					vector<Vect2i>& out_path,
					int max_distance,bool left);

	void CheckAllLink();
};


static AStarTile flags_=ClusterFind::GROUND_MASK;

template<class ClusterHeuristic>
float HeuristicLine(int xfrom,int yfrom,int xto,int yto,
		  int dx,int dy,AStarTile* walk_map,ClusterHeuristic& heuristic,
		  bool& debug_xor )
{

	float x,y;
	float lx,ly;
	if(xfrom==xto && yfrom==yto)return 0;
	xassert(xfrom>=0 && xfrom<dx && yfrom>=0 && yfrom<dy);
	xassert(xto>=0 && xto<dx && yto>=0 && yto<dy);

	x=xfrom;
	y=yfrom;

	lx=xto-xfrom;
	ly=yto-yfrom;
	int t,maxt;
	if(fabsf(lx)>fabsf(ly))
	{
		maxt=fabsf(lx);
		ly=ly/fabsf(lx);
		lx=(lx>0)?+1:-1;
	}else
	{
		maxt=fabsf(ly);
		lx=lx/fabsf(ly);
		ly=(ly>0)?+1:-1;
	}

	float sq_mul=sqrtf(sqr(lx)+sqr(ly));
	float len=0;
	AStarTile walk_from,walk_to;
	
//	if(walk_map[yfrom*dx+xfrom] & (~flags_))
//		walk_from = 0xFF;
//	else
//		walk_from = walk_map[yfrom*dx+xfrom];
//	walk_from = (walk_map[yfrom*dx+xfrom] & (~flags_))? HEURISTIC_MAX : walk_map[yfrom*dx+xfrom];
	walk_from=walk_map[yfrom*dx+xfrom];

	debug_xor=false;

	for(t=0;t<maxt;t++)
	{
		x+=lx;y+=ly;

		int ix=round(x),iy=round(y);
		xassert(ix>=0 && ix<dx && iy>=0 && iy<dy);

//		if(walk_map[iy*dx+ix] & (~flags_))
//			walk_to = 0xFF;
//		else
//			walk_to = walk_map[iy*dx+ix];
//		walk_to = (walk_map[iy*dx+ix] & (~flags_))? HEURISTIC_MAX : walk_map[iy*dx+ix];
		walk_to=walk_map[iy*dx+ix];
		float l=heuristic(walk_from,walk_to)*sq_mul;
		debug_xor|=((walk_from^walk_to)&ClusterFind::UP_MASK)?true:false;

		walk_from=walk_to;
		len+=l;
	}

	return len;
}


template<class ClusterHeuristic>
void SoftPath2(vector<Vect2i>& out_path,
			int dx,int dy,AStarTile* walk_map,
			ClusterHeuristic& heuristic)
{
	//for(int iteration=0;iteration<2;iteration++)
	for(int i=1;i<out_path.size()-1;i++)
	{
		//Убрать эту точку
		Vect2i p0,p1,p2;
		p0=out_path[i-1];
		p1=out_path[i];
		p2=out_path[i+1];

		float l0,l1,lskip;
		bool b0,b1,bskip;

		lskip=HeuristicLine(p0.x,p0.y,p2.x,p2.y,
				dx,dy,walk_map,heuristic,bskip);

		l0=HeuristicLine(p0.x,p0.y,p1.x,p1.y,
				dx,dy,walk_map,heuristic,b0);

		l1=HeuristicLine(p1.x,p1.y,p2.x,p2.y,
				dx,dy,walk_map,heuristic,b1);


		if(lskip<l0+l1)
		{
			if(bskip || b0 || b1)
			{
				int k=0;
			}

			out_path.erase(out_path.begin()+i);
			i--;
		}
	}
}


//////////////////////////////////////////////////////////////
//		PathFind
//////////////////////////////////////////////////////////////
static const float gmul=1.0f;
static const float gfield=gmul*10000;

class ClusterHeuristicSimple
{
public:
	typedef ClusterFind::Cluster Node;
	Node* end;

	//Предполагаемые затраты на продвижение из pos1 к окончанию
	inline float GetH(Node* pos0, Node* pos)
	{
		return sqrtf(sqr(pos->xcenter-end->xcenter)+
					sqr(pos->ycenter-end->ycenter))*gmul;
	}

	inline float operator()(AStarTile walk_from, AStarTile walk_to)
	{
		if((walk_from^walk_to)&ClusterFind::UP_MASK)
			return gfield;

//		return (walk_to&ClusterFind::DOWN_MASK)+gmul;
		return gmul;
	}

	//Затраты на продвижение из pos1 в pos2
	inline float GetG(Node* pos1,Node* pos2)
	{
//		float mul=(pos2->walk&ClusterFind::DOWN_MASK)+gmul;
		float mul=gmul;

		float f=sqrtf(sqr(pos1->xcenter-pos2->xcenter)+
					sqr(pos1->ycenter-pos2->ycenter))*mul;

		if((pos1->walk^pos2->walk)&ClusterFind::UP_MASK)
			f+=gfield;

		return f;
	}

	inline bool IsEndPoint(Node* pos){return pos==end;}
};
/*
class ClusterHeuristicDitch : public ClusterHeuristic
{//Рвы считаются непреодолимыми препятствиями (высота==0)
public:
	enum {
		heuristic_ditch = 127 //Ров на нулевой высоте
	};

	inline float GetG(Node* pos1,Node* pos2)
	{
		if((pos2->walk & ClusterFind::DOWN_MASK) == heuristic_ditch)
			return 1000.0f;
		return ClusterHeuristic::GetG(pos1,pos2);
	}
	inline float operator()(BYTE walk_from,BYTE walk_to)
	{
		if((walk_to & ClusterFind::DOWN_MASK) == heuristic_ditch)
			return 1000.0f;

		return (*(ClusterHeuristic*)this)(walk_from,walk_to);
	}
};

class ClusterHeuristicOnlyWater : public ClusterHeuristic {
// только Вода - непроходимая.
public:
	enum {
		heuristic_ditch = 127 //Ров на нулевой высоте
	};

	inline float GetG(Node* pos1,Node* pos2)
	{
		if(pos2->walk & ClusterFind::WATER_MASK)
			return 1000.0f;
		return ClusterHeuristic::GetG(pos1,pos2);
	}
	inline float operator()(BYTE walk_from,BYTE walk_to)
	{
		if(walk_to & ClusterFind::WATER_MASK)
			return 1000.0f;

		return (*(ClusterHeuristic*)this)(walk_from,walk_to);
	}
};

class ClusterHeuristicOnlyImpassibility : public ClusterHeuristic {
// только зоны непроходимости.
public:
	enum {
		heuristic_ditch = 127 //Ров на нулевой высоте
	};

	inline float GetG(Node* pos1,Node* pos2)
	{
		if(pos2->walk & ClusterFind::IMPASSIBILITY_MASK)
			return 1000.0f;
		return ClusterHeuristic::GetG(pos1,pos2);
	}
	inline float operator()(BYTE walk_from,BYTE walk_to)
	{
		if(walk_to & ClusterFind::IMPASSIBILITY_MASK)
			return 1000.0f;

		return (*(ClusterHeuristic*)this)(walk_from,walk_to);
	}
};
class ClusterHeuristicWaterImpassibility : public ClusterHeuristic {
// Вода и зоны - непроходимые.
public:
	enum {
		heuristic_ditch = 127 //Ров на нулевой высоте
	};

	inline float GetG(Node* pos1,Node* pos2)
	{
		if((pos2->walk & ClusterFind::WATER_MASK) || (pos2->walk & ClusterFind::IMPASSIBILITY_MASK))
			return 1000.0f;
		return ClusterHeuristic::GetG(pos1,pos2);
	}
	inline float operator()(BYTE walk_from,BYTE walk_to)
	{
		if((walk_to & ClusterFind::WATER_MASK) || (walk_to & ClusterFind::IMPASSIBILITY_MASK))
			return 1000.0f;

		return (*(ClusterHeuristic*)this)(walk_from,walk_to);
	}
};

/*
class ClusterHeuristicHard
{
public:
	typedef ClusterFind::Cluster Node;
	Node* end;

	//Предполагаемые затраты на продвижение из pos1 к окончанию
	inline float GetH(Node* pos)
	{
		return sqrtf(sqr(pos->xcenter-end->xcenter)+
					sqr(pos->ycenter-end->ycenter));
	}

	inline float operator()(BYTE walk_from,BYTE walk_to)
	{
		return (walk_to&ClusterFind::DOWN_MASK)*10000.0f+1.0f;
	}

	//Затраты на продвижение из pos1 в pos2
	inline float GetG(Node* pos1,Node* pos2)
	{
		float mul=(pos2->walk&ClusterFind::DOWN_MASK)*10000.0f+1.0f;

		float f=sqrtf(sqr(pos1->xcenter-pos2->xcenter)+
					sqr(pos1->ycenter-pos2->ycenter))*mul;
		return f;
	}

	inline bool IsEndPoint(Node* pos){return pos==end;}
};
*/

class ClusterHeuristicComplex : public ClusterHeuristicSimple {
public:

	inline float GetG(Node* pos1,Node* pos2)
	{
		if(pos2->walk & (~flags_))
			return HEURISTIC_MAX;
		return ClusterHeuristicSimple::GetG(pos1,pos2);
	}
	inline float operator()(AStarTile walk_from,AStarTile walk_to)
	{
		if(walk_to & (~flags_))
			return HEURISTIC_MAX;

		return (*(ClusterHeuristicSimple*)this)(walk_from,walk_to);
	}
};

#endif //__CLUSTERFIND_H__
