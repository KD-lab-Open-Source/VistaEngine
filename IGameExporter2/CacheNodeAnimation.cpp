#include "StdAfx.h"
#include "CacheNodeAnimation.h"

CacheNodeAnimation::CacheNodeAnimation()
{
}

CacheNodeAnimation::~CacheNodeAnimation()
{
	TSM::iterator it;
	FOR_EACH(time_slice_map,it)
		delete it->second;
	time_slice_map.clear();
}

void CacheNodeAnimation::Build(IGameScene *pIgame)
{
	all_nodes.clear();
	for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
	{
		IGameNode* pGameNode = pIgame->GetTopLevelNode(loop);
		all_nodes.push_back(pGameNode);
		BuildRecursive(pGameNode);
	}

	for(int i=0;i<all_nodes.size();i++)
	{
		node_map[all_nodes[i]]=i;
	}
}

void CacheNodeAnimation::BuildRecursive(IGameNode* pNode)
{
	for(int count=0;count<pNode->GetChildCount();count++)
	{
		IGameNode * pGameNode = pNode->GetNodeChild(count);
		all_nodes.push_back(pGameNode);
		BuildRecursive(pGameNode);
	}
}

Matrix3 CacheNodeAnimation::GetWorldTM(IGameNode* node,int max_time)
{
	TimeSlice* slice=BuildTimeSlice(max_time);

	NM::iterator node_it=node_map.find(node);
	if(node_it==node_map.end())
	{
		xassert(0);
		node_it=node_map.begin();
	}

	return slice->world_tm[node_it->second];
}

Matrix3 CacheNodeAnimation::GetLocalTM(IGameNode* node,int max_time)
{
	TimeSlice* slice=BuildTimeSlice(max_time);

	NM::iterator node_it=node_map.find(node);
	if(node_it==node_map.end())
	{
		xassert(0);
		node_it=node_map.begin();
	}

	return slice->local_tm[node_it->second];
}

CacheNodeAnimation::TimeSlice* CacheNodeAnimation::BuildTimeSlice(int max_time)
{
	TSM::iterator it=time_slice_map.find(max_time);
	if(it!=time_slice_map.end())
	{
		xassert(it->second->time==max_time);
		return it->second;
	}

	TimeSlice* p=new TimeSlice;
	p->time=max_time;
	p->world_tm.resize(all_nodes.size());
	p->local_tm.resize(all_nodes.size());

	for(int i=0;i<all_nodes.size();i++)
	{
		IGameNode* node=all_nodes[i];
		p->world_tm[i]=node->GetWorldTM(max_time).ExtractMatrix3();
		p->local_tm[i]=node->GetLocalTM(max_time).ExtractMatrix3();
	}

	time_slice_map[max_time]=p;
	return p;
}
