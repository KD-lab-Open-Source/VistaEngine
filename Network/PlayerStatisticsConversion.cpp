#include "stdAfx.h"
#undef XREALLOC
#undef XFREE
//#include "..\Network\DemonWareAux\Scores.h"
//#include "..\Network\DemonWareAux\ScoresQueries.h"
#include "PlayerStatistics.h"
#include "AttributeReference.h"
#include "GlobalStatistics.h"
#include "UnitAttribute.h"

/*
void PlayerStatistics::set(const Scores& scores)
{
	statistics_[TOTAL_TIME_PLAYED] = scores.getTotalTimePlayed();
	statistics_[TOTAL_GAMES_PLAYED] = scores.getTotalWins() + scores.getTotalLosses();
	statistics_[TOTAL_WINS] = scores.getTotalWins();
	statistics_[TOTAL_LOSSES] = scores.getTotalLosses();
	statistics_[TOTAL_DISCONNECTIONS] = scores.getTotalConnections() - scores.getTotalWins() - scores.getTotalLosses();
	statistics_[STAT_OBJECTS_MY_BUILT] = scores.getUnitsBuilt();
	statistics_[STAT_OBJECTS_ENEMY_KILLED] = scores.getEnemyUnitsKilled();
	statistics_[STAT_OBJECTS_MY_KILLED] = scores.getUnitsKilledByEnemy();
	statistics_[STAT_OBJECTS_ENEMY_CAPTURED] = scores.getEnemyUnitsCaptured();
	statistics_[STAT_OBJECTS_MY_CAPTURED] = scores.getUnitsCapturedByEnemy();
	statistics_[STAT_KEYPOINTS_CAPTURED] = scores.getStrategicPointsCaptured();
	statistics_[STAT_KEYPOINTS_LOST] = scores.getStrategicPointsLost();
	statistics_[STAT_HEROES_ENEMY_KILLED] = scores.getHeroesKilled();
	statistics_[STAT_HEROES_MY_KILLED] = scores.getHeroesLost();
	statistics_[STAT_RESOURCES_COLLECTED] = scores.getTotalResourcesCollected();
	statistics_[STAT_RESOURCES_SPENT] = scores.getTotalResourcesSpent();
	statistics_[STAT_OBJECTS_ENEMY_KILLED_RESOURCES] = scores.getResourceCostOfAllEnemyUnitsKild();
	statistics_[STAT_OBJECTS_MY_KILLED_RESOURCES] = scores.getResourceCostOfPlayersUnitsKild();
}

void PlayerStatistics::get(Scores& scores) const
{
	scores.setRating(statistics_[STAT_RATING]);
	scores.setTotalTimePlayed(statistics_[TOTAL_TIME_PLAYED]);
	scores.setTotalWins(statistics_[TOTAL_WINS]);
	scores.setTotalLosses(statistics_[TOTAL_LOSSES]);
	scores.setUnitsBuilt(statistics_[STAT_OBJECTS_MY_BUILT]);
	scores.setEnemyUnitsKilled(statistics_[STAT_OBJECTS_ENEMY_KILLED]);
	scores.setUnitsKilledByEnemy(statistics_[STAT_OBJECTS_MY_KILLED]);
	scores.setEnemyUnitsCaptured(statistics_[STAT_OBJECTS_ENEMY_CAPTURED]);
	scores.setUnitsCapturedByEnemy(statistics_[STAT_OBJECTS_MY_CAPTURED]);
	scores.setStrategicPointsCaptured(statistics_[STAT_KEYPOINTS_CAPTURED]);
	scores.setStrategicPointsLost(statistics_[STAT_KEYPOINTS_LOST]);
	scores.setHeroesKilled(statistics_[STAT_HEROES_ENEMY_KILLED]);
	scores.setHeroesLost(statistics_[STAT_HEROES_MY_KILLED]);
	scores.setTotalResourcesCollected(statistics_[STAT_RESOURCES_COLLECTED]);
	scores.setTotalResourcesSpent(statistics_[STAT_RESOURCES_SPENT]);
	scores.setResourceCostOfAllEnemyUnitsKild(statistics_[STAT_OBJECTS_ENEMY_KILLED_RESOURCES]);
	scores.setResourceCostOfPlayersUnitsKild(statistics_[STAT_OBJECTS_MY_KILLED_RESOURCES]);
}


void GlobalStatistics::set(const ScoresResult& scoresResult)
{
	clear();
	for(int i = 0; i < scoresResult.m_numRows; i++){
		push_back(StatisticsEntry());
		back().set(scoresResult.m_ScoresRow[i]);
	}
}

void StatisticsEntry::set(const Scores& scores)
{
	char buffer[BD_MAX_USERNAME_LENGTH + 1];
	scores.getEntityName(buffer, BD_MAX_USERNAME_LENGTH);
	name = buffer;
	position = scores.getRank();
	rating = scores.getRating();
	PlayerStatistics::set(scores);
}
*/
