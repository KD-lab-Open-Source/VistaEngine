#ifndef __GLOBAL_STATISTICS_H__
#define __GLOBAL_STATISTICS_H__

#include "PlayerStatistics.h"

struct StatisticsEntry : PlayerStatistics
{
	int position;
	string name;
	__int64 rating;

	void set();
};

class GlobalStatistics : public vector<StatisticsEntry> 
{
public:
	void set();
};

#endif //__GLOBAL_STATISTICS_H__