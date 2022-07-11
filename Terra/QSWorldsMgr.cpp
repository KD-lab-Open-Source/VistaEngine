#include "stdafxTr.h"
#include "QSWorldsMgr.h"
#include "Serialization\Serialization.h"
#include "Serialization\XPrmArchive.h"
#include "FileUtils\FileUtils.h"
#include "FileUtils\XGUID.h"

//#include "Network\LogMsg.h" // Недопустимая зависимость


QSWorldsMgr qsWorldsMgr;

static const char* QSWorldsMgrFileName	= "RESOURCE\\WORLDS\\qswl.scr"; //"Scripts\\Content\\qswl.scr";
static const char* QSWorldsMgrSection	= "QSDate";

QSWorldsMgr::QSWorldsMgr()
{
	worldsLst.reserve(32);
	XPrmIArchive ia;
	if(ia.open(QSWorldsMgrFileName)){
		ia.serialize(*this, QSWorldsMgrSection, 0);
	}
	//check
	int cnt=0;
	vector<QSWorldInfo>::iterator p;
	for(p=worldsLst.begin(); p!=worldsLst.end(); p++){
		xassert(cnt == p->missionNum);
		p->missionNum = cnt;
		cnt++;
	}
}
void QSWorldsMgr::save()
{
	XPrmOArchive oa(QSWorldsMgrFileName);
	oa.serialize(*this, QSWorldsMgrSection, 0);
}

void QSWorldInfo::serialize(Archive& ar)
{
	ar.serialize(missionNum, "missionNum", 0);
	ar.serialize(missionName, "missionName", 0);
	ar.serialize(missionGUID, "missionGUID", 0);
}

void QSWorldsMgr::serialize(Archive& ar)
{
	ar.serialize(worldsLst, "worldsLst", 0);
}

void QSWorldsMgr::updateQSWorld(const char* _missionName, XGUID& _missionID)
{
	string missionName = extractFileBase(_missionName);
	int cnt=0;
	vector<QSWorldInfo>::iterator p;
	for(p=worldsLst.begin(); p!=worldsLst.end(); p++){
		if(stricmp(missionName.c_str(), p->missionName.c_str())==0){
			p->missionGUID = _missionID;
			break;
		}
		cnt++;
	}
	if(p==worldsLst.end()){
		worldsLst.push_back(QSWorldInfo(cnt, missionName.c_str(), _missionID));
	}
	//save();
}

__int64 QSWorldsMgr::createFilter(const std::vector<XGUID>& missionFilter)
{
	__int64 outLowFilterMap=0;
	int cnt=0;
	vector<QSWorldInfo>::iterator p;
	for(p=worldsLst.begin(); p!=worldsLst.end(); p++){
		if(!missionFilter.empty()){
			vector<XGUID>::const_iterator k;
			for(k=missionFilter.begin(); k!=missionFilter.end(); k++){
				if(p->missionGUID == *k){
					outLowFilterMap |= (__int64)1 << cnt;
					break;
				}
			}
		}
		else
			outLowFilterMap |= (__int64)1 << cnt;

		cnt++;
		if(cnt >= 64) {
			xassert(0&&"QSWorldsMgr-worldsLst big!");
			break;
		}
	}
	return outLowFilterMap;
}

void QSWorldsMgr::getRandomMissionGuid(__int64& inLowFilterMap, XGUID& outMissionGuid)
{
	int cntMission=0;
	for(int i=0; i<64; i++){
		if( (inLowFilterMap&((__int64)1<<i)) !=0 )
			cntMission++;
	}
	//LogMsg("QS - missionFiltered:%u\n", cntMission);
	RandomGenerator myrnd;
	myrnd.set(GetTickCount());
    int number=myrnd(cntMission);

	int missionnum;
	for(missionnum=0, cntMission=0; missionnum < 64; missionnum++){
		if( (inLowFilterMap&((__int64)1<<missionnum)) !=0){
			if(cntMission==number)
                break;
			cntMission++;
		}
	}
	//на выходе missionnum
	//LogMsg("QS - missionSelect:%u,%u\n", number, missionnum);
	if(worldsLst.empty())
		memset(&outMissionGuid, '\0', sizeof(outMissionGuid));
	else
		outMissionGuid=worldsLst.begin()->missionGUID;

	int cnt=0;
	vector<QSWorldInfo>::iterator p;
	for(p=worldsLst.begin(); p!=worldsLst.end(); p++){
		if(cnt==missionnum){
			outMissionGuid=p->missionGUID;
			break;
		}
		cnt++;
	}
	//if(p==worldsLst.end())
    //    LogMsg("QS - error missionSelect - select default\n");
	//else
    //    LogMsg("QS - missionSelect ok\n");
}

bool QSWorldsMgr::isMissionPresent(const XGUID& missionGuid)
{
	vector<QSWorldInfo>::iterator p;
	for(p=worldsLst.begin(); p!=worldsLst.end(); p++){
		if(p->missionGUID == missionGuid)
			return true;
	}
	return false;
}



