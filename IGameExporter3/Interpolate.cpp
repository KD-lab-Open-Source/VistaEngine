#include "StdAfx.h"
#include "Interpolate.h"

struct OneVisibility{
	int interval_begin;
	int interval_size;
	bool value;
};

void RefinerBool::refine(int interval_size, bool cycled)
{
	if(visibility_.empty())
		return;
	vector<OneVisibility> intervals;
	//¬ принципе аналог кода из InterpolatePosition, но дл€ констант.
	int size = visibility_.size();
	bool cur = visibility_[0];
	int begin=0;
	for(int i=0;i<size;i++)
	{
		bool b=visibility_[i];
		if(b!=cur)
		{
			OneVisibility one;
			one.interval_begin=begin;
			one.interval_size=i-begin;
			xassert(one.interval_size>0);
			one.value=cur;
			intervals.push_back(one);

			begin=i;
			cur=b;
		}
	}

	OneVisibility one;
	one.interval_begin=begin;
	one.interval_size=i-begin;
	xassert(one.interval_size>0);
	one.value=cur;
	intervals.push_back(one);

	int sum=0;
	int cur_begin=0;
	for(i=0;i<intervals.size();i++)
	{
		OneVisibility& o=intervals[i];
		xassert(o.interval_begin==sum);
		sum+=o.interval_size;
	}
	xassert(sum==size);

	nodes_.resize(intervals.size());
	for(i = 0; i < intervals.size(); i++){
		OneVisibility& o = intervals[i];
		nodes_[i].interval_begin = float((o.interval_begin)/(float)interval_size);
		nodes_[i].interval_size  = float(o.interval_size/(float)interval_size);
		nodes_[i].value = o.value;
	}
}

void RefinerBool::export(Interpolator3dxBool& interpolator) const
{
	interpolator.values.resize(nodes_.size());
	for(int i=0; i < nodes_.size(); i++){
		Interpolator3dxBool::Data& data = interpolator.values[i];
		const Node& node = nodes_[i];
		data.tbegin = node.interval_begin;
		data.inv_tsize = 1.0f/node.interval_size;
		data.value = (int)node.value;
	}
}

