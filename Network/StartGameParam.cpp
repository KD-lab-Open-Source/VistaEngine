#include "StdAfx.h"

#include "P2P_interface.h"


StartGameParam::StartGameParam(MTSection& mts) : mtLock(mts)
{
	quickStart=false;
	useMapSettingsFilter=NO_FILTER;
	maxPlayersFilter=NO_FILTER;
	gameProtection=NO_FILTER;
}

void StartGameParam::reset()
{
	useMapSettingsFilter=NO_FILTER;
	maxPlayersFilter=NO_FILTER;
	quickStart=NO_FILTER;
	gameOrder = GameOrder_NoFilter;
	gameProtection=NO_FILTER;
	missionFilter.clear();
}

void StartGameParam::setNS(int _useMapSettingsFilter, const std::vector<XGUID>& _missionFilter, int _maxPlayersFilter, char _gameProtection)
{
	MTAuto _Lock(mtLock);

	reset();
	quickStart=false;
	useMapSettingsFilter=_useMapSettingsFilter;
	missionFilter.clear();
	std::vector<XGUID>::const_iterator p;
	for(p=_missionFilter.begin(); p!=_missionFilter.end(); p++)
		missionFilter.push_back(*p);
	maxPlayersFilter=_maxPlayersFilter;
	gameProtection=_gameProtection;
}

void StartGameParam::setQS(eGameOrder _gameOrder, __int64 _filterMap)
{
	MTAuto _Lock(mtLock);

	reset();
	quickStart=true;
	gameOrder=_gameOrder;
	filterMap=_filterMap;
}

bool StartGameParam::isConditionEntry(const sGameStatusInfo& gsi)
{
	MTAuto _Lock(mtLock);

	if((bool)quickStart!=gsi.flag_quickStart) return false;
	if(quickStart){
		return ( (filterMap&gsi.filterMap) != 0) && (gameOrder==gsi.gameOrder);
	}
	else {
		if(useMapSettingsFilter!=NO_FILTER && useMapSettingsFilter!=(int)gsi.jointGameType.isUseMapSetting() )
			return false;
		if(!missionFilter.empty()){
			vector<GUID>::iterator p;
			for(p=missionFilter.begin(); p!=missionFilter.end(); ++p)
				if( *p == gsi.missionGuid) break;
			if(p==missionFilter.end())
				return false;
		}
		if( maxPlayersFilter!=NO_FILTER && gsi.maximumPlayers > maxPlayersFilter)
			return false;
		return true;
	}
}

StartGameParamBase StartGameParam::getGameParam()
{
	MTAuto _Lock(mtLock);
	StartGameParamBase sgp= static_cast<StartGameParamBase&>(*this);
	return sgp;
}

