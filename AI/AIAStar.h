#ifndef __AI_A_STAR_H_INCLUDED__
#define __AI_A_STAR_H_INCLUDED__
//K-D Lab / Balmer

#include <map>
#include <vector>
#include <algorithm>

#define HEURISTIC_MAX 1.0e6f // Используется для обозначения непроходимых зон

///////////////////////AIAStarGraph/////////////
//AIAStarGraph - Так-же поиск пути, но ориентированный на поиск в произвольном графе
/*
class Node
{
	typedef ... iterator;
	iterator begin();//Работа со списком связанных с этой Node нод.
	iterator end();

	void* AIAStarPointer;//Используется в AIAStarGraph
};

class Heuristic
{
	float GetH(Node* pos);//Предполагаемые затраты на продвижение из pos1 к окончанию
	float GetG(Node* pos1,Node* pos2);//Затраты на продвижение из pos1 в pos2
	bool IsEndPoint(Node* pos);//Рекурсия должна окончиться здесь
	//то есть класс AIAStar позволяет задавать несколько точек окончания поиска пути
};
*/

template<class Heuristic,class Node,class TypeH=float>
class AIAStarGraph
{
public:
	struct OnePoint;
	typedef multimap<TypeH, OnePoint*> type_point_map;

	struct OnePoint
	{
		TypeH g;//Затраты на продвижение до этой точки
		TypeH h;//Предполагаемые затраты на продвижение до финиша
		DWORD used;
		OnePoint* parent;
		bool is_open;

		Node* node;
		typename type_point_map::iterator self_it;

		inline TypeH f(){return g+h;}
	};
protected:
	vector<OnePoint> chart;
	type_point_map open_map;

	DWORD is_used_num;//Если is_used_num==used, то ячейка используется

	int num_point_examine;//количество посещённых ячеек
	int num_find_erase;//Сколько суммарно искали ячейки для удаления
	Heuristic* heuristic;
public:
	AIAStarGraph();

	//Общее количество узлов. Константа, которая не должна меняться,
	//пока существует класс, указывающий на неё.
	void Init(vector<Node>& all_node);

	bool FindPath(Node* from, Heuristic* h, bool considerImpassabilities, vector<Node*>& path);
	
protected:
	void clear();
	inline Node* PosBy(OnePoint* p)
	{
		return p->node;
	}
};

template<class Heuristic,class Node,class TypeH>
AIAStarGraph<Heuristic,Node,TypeH>::AIAStarGraph()
{
	heuristic=NULL;
}

template<class Heuristic,class Node,class TypeH>
void AIAStarGraph<Heuristic,Node,TypeH>::Init(vector<Node>& all_node)
{
	int size=all_node.size();
	chart.resize(size);

	for(int i=0;i<size;i++)
	{
		OnePoint* c=&chart[i];
		c->node=&all_node[i];
		c->node->AIAStarPointer=(void*)c;
	}
	clear();
}

template<class Heuristic,class Node,class TypeH>
void AIAStarGraph<Heuristic,Node,TypeH>::clear()
{
	is_used_num=0;
	vector<OnePoint>::iterator it;
	FOR_EACH(chart,it)
		it->used=0;
}

template<class Heuristic,class Node,class TypeH>
bool AIAStarGraph<Heuristic,Node,TypeH>::FindPath(Node* from, Heuristic* hr, bool considerImpassabilities, vector<Node*>& path)
{
	num_point_examine=0;
	num_find_erase=0;

	is_used_num++;
	open_map.clear();
	path.clear();
	if(is_used_num==0)
		clear();//Для того, чтобы вызвалась эта строчка, необходимо гиганское время
	heuristic=hr;

	OnePoint* p=(OnePoint*)from->AIAStarPointer;
	Node* from_node=p->node;
	p->g=0;
	p->h=heuristic->GetH(NULL, p->node);
	p->used=is_used_num;
	p->is_open=true;
	p->parent=NULL;

	p->self_it = open_map.insert(type_point_map::value_type(p->f(),p));

	bool excludeImpassabilities = !considerImpassabilities;

	while(!open_map.empty())
	{
		type_point_map::iterator low=open_map.begin();

		OnePoint* parent=low->second;
		Node* node = parent->node;

		parent->is_open=false;
		open_map.erase(low);

		if(heuristic->IsEndPoint(node))
		{
			if(excludeImpassabilities)
				return false;

			bool full_path = true;
			float heuristicMax = (excludeImpassabilities == considerImpassabilities ? sqr(HEURISTIC_MAX) : HEURISTIC_MAX) * 0.1f;
			if(parent->g > heuristicMax)
				full_path = false;

			//сконструировать путь
			Node* p;
			while(parent)
			{
				p=PosBy(parent);
				xassert(parent->used==is_used_num);

				if(parent->g < heuristicMax) 
					path.push_back(p);
				parent=parent->parent;
			}
			xassert(p==from_node);
			reverse(path.begin(),path.end());
			return full_path;
		}

		//для каждого наследника child узла parent
		Node::iterator it;
		FOR_EACH(*node,it)
		{
			Node* cur_node=*it;
			OnePoint* p=(OnePoint*)cur_node->AIAStarPointer; // !!! exception
			num_point_examine++;

			TypeH addg = heuristic->GetG(node, cur_node);
			
			if(isLess(addg, HEURISTIC_MAX)){
				if(excludeImpassabilities)
					excludeImpassabilities = false;
				if(considerImpassabilities && isGreater(parent->g, HEURISTIC_MAX))
					addg *= HEURISTIC_MAX;
			}else if(considerImpassabilities == excludeImpassabilities)
				addg *= HEURISTIC_MAX;

			addg *= heuristic->GetDistance(node, cur_node);
			
			TypeH newg = parent->g + addg;

			if(p->used==is_used_num)
			{
				if(!p->is_open)continue;
				if(p->g<=newg)continue;

				open_map.erase(p->self_it);
				num_find_erase++;
			}

			p->parent=parent;
			p->g=newg;
			p->h=heuristic->GetH(node,cur_node);

			p->self_it = open_map.insert(type_point_map::value_type(p->f(),p));

			p->is_open=true;
			p->used=is_used_num;
		}
	}

	xassert(!"Очень серьезная ошибка...");
	return false;
}

#endif
