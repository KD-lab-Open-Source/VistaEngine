#pragma once
#include <hash_map>

class CacheNodeAnimation
{
public:
	CacheNodeAnimation();
	~CacheNodeAnimation();
	void Build(IGameScene *pIgame);
	Matrix3 GetWorldTM(IGameNode* node,int max_time);
	Matrix3 GetLocalTM(IGameNode* node,int max_time);
protected:
	vector<IGameNode*> all_nodes;

	struct TimeSlice
	{
		int time;
		vector<Matrix3> world_tm;
		vector<Matrix3> local_tm;
	};

	typedef hash_map<int, TimeSlice*> TSM;
	typedef map<IGameNode*,int> NM;
	TSM time_slice_map;
	NM node_map;
protected:
	void BuildRecursive(IGameNode* pIgame);
	TimeSlice* BuildTimeSlice(int max_time);
};
