#ifndef __QSWORLDSMGR_H__
#define __QSWORLDSMGR_H__

#include "FileUtils\XGUID.h"

struct QSWorldInfo {
	int missionNum;
	string missionName;
    XGUID missionGUID;
	QSWorldInfo(){};
	QSWorldInfo(int _missionNum, const char* _missionName, XGUID& _missionGUID){
		missionNum=_missionNum;
		missionName=_missionName;
		missionGUID=_missionGUID;
	}
	void serialize(Archive& ar);
};

class QSWorldsMgr {
public:
	QSWorldsMgr();
	void updateQSWorld(const char* missionName, XGUID& missionID);
	void serialize(Archive& ar);
	void save();
	__int64 createFilter(const std::vector<XGUID>& missionFilter);
	void getRandomMissionGuid(__int64& inLowFilterMap, XGUID& outMissionGuid);
	bool isMissionPresent(const class XGUID& missionGuid);
private:
	vector<QSWorldInfo> worldsLst;
};

extern QSWorldsMgr qsWorldsMgr;

#endif //__QSWORLDSMGR_H__