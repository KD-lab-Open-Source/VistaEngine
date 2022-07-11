#ifndef __GLOBAL_STATISTICS_H__
#define __GLOBAL_STATISTICS_H__

#include "PlayerStatistics.h"

class Scores;
class ScoresResult;

struct StatisticsEntry : PlayerStatistics
{
	int position;
	string name;
	__int64 rating;

	void set(const Scores& scores);
};

class GlobalStatistics : public vector<StatisticsEntry> 
{
public:
	void set(const ScoresResult& scoresResult);
};

#endif //__GLOBAL_STATISTICS_H__