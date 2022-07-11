#pragma once
#include <hash_map>

class CacheNodeAnimation
{
public:
	CacheNodeAnimation();
	~CacheNodeAnimation();
	void Build(IVisExporter *pIgame);
	Matrix3 GetWorldTM(IVisNode* node,int max_time);
	Matrix3 GetLocalTM(IVisNode* node,int max_time);
protected:
	vector<IVisNode*> all_nodes;

	struct TimeSlice
	{
		int time;
		vector<Matrix3> world_tm;
		vector<Matrix3> local_tm;
	};

	typedef hash_map<int, TimeSlice*> TSM;
	typedef map<IVisNode*,int> NM;
	TSM time_slice_map;
	NM node_map;
protected:
	void BuildRecursive(IVisNode* pIgame);
	TimeSlice* BuildTimeSlice(int max_time);
};
