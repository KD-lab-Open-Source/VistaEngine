#include "stdafx.h"
#include "NetPlayer.h"
#include "crc.h"
#include "CommonEvents.h"
#include "Serialization.h"
#include "GUIDSerialize.h"
#include "ComboVectorString.h"
#include "LocString.h"
#include "ComboListColor.h"
#include "Runtime.h"
#include "XPrmArchive.h"
#include "MultiArchive.h"
#include "..\Environment\Environment.h"
#include "..\Environment\Anchor.h"
#include "Universe.h"
#include "GlobalAttributes.h"
#include "..\Terra\QSWorldsMgr.h"

// коммент не удалять!  нужно для перевода: _VISTA_ENGINE_EXTERNAL_

const GUID ReplayHeader::FilePlayReelID = { 0x129460bb, 0x93cf, 0x499f, { 0x8a, 0xef, 0x76, 0x77, 0x2b, 0xc7, 0x5e, 0xa0 } };
const int ReplayHeader::FilePlayReelVersion = 11;

enum BitFlag {
	BIT_FLAG_0 = 1 << 0,
	BIT_FLAG_1 = 1 << 1,
	BIT_FLAG_2 = 1 << 2,
	BIT_FLAG_3 = 1 << 3,
	BIT_FLAG_4 = 1 << 4,
	BIT_FLAG_5 = 1 << 5,
	BIT_FLAG_6 = 1 << 6,
	BIT_FLAG_7 = 1 << 7,
	BIT_FLAG_8 = 1 << 8,
	BIT_FLAG_9 = 1 << 9,
	BIT_FLAG_10 = 1 << 10,
	BIT_FLAG_11 = 1 << 11,
	BIT_FLAG_12 = 1 << 12,
	BIT_FLAG_13 = 1 << 13,
	BIT_FLAG_14 = 1 << 14,
	BIT_FLAG_15 = 1 << 15,
	BIT_FLAG_16 = 1 << 16
};

BEGIN_ENUM_DESCRIPTOR(BitFlag, "BitFlag")
REGISTER_ENUM(BIT_FLAG_0, "0")
REGISTER_ENUM(BIT_FLAG_1, "1")
REGISTER_ENUM(BIT_FLAG_2, "2")
REGISTER_ENUM(BIT_FLAG_3, "3")
REGISTER_ENUM(BIT_FLAG_4, "4")
REGISTER_ENUM(BIT_FLAG_5, "5")
REGISTER_ENUM(BIT_FLAG_6, "6")
REGISTER_ENUM(BIT_FLAG_7, "7")
REGISTER_ENUM(BIT_FLAG_8, "8")
REGISTER_ENUM(BIT_FLAG_9, "9")
REGISTER_ENUM(BIT_FLAG_10, "10")
REGISTER_ENUM(BIT_FLAG_11, "11")
REGISTER_ENUM(BIT_FLAG_12, "12")
REGISTER_ENUM(BIT_FLAG_13, "13")
REGISTER_ENUM(BIT_FLAG_14, "14")
REGISTER_ENUM(BIT_FLAG_15, "15")
REGISTER_ENUM(BIT_FLAG_16, "16")
END_ENUM_DESCRIPTOR(BitFlag)


BEGIN_ENUM_DESCRIPTOR(GameType, "GameType")
REGISTER_ENUM(GAME_TYPE_SCENARIO, "Прохождение")
REGISTER_ENUM(GAME_TYPE_BATTLE, "Сражения")
REGISTER_ENUM(GAME_TYPE_REEL, "Ролик")
END_ENUM_DESCRIPTOR(GameType)

PlayerData::PlayerData(int playerIDIn, RealPlayerType realPlayerTypeIn)
{
	teamIndex = 0;
	shuffleIndex = playerID = playerIDIn;
	teamIndex = 0;
	realPlayerType = realPlayerTypeIn;
	colorIndex = GlobalAttributes::instance().playerAllowedColorSize();
	signIndex = GlobalAttributes::instance().playerAllowedSignSize();
	silhouetteColorIndex = GlobalAttributes::instance().playerAllowedSilhouetteSize();
	underwaterColorIndex = GlobalAttributes::instance().playerAllowedUnderwaterColorSize();
	clan = -1;
	flag_playerStartReady = false;
	flag_playerGameLoaded = false;
	//compAndUserID = 0;

	unid.setEmpty();// = 0;

	//randomScenario = false;
	//useBattleTriggers = false;

	strcpy(playerName, "");
}

void PlayerData::readNet(XBuffer& in) 
{ 
	char tmp;
	in.read(&tmp, sizeof(tmp)); playerID=(int)tmp;
	in.read(&tmp, sizeof(tmp)); teamIndex=(int)tmp;
	in.read(&tmp, sizeof(tmp)); shuffleIndex=(int)tmp;
	in.read(&tmp, sizeof(tmp)); realPlayerType=(RealPlayerType)tmp;
	in.read(&tmp, sizeof(tmp)); race.setKey(tmp);
	in.read(&tmp, sizeof(tmp)); colorIndex = tmp;
	in.read(&tmp, sizeof(tmp)); silhouetteColorIndex = tmp;
	in.read(&tmp, sizeof(tmp)); underwaterColorIndex = tmp;
	in.read(&tmp, sizeof(tmp)); signIndex = tmp;
	in.read(&tmp, sizeof(tmp)); clan=(int)tmp;
	in.read(&tmp, sizeof(tmp)); difficulty.setKey(tmp);
	in.read(&flag_playerStartReady, sizeof(flag_playerStartReady));
	in.read(&flag_playerGameLoaded, sizeof(flag_playerGameLoaded));
	//in.read(compAndUserID);
	in.read(&playerName, sizeof(playerName));
	in.read(&unid, sizeof(unid));
}

void PlayerData::writeNet(XBuffer& out) const 
{ 
	char tmp;
	tmp=(char)playerID;			out.write(&tmp, sizeof(tmp));
	tmp=(char)teamIndex;			out.write(&tmp, sizeof(tmp));
	tmp=(char)shuffleIndex;		out.write(&tmp, sizeof(tmp));
	tmp=(char)realPlayerType;		out.write(&tmp, sizeof(tmp));
	tmp=(char)race.key();			out.write(&tmp, sizeof(tmp));
	tmp=(char)colorIndex;			out.write(&tmp, sizeof(tmp));
	tmp=(char)silhouetteColorIndex;out.write(&tmp, sizeof(tmp));
	tmp=(char)underwaterColorIndex;out.write(&tmp, sizeof(tmp));
	tmp=(char)signIndex;			out.write(&tmp, sizeof(tmp));
	tmp=(char)clan;				out.write(&tmp, sizeof(tmp));
	tmp=(char)difficulty.key();	out.write(&tmp, sizeof(tmp));
	out.write(&flag_playerStartReady, sizeof(flag_playerStartReady));
	out.write(&flag_playerGameLoaded, sizeof(flag_playerGameLoaded));
	//out.write(compAndUserID);
	out.write(&playerName, sizeof(playerName));
	out.write(&unid, sizeof(unid));
}

void PlayerData::serialize(Archive& ar) 
{
	ar.serialize(playerID, "playerID", 0);
	ar.serialize(shuffleIndex, "shuffleIndex", 0);
	ar.serialize(realPlayerType, "realPlayerType", 0);
	ar.serialize(race, "race", "Раса");
	if(ar.isEdit()){
		ComboListColor color(GlobalAttributes::instance().playerColors, GlobalAttributes::instance().playerColors[colorIndex]);
		ar.serialize(color, "color", "Цвет");
		colorIndex = color.index();
	}
	else
		ar.serialize(colorIndex, "colorIndex", "Цвет");
	
	if(colorIndex >= (int)GlobalAttributes::instance().playerColors.size())
		colorIndex = GlobalAttributes::instance().playerAllowedColorSize();

	if(ar.isEdit()) {
		ComboListColor color(GlobalAttributes::instance().silhouetteColors, GlobalAttributes::instance().silhouetteColors[silhouetteColorIndex]);
		ar.serialize(color, "silhouetteColor", "Цвет силуэтов");
		silhouetteColorIndex = color.index();
	} else {
		ar.serialize(silhouetteColorIndex, "silhouetteColorIndex", "Цвет силуэтов");
	}

	if(ar.isEdit()) {
		vector<sColor4f> colors;
		vector<UnitColor>::iterator it;
		FOR_EACH(GlobalAttributes::instance().underwaterColors, it)
			colors.push_back(sColor4f(it->color));
		ComboListColor color(colors, colors[underwaterColorIndex]);
		ar.serialize(color, "underwaterColor", "Цвет под водой");
		underwaterColorIndex = color.index();
	} else {
		ar.serialize(underwaterColorIndex, "underwaterColorIndex", "Цвет под водой");
	}

	if(underwaterColorIndex >= (int)GlobalAttributes::instance().underwaterColors.size())
		underwaterColorIndex = GlobalAttributes::instance().playerAllowedUnderwaterColorSize();

	if(ar.isEdit()){
		GlobalAttributes::Signs::const_iterator it;
		vector<string> tmp;
		FOR_EACH(GlobalAttributes::instance().playerSigns, it)
			tmp.push_back(string(it->unitTexture));
		ComboVectorString emblem(tmp, signIndex, true);
		ar.serialize(emblem, "emblem", "Эмблема");
		signIndex = emblem.value() - 1;
	}else
		ar.serialize(signIndex, "signIndex", "Эмблема");

	if(signIndex >= (int)GlobalAttributes::instance().playerSigns.size())
		signIndex = GlobalAttributes::instance().playerAllowedSignSize();

	ar.serialize(clan, "clan", "Клан");
	ar.serialize(difficulty, "difficulty", "Сложность");

	//ar.serialize(flag_playerStartReady, "flag_playerStartReady", 0);
	//ar.serialize(flag_playerGameReady, "flag_playerGameReady", 0);
	//ar.serialize(compAndUserID, "compAndUserID", 0);
	string name = playerName;
	ar.serialize(name, "name", "Имя игрока");
	setName(name.c_str());
}


/////////////////////////////////////////////////////////////////////////////
UserData::UserData()
{
	flag_userConnected=false;
	flag_playerStartReady = false;
	flag_playerGameLoaded = false;
	//compAndUserID = 0;
	playerNameE[0]=0;

	strcpy(playerNameE, "");

	unid.setEmpty();// = 0;
	clientGameCRC = 0;
	lastTimeBackPacket=0;
	requestPause=0;
	clientPause=0;
	timeRequestPause=0;
	lagQuant=0;
	lastExecuteQuant=0;
	curLastQuant=0;
	confirmQuant=0;
	accessibleLogicQuantPeriod=0;
}

SlotData::SlotData(int playerIDIn, RealPlayerType realPlayerTypeIn)
{
	shuffleIndex = playerIDIn; //playerID = 
	realPlayerType = realPlayerTypeIn;
	colorIndex = GlobalAttributes::instance().playerAllowedColorSize();
	signIndex = GlobalAttributes::instance().playerAllowedSignSize();
	silhouetteColorIndex = GlobalAttributes::instance().playerAllowedSilhouetteSize();
	underwaterColorIndex = GlobalAttributes::instance().playerAllowedUnderwaterColorSize();
	clan = -1;

	for(int i = 0; i<NETWORK_TEAM_MAX; i++) 
		usersIdxArr[i]=USER_IDX_NONE;

	//randomScenario = false;
	//useBattleTriggers = false;
}

void SlotData::readNet(XBuffer& in) 
{ 
	char tmp;
	//in.read(&tmp, sizeof(tmp)); playerID=(int)tmp;
	//in.read(&tmp, sizeof(tmp)); cooperativeIndex=(int)tmp;
	in.read(&tmp, sizeof(tmp)); shuffleIndex=(int)tmp;
	in.read(&tmp, sizeof(tmp)); realPlayerType=(RealPlayerType)tmp;
	in.read(&tmp, sizeof(tmp)); race.setKey(tmp);
	in.read(&tmp, sizeof(tmp)); colorIndex = tmp;
	in.read(&tmp, sizeof(tmp)); silhouetteColorIndex = tmp;
	in.read(&tmp, sizeof(tmp)); underwaterColorIndex = tmp;
	in.read(&tmp, sizeof(tmp)); signIndex = tmp;
	in.read(&tmp, sizeof(tmp)); clan=(int)tmp;
	in.read(&tmp, sizeof(tmp)); difficulty.setKey(tmp);
	in.read(&usersIdxArr[0], sizeof(usersIdxArr));
}

void SlotData::writeNet(XBuffer& out) const 
{ 
	char tmp;
	//tmp=(char)playerID;			out.write(&tmp, sizeof(tmp));
	//tmp=(char)cooperativeIndex;			out.write(&tmp, sizeof(tmp));
	tmp=(char)shuffleIndex;		out.write(&tmp, sizeof(tmp));
	tmp=(char)realPlayerType;		out.write(&tmp, sizeof(tmp));
	tmp=(char)race.key();			out.write(&tmp, sizeof(tmp));
	tmp=(char)colorIndex;			out.write(&tmp, sizeof(tmp));
	tmp=(char)silhouetteColorIndex;out.write(&tmp, sizeof(tmp));
	tmp=(char)underwaterColorIndex;out.write(&tmp, sizeof(tmp));
	tmp=(char)signIndex;			out.write(&tmp, sizeof(tmp));
	tmp=(char)clan;				out.write(&tmp, sizeof(tmp));
	tmp=(char)difficulty.key();	out.write(&tmp, sizeof(tmp));
	out.write(&usersIdxArr[0], sizeof(usersIdxArr));
}

void SlotData::logVar() const
{ 
	log_var(shuffleIndex);
	log_var(realPlayerType);
	log_var(race.key());
	log_var(colorIndex);
	log_var(silhouetteColorIndex);
	log_var(underwaterColorIndex);
	log_var(signIndex);
	log_var(clan);
	log_var(difficulty.key());
	log_var(usersIdxArr[0]);
}

bool SlotData::hasFree() const
{
	for(int i = 0; i < NETWORK_TEAM_MAX; ++i)
		if(usersIdxArr[i] == USER_IDX_NONE)
			return true;
	return false;
}

void SlotData::serialize(Archive& ar) 
{
	//ar.serialize(playerID, "playerID", 0); //WRAP_OBJECT(playerID);
	ar.serialize(shuffleIndex, "shuffleIndex", 0); //WRAP_OBJECT(shuffleIndex);
	ar.serialize(realPlayerType, "realPlayerType", 0); //WRAP_OBJECT(realPlayerType);
	ar.serializeArray(usersIdxArr, "usersIdxArr", 0);
	ar.serialize(race, "race", "Раса"); //TRANSLATE_OBJECT(race, "Раса");
	if(ar.isEdit()){
		ComboListColor color(GlobalAttributes::instance().playerColors, GlobalAttributes::instance().playerColors[colorIndex]);
		ar.serialize(color, "color", "Цвет");//TRANSLATE_OBJECT(color, "Цвет");
		colorIndex = color.index();
	}
	else
		ar.serialize(colorIndex, "colorIndex", "Цвет");//TRANSLATE_OBJECT(colorIndex, "Цвет");
	
	if(colorIndex >= (int)GlobalAttributes::instance().playerColors.size())
		colorIndex = GlobalAttributes::instance().playerAllowedColorSize();

	if(ar.isEdit()) {
		ComboListColor color(GlobalAttributes::instance().silhouetteColors, GlobalAttributes::instance().silhouetteColors[silhouetteColorIndex]);
		ar.serialize(color, "silhouetteColor", "Цвет силуэтов");//TRANSLATE_NAME(color, "silhouetteColor", "Цвет силуэтов");
		silhouetteColorIndex = color.index();
	} else {
		ar.serialize(silhouetteColorIndex, "silhouetteColorIndex", "Цвет силуэтов");//TRANSLATE_NAME(silhouetteColorIndex, "silhouetteColorIndex", "Цвет силуэтов");
	}

	if(ar.isEdit()) {
		vector<sColor4f> colors;
		vector<UnitColor>::iterator it;
		FOR_EACH(GlobalAttributes::instance().underwaterColors, it)
			colors.push_back(sColor4f(it->color));
		ComboListColor color(colors, colors[underwaterColorIndex]);
		ar.serialize(color, "underwaterColor", "Цвет под водой");//TRANSLATE_NAME(color, "underwaterColor", "Цвет под водой");
		underwaterColorIndex = color.index();
	} else {
		ar.serialize(underwaterColorIndex, "underwaterColorIndex", "Цвет под водой");//TRANSLATE_NAME(underwaterColorIndex, "underwaterColorIndex", "Цвет под водой");
	}

	if(underwaterColorIndex >= (int)GlobalAttributes::instance().underwaterColors.size())
		underwaterColorIndex = GlobalAttributes::instance().playerAllowedUnderwaterColorSize();

	if(ar.isEdit()){
		GlobalAttributes::Signs::const_iterator it;
		vector<string> tmp;
		FOR_EACH(GlobalAttributes::instance().playerSigns, it)
			tmp.push_back(string(it->unitTexture));
		ComboVectorString emblem(tmp, signIndex, true);
		ar.serialize(emblem, "emblem","Эмблема");//TRANSLATE_OBJECT(emblem, "Эмблема");
		signIndex = emblem.value() - 1;
	}else
		ar.serialize(signIndex, "signIndex", "Эмблема");//TRANSLATE_OBJECT(signIndex, "Эмблема");

	if(signIndex >= (int)GlobalAttributes::instance().playerSigns.size())
		signIndex = GlobalAttributes::instance().playerAllowedSignSize();

	ar.serialize(clan, "clan", "Клан");//TRANSLATE_OBJECT(clan, "Клан");
	ar.serialize(difficulty, "difficulty", "Сложность");//TRANSLATE_OBJECT(difficulty, "Сложность");
}
///////////////////////////////////////////////////
void UserData::readNet(XBuffer& in) 
{
	in.read(&flag_userConnected, sizeof(flag_userConnected)); //при миграции необходимо знать кто первоначально был подключен
	in.read(&flag_playerStartReady, sizeof(flag_playerStartReady));
	in.read(&flag_playerGameLoaded, sizeof(flag_playerGameLoaded));
	//in.read(compAndUserID);
	in.read(&playerNameE, sizeof(playerNameE));
	in.read(&unid, sizeof(unid));
}

void UserData::writeNet(XBuffer& out) const 
{ 
	out.write(&flag_userConnected, sizeof(flag_userConnected)); //при миграции необходимо знать кто первоначально был подключен
	out.write(&flag_playerStartReady, sizeof(flag_playerStartReady));
	out.write(&flag_playerGameLoaded, sizeof(flag_playerGameLoaded));
	//out.write(compAndUserID);
	out.write(&playerNameE, sizeof(playerNameE));
	out.write(&unid, sizeof(unid));
}

void UserData::serialize(Archive& ar) 
{
	ar.serialize(flag_userConnected, "flag_userConnected", 0);
	string name = playerNameE;
	ar.serialize(name, "name", "Имя игрока");
}

//-------------------------------------------------
MissionDescription::MissionDescription(const char* fname, GameType gameType) 
{
	revision_ = VERSION_REVISION;
	gameType_ = gameType;
	playersAmountMax_ = 2;
	auxPlayersAmount_ = 0;
	activeUserIdx_ = 0;
	globalTime = 1;
	gameSpeed = 1;
	gamePaused = false;
	flag_missionDescriptionUpdate = true;
	useMapSettings_ = true;
	triggerFlags_ = 0;

	is_fog_of_war = false;
	is_water = true;
	enableInterface = true;
	enablePause = true;
	is_temperature=false;
	silhouettesEnabled = true;

	userSave_ = false;
	isBattle_ = false;
	errorCode=ErrMD_None;

	worldSize_.x = 0;
	worldSize_.y = 0;

	const GUID guidzero = {0, 0, 0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
	worldGUID = guidzero;
	missionGUID_ = guidzero;

	setChanged();

	if(fname){
		if(!(gameType & GAME_TYPE_REEL))
			setSaveName(fname);
		else
			loadReplayInfoInFile(fname);

		XPrmIArchive ia;
		if(ia.open(saveName(), 10000))
			ia.serialize(*this, "header", 0);
		else
			*this = MissionDescription();

		if(interfaceName_.empty()) // Conversion
			setInterfaceName(saveName());
		
		if(!(gameType & GAME_TYPE_REEL))
			setGameType(gameType, true); 
		else
			loadReplayInfoInFile(fname);
	}
	(int&)gameType_ |= gameType;
}

void MissionDescription::loadReplayInfoInFile(const char* fname)
{
	reelName_ = setExtention(fname, getExtention(GAME_TYPE_REEL)).c_str();
	XStream fi(reelName_.c_str(), XS_IN);
	XBuffer buffer(fi.size());
	fi.read(buffer.buffer(), fi.size());
	ReplayHeader fph;
	fph.read(buffer);
	if(!fph.valid()){
		errorCode=ErrMD_IncorrectReplay;
		return;
	}
	loadReplayInfoInBuf(buffer);
}

void MissionDescription::loadReplayInfoInBuf(XBuffer& buffer)
{
	buffer > StringInWrapper(worldName_) > StringInWrapper(interfaceName_) > StringInWrapper(saveName_); 
	readNet(buffer);
}


void MissionDescription::saveReplay(XBuffer& buffer)
{
	HRESULT resultCreateGUID = CoCreateGuid(&missionGUID_);
	xassert(resultCreateGUID==S_OK);

	buffer < StringOutWrapper(worldName_) < StringOutWrapper(interfaceName_) < StringOutWrapper(saveName_); 
	writeNet(buffer);
}

void MissionDescription::setGameType(GameType gameType, bool force)
{
	if(!force && gameType == gameType_)
		return;

	(int&)gameType_ |= gameType;
	
	if( (gameType_ & GAME_TYPE_REEL) || userSave_)
		return;

	int aiMode = 1;
	check_command_line_parameter("AI", aiMode);

	for(int i = 0; i < NETWORK_PLAYERS_MAX; i++){
		for(int k = 0; k < NETWORK_TEAM_MAX; k++)
			slotsData[i].usersIdxArr[k] = USER_IDX_NONE;

		if(aiMode == 0){
			slotsData[i].realPlayerType = REAL_PLAYER_TYPE_PLAYER;
			slotsData[i].usersIdxArr[0] = i;
			usersData[i].flag_userConnected = true;
		}
		else if(aiMode == 2){
			slotsData[i].realPlayerType = REAL_PLAYER_TYPE_AI;
		}
		else if(gameType & GameType_Multiplayer){
			slotsData[i].realPlayerType = REAL_PLAYER_TYPE_OPEN;
			usersData[i].flag_userConnected = false;
		}
		else{
			if(i==0){
				slotsData[0].usersIdxArr[0] = 0;
				usersData[0].flag_userConnected = true;
				slotsData[i].realPlayerType = REAL_PLAYER_TYPE_PLAYER;
			}
			else 
				slotsData[i].realPlayerType = REAL_PLAYER_TYPE_AI;
		}
	}
	setActivePlayerIdx(0);
}

void MissionDescription::restart() 
{ 
	if(gameType_ & GAME_TYPE_REEL)
		return;
	gameSpeed = 1;
	gamePaused = false;
	userSave_ = false;
	setActivePlayerIdx(0);
	setByWorldName(worldName()); // Original name
}

void MissionDescription::setSaveName(const char* fname) 
{ 
	saveName_ = cutPathToResource(fname);
	saveName_ = setExtention(saveName_.c_str(), "spg");
}

void MissionDescription::setInterfaceName(const char* fname) 
{
	interfaceName_ = setExtention(fname, "");
	size_t pos = interfaceName_.rfind("\\");
	if(pos != string::npos)
		interfaceName_.erase(0, pos + 1);
}

const char* MissionDescription::getExtention(GameType type)
{
	return type & GAME_TYPE_REEL ? "rpl" : "spg";
}

const char* MissionDescription::getMapSizeName(float size)
{
	const MapSizeNames& names = GlobalAttributes::instance().mapSizeNames;
	MapSizeNames::const_iterator it;
	FOR_EACH(names, it){
		if(it->size >= size - FLT_COMPARE_TOLERANCE)
			return it->name.c_str();
	}
	return "";
}

const char* MissionDescription::getMapSizeName() const
{
	return MissionDescription::getMapSizeName(float(worldSize_.x) /1024.f * float(worldSize_.y) /1024.f);
}

void MissionDescription::readNet(XBuffer& in) 
{ 
	//in > StringInWrapper(worldName_) > StringInWrapper(interfaceName_) > StringInWrapper(saveName_); 
	in.read(&missionGUID_, sizeof(missionGUID_));
	in.read(&worldGUID, sizeof(worldGUID));
	for(int i = 0; i < NETWORK_PLAYERS_MAX; i++)
		slotsData[i].readNet(in);
	for(int i = 0; i < NETWORK_PLAYERS_MAX; i++)
		usersData[i].readNet(in);
	unsigned char tmp;
	in.read(&tmp, sizeof(tmp)); useMapSettings_ =(int)tmp;
	in.read(&triggerFlags_, sizeof(triggerFlags_));
	in.read(&tmp, sizeof(tmp)); playersAmountMax_=(int)tmp;
	in.read(&tmp, sizeof(tmp)); auxPlayersAmount_=(int)tmp;
	in.read(&tmp, sizeof(tmp)); gameType_=(GameType)tmp;
	in.read(&tmp, sizeof(tmp)); is_fog_of_war = (int)tmp;
	in.read(&tmp, sizeof(tmp)); is_water = (int)tmp;
	in.read(&tmp, sizeof(tmp)); is_temperature = (int)tmp;
	in.read(&tmp, sizeof(tmp)); activeUserIdx_=(int)tmp;
	in.read(startLocations_, sizeof(startLocations_));
	in.read(&worldSize_, sizeof(worldSize_));
}

void MissionDescription::writeNet(XBuffer& out) const 
{ 
	//out < StringOutWrapper(worldName_) < StringOutWrapper(interfaceName_) < StringOutWrapper(saveName_); 
	out.write(&missionGUID_, sizeof(missionGUID_));
	out.write(&worldGUID, sizeof(worldGUID));
	for(int i = 0; i < NETWORK_PLAYERS_MAX; i++)
		slotsData[i].writeNet(out);
	for(int i = 0; i < NETWORK_PLAYERS_MAX; i++)
		usersData[i].writeNet(out);
	unsigned char tmp;
	tmp=(unsigned char)useMapSettings_;		out.write(&tmp, sizeof(tmp));
	out.write(&triggerFlags_, sizeof(triggerFlags_));
	tmp=(unsigned char)playersAmountMax_;		out.write(&tmp, sizeof(tmp));
	tmp=(unsigned char)auxPlayersAmount_;		out.write(&tmp, sizeof(tmp));
	tmp=(unsigned char)gameType_;				out.write(&tmp, sizeof(tmp));
	tmp=(unsigned char)is_fog_of_war;			out.write(&tmp, sizeof(tmp));
	tmp=(unsigned char)is_water;				out.write(&tmp, sizeof(tmp));
	tmp=(unsigned char)is_temperature;			out.write(&tmp, sizeof(tmp));
	tmp=(unsigned char)activeUserIdx_;			out.write(&tmp, sizeof(tmp));
	out.write(startLocations_, sizeof(startLocations_));
	out.write(&worldSize_, sizeof(worldSize_));
}

void MissionDescription::logVar() const
{
	log_var(worldName_);
	log_var(saveName_); 
	for(int i = 0; i < NETWORK_PLAYERS_MAX; i++)
		slotsData[i].logVar();
	log_var(useMapSettings_);
	log_var(triggerFlags_);
	log_var(playersAmountMax_);
	log_var(auxPlayersAmount_);
	log_var(is_fog_of_war);
	log_var(is_water);
	log_var(is_temperature);
	log_var_crc(startLocations_, sizeof(startLocations_));
	log_var(worldSize_);
}

int MissionDescription::playersAmount() const 
{
	int cntPlayers=0;
	for(unsigned int i=0; i<playersAmountMax(); i++){
		if(!(playerData(i).realPlayerType & REAL_PLAYER_TYPE_EMPTY))
			cntPlayers++;
	}
	return cntPlayers;
}

//void MissionDescription::packPlayerIDs()
//{
//	setChanged();
//	int id = 0;
//	//int i;
//	//for(i = 0; i < NETWORK_PLAYERS_MAX; i++)
//	//	if(!(playersData_[i].realPlayerType & REAL_PLAYER_TYPE_EMPTY))
//	//		playersData_[i].playerID = id++;
//	//for(i = 0; i < NETWORK_PLAYERS_MAX; i++)
//	//	if(playersData_[i].realPlayerType & REAL_PLAYER_TYPE_EMPTY)
//	//		playersData_[i].playerID = id++;
//
//	//int i,curSlotID;
//	//int lastSlotID=0;
//	//bool flag_packSlotID=false;
//	//for(curSlotID=0; curSlotID < playersAmountMax(); curSlotID++){
//	//	int cntSlotID=0;
//	//	for(i = 0; i < NETWORK_PLAYERS_MAX; i++){
//	//		if(!(playersData_[i].realPlayerType & REAL_PLAYER_TYPE_EMPTY))
// //               if(playersData_[i].playerID==curSlotID)
//	//				cntSlotID++;
//	//	}
//	//	if(cntSlotID){
//	//		if(flag_packSlotID){ 
//	//			for(i = 0; i < NETWORK_PLAYERS_MAX; i++){
//	//				if(!(playersData_[i].realPlayerType & REAL_PLAYER_TYPE_EMPTY))
//	//			        if(playersData_[i].playerID==curSlotID)
//	//						playersData_[i].playerID=lastSlotID;
//	//			}
//	//		}
//	//		flag_packSlotID=false;
//	//		lastSlotID++;
//	//	}
//	//	else { //(cntSlotID==0)
//	//		flag_packSlotID=true;
//	//	}
//	//}
//
//	int i;
//	for(i = 0; i < playersAmountMax(); i++)
//		if(!(slotsData[i].realPlayerType & REAL_PLAYER_TYPE_EMPTY))
//			slotsData[i].playerID = id++;
//	for(i = 0; i < playersAmountMax(); i++)
//		if(slotsData[i].realPlayerType & REAL_PLAYER_TYPE_EMPTY)
//			slotsData[i].playerID = id++;
//}

bool MissionDescription::valid() const
{
	if(!isCorrect())
		return false;
	XStream testFile(0);
	if(!testFile.open(saveName(), XS_IN))
		return false;
	else if(!testFile.open(worldData("world.cls").c_str(), XS_IN))
		return false;
	else if(gameType() & GAME_TYPE_REEL && (errorCode == ErrMD_IncorrectReplay || !testFile.open(reelName(), XS_IN)))
		return false;
	return true;
}

void MissionDescription::deleteSave() const 
{
	// удаляем все файлы с таким именем (назависимо от расширения)
	string mask = gameType() & GAME_TYPE_REEL ? setExtention(reelName(), "*") : saveData("*");
	int pos = mask.rfind("\\");
	string dir(mask, 0, pos != string::npos ? pos + 1 : 0); 

	WIN32_FIND_DATA FindFileData;
	HANDLE hf = ::FindFirstFile(mask.c_str(), &FindFileData);
	if(hf != INVALID_HANDLE_VALUE){
		do{
			mask = dir;
			mask += FindFileData.cFileName;
			::DeleteFile(mask.c_str());
		}while(::FindNextFile(hf, &FindFileData));
		::FindClose(hf);
	}
}


void MissionDescription::packCooperativeIdx()
{
	for(int curSlotID=0; curSlotID < playersAmountMax(); curSlotID++){
		packCooperativeIdx(curSlotID);
	}
}

void MissionDescription::packCooperativeIdx(int slotId)
{
	//int i;
	//int lastCoopIdx=0;
	//bool flag_packIdx=false;
	//for(i=0; i<NETWORK_COOPERATIVE_MAX; i++){
		//PlayerData& pd=changePlayerData(slotId, i);
		//if(pd.playerID!=PlayerData::PLAYER_ID_NONE){
		//	if(flag_packIdx){
		//		pd.cooperativeIndex=lastCoopIdx;
		//	}
		//	lastCoopIdx++;
		//	flag_packIdx=false;
		//}
		//else {
		//	flag_packIdx=true;
		//}
	//}
	int cntUser=0;
	SlotData& sd=changePlayerData(slotId);
	for(int j=0; j<NETWORK_TEAM_MAX; j++){
		if(sd.usersIdxArr[j]==USER_IDX_NONE){
			for(int k=j+1; k<NETWORK_TEAM_MAX; k++){
				if(sd.usersIdxArr[k]!=USER_IDX_NONE){
					cntUser++;
					sd.usersIdxArr[j]=sd.usersIdxArr[k];
					sd.usersIdxArr[k]=USER_IDX_NONE;
					break;
				}
			}
		}
		else
			cntUser++;
	}
	if((!cntUser) && (sd.realPlayerType==REAL_PLAYER_TYPE_PLAYER))
		sd.realPlayerType=REAL_PLAYER_TYPE_OPEN;
}

string MissionDescription::saveData(const char* extention) const
{
	return setExtention(saveName(), extention);
}

string MissionDescription::worldData(const char* fileName) const
{
	XBuffer buf;
	buf < vMap.getWorldsDir() < "\\" < worldName_.c_str() < "\\" < fileName;
	return string(buf);
}

void MissionDescription::setByWorldName(const char* worldName)
{
	worldName_ = worldName;
	setSaveName((string(vMap.getWorldsDir()) + "\\" + worldName + "." + getExtention(gameType_)).c_str());
}

void MissionDescription::serialize(Archive& ar)
{
	ar.serialize(revision_, "revision", 0);
	ar.serialize(interfaceName_, "interfaceName", "Интерфейсное имя мира");
	ar.serialize(enableInterface, "enableInterface", "Интерфейс включен");
	ar.serialize(enablePause, "enablePause", "Пауза разрешена");
	ar.serialize(is_fog_of_war, "is_fog_of_war", "Включить туман войны"); // 0 для EXTERNAL @Hallkezz
	ar.serialize(silhouettesEnabled, "silhouettesEnabled", "Разрешить силуэты");
	ar.serialize(is_water, "is_water", "Включить воду");
	ar.serialize(is_temperature, "is_temperature", "Включить замерзание жидкости");
	ar.serialize(isBattle_, "isBattle_", "Мир для сражений");
	ar.serialize(worldName_, "worldName", 0);

	UI_ScreenReference screenToPreload(screenToPreload_);
	ar.serialize(screenToPreload, "screenToPreload", "Экран интерфейса для предзагрузки");
	screenToPreload_ = screenToPreload.referenceString();

	is_water = true; // !!!
	
	ar.serialize(missionDescription_, "missionDescriptionLoc", "Описание миссии (лок)");

	ar.serialize(playersAmountMax_, "playersAmountMax", 0);
	ar.serialize(auxPlayersAmount_, "auxPlayersAmount", 0);

	if(!ar.serializeArray(slotsData, "slotsData", 0)){
		//convert
		PlayerData playersData_[NETWORK_PLAYERS_MAX];
		ar.serializeArray(playersData_, "playersData", 0);
		for(int i=0; i<NETWORK_PLAYERS_MAX; i++){
			slotsData[i].clan=playersData_[i].clan;
			slotsData[i].colorIndex=playersData_[i].colorIndex;
			slotsData[i].difficulty=playersData_[i].difficulty;
			slotsData[i].race=playersData_[i].race;
			slotsData[i].realPlayerType=playersData_[i].realPlayerType;
			slotsData[i].shuffleIndex=playersData_[i].shuffleIndex;
			slotsData[i].signIndex=playersData_[i].signIndex;
			slotsData[i].silhouetteColorIndex=playersData_[i].silhouetteColorIndex;
			slotsData[i].underwaterColorIndex=playersData_[i].underwaterColorIndex;
			slotsData[i].usersIdxArr[0]=i;
			memcpy(usersData[i].playerNameE, playersData_[i].playerName, sizeof(usersData[i].playerNameE));
			usersData[i].playerNameE[sizeof(usersData[i].playerNameE)-1]=0;
		}
	}
    ar.serializeArray(usersData, "usersData", 0);
	
	ar.serialize(activeUserIdx_, "activeUserIdx", 0); 
	ar.serialize(userSave_, "userSave", 0); 
	ar.serialize(useMapSettings_, "useMapSettings", 0); 
	if(ar.isEdit()){
		BitVector<BitFlag> flags = triggerFlags_;
		ar.serialize(flags, "triggerFlags", "Переменные условий победы/поражения"); 
		triggerFlags_ = flags;
	}
	else
		ar.serialize(triggerFlags_, "triggerFlags", 0); 
		
	ar.serialize(globalTime, "globalTime", 0);
	ar.serialize(gameSpeed, "gameSpeed", 0);
	ar.serialize(gamePaused, "gamePaused", 0);

	ar.serialize(worldSize_, "worldSize",0);
	ar.serializeArray(startLocations_, "startLocations", 0);

	///  CONVERSION 2006-10-18
	if(!ar.serialize(GUIDSerialization(worldGUID), "worldGUID", 0)){
		ar.serialize(worldGUID.Data1, "worldGUID_Data1", 0);
		ar.serialize(worldGUID.Data2, "worldGUID_Data2", 0);
		ar.serialize(worldGUID.Data3, "worldGUID_Data3", 0);
		ar.serializeArray(worldGUID.Data4, "worldGUID_Data4", 0);
	}
	if(!ar.serialize(GUIDSerialization(missionGUID_), "missionGUID", 0)){
		ar.serialize(missionGUID_.Data1, "missionGUID_Data1", 0);
		ar.serialize(missionGUID_.Data2, "missionGUID_Data2", 0);
		ar.serialize(missionGUID_.Data3, "missionGUID_Data3", 0);
		ar.serializeArray(missionGUID_.Data4, "missionGUID_Data4", 0);
	}
	/// ^^^^^

#ifndef _FINAL_VERSION_
	if(check_command_line("ignoreUserSave"))
		userSave_ = false;
#endif
}

int MissionDescription::findFreeUserIdx()
{
	for(int i=0; i<NETWORK_PLAYERS_MAX; i++)
		if(!usersData[i].flag_userConnected)
			return i;
	return USER_IDX_NONE;
}

void MissionDescription::clearSlotAndUserData()
{
	for(int i = 0; i < NETWORK_PLAYERS_MAX; i++){
		slotsData[i].realPlayerType = REAL_PLAYER_TYPE_OPEN;
		usersData[i].flag_userConnected = false;
		for(int k = 0; k < NETWORK_TEAM_MAX; k++)
			slotsData[i].usersIdxArr[k] = USER_IDX_NONE;
	}

}
void MissionDescription::insertPlayerData(const PlayerDataEdit& data, bool flag_active)
{
	if(playersAmountMax_ < NETWORK_PLAYERS_MAX){
		int curSlotIdx = playersAmountMax_++;
		changePlayerData(curSlotIdx)=data;
		for(int k = 0; k < NETWORK_TEAM_MAX; k++)
			slotsData[curSlotIdx].usersIdxArr[k] = USER_IDX_NONE;
		if(slotsData[curSlotIdx].realPlayerType == REAL_PLAYER_TYPE_PLAYER){
			int uidx = findFreeUserIdx();
			if(uidx!=USER_IDX_NONE){
				slotsData[curSlotIdx].usersIdxArr[0]=uidx;
				usersData[uidx].flag_userConnected = true;
				usersData[uidx].setName(data.name());
				if(flag_active)
					activeUserIdx_=uidx;
			}
		}
	}
}

void MissionDescription::resetToSave(bool userSave)
{
	userSave_ = userSave;

	revision_ = VERSION_REVISION;

	if(!userSave)
		useMapSettings_ = true;

	clearSlotAndUserData();
	playersAmountMax_ = 0;
	auxPlayersAmount_ = 0;
	PlayerVect::const_iterator pi;
	FOR_EACH(universe()->Players, pi)
		if(!(*pi)->isWorld()){
			if((*pi)->auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER){
				PlayerDataEdit data;
				(*pi)->getPlayerData(data);
				//changePlayerData(playersAmountMax_++) = data;
				insertPlayerData(data, (*pi)->active());
			}
			else
				auxPlayersAmount_++;
		}

	xassert(vMap.isWorldLoaded());
	worldGUID = vMap.guid;
	HRESULT resultCreateGUID = CoCreateGuid(&missionGUID_);
	xassert(resultCreateGUID==S_OK);

	if(userSave_){
		setInterfaceName(saveName());
		return;
	}

	activeUserIdx_ = 0;
	for(int i = 0; i < NETWORK_PLAYERS_MAX; i++)
		slotsData[i].shuffleIndex = i;

	string name = saveName();
	strlwr((char*)name.c_str());
	if(vMap.isWorldLoaded())
		worldSize_ = Vect2s(vMap.H_SIZE, vMap.V_SIZE);

	int number = 0;
	Environment::Anchors::const_iterator it;
	FOR_EACH(environment->anchors(), it)
		if((*it)->type() == Anchor::START_LOCATION){
			startLocations_[number] = (const Vect2f&)(*it)->pose().trans();
			if(++number == NETWORK_PLAYERS_MAX)
				break;
		}

	while(number < NETWORK_PLAYERS_MAX)
		startLocations_[number++].set(worldSize_.x/2, worldSize_.y/2);
}

//void MissionDescription::setActivePlayerID(int playerID, int cooperativeIndex)
//{ 
//	setChanged(); 
//	xassert(playerID < min(playersAmountMax()-1, NETWORK_PLAYERS_MAX) ); 
//	xassert(cooperativeIndex < NETWORK_COOPERATIVE_MAX);
//	int i;
//	for(i=0; i<NETWORK_PLAYERS_MAX; i++){
//		if(playersData_[i].playerID==playerID && playersData_[i].cooperativeIndex==cooperativeIndex){
//			activePlayerIdx_=i;
//			break;
//		}
//	}
//	if(i==NETWORK_PLAYERS_MAX){
//		xassert(0 && "playerID&cooperativeIndex not found!");
//		activePlayerIdx_=0;
//	}
//}

int MissionDescription::findSlotIdx(int userIdx) const
{
	if(userIdx <0 || userIdx >= NETWORK_PLAYERS_MAX)
		return PLAYER_ID_NONE;
	for(int i=0; i<playersAmountMax(); i++)
		for(int j=0; j<NETWORK_TEAM_MAX; j++)
			if(slotsData[i].usersIdxArr[j]==userIdx)
				//return slotsData[i].playerID;
				return i;
	//xassert(0&&"UserID not found!");
	return PLAYER_ID_NONE;
}

int MissionDescription::findCooperativeIdx(int userIdx) const
{
	for(int i=0; i<playersAmountMax(); i++)
		for(int j=0; j<NETWORK_TEAM_MAX; j++)
			if(slotsData[i].usersIdxArr[j]==userIdx)
				return j;
	//xassert(0&&"UserID not found!");
	return USER_IDX_NONE;
}

int MissionDescription::findPlayerIndex(int slotIdx) const
{
	if(slotIdx >= playersAmountMax() || !(slotsData[slotIdx].realPlayerType & (REAL_PLAYER_TYPE_PLAYER | REAL_PLAYER_TYPE_AI)))
		return -1;
	int decrement = 0;
	for(int i = 0; i < slotIdx; i++)
		if(!(slotsData[i].realPlayerType & (REAL_PLAYER_TYPE_PLAYER | REAL_PLAYER_TYPE_AI)))
			decrement++;
	return slotIdx - decrement;
}

void MissionDescription::setActivePlayerIdx(int _activeUserIdx)
{ 
	setChanged(); 
	xassert(_activeUserIdx>=0 && _activeUserIdx < NETWORK_PLAYERS_MAX);
	activeUserIdx_=_activeUserIdx;
}

//void MissionDescription::setActivePlayer(int playerID)
//{
//	for(int i = 0; i < playersAmountMax(); i++)
//		if(slotsData[i].realPlayerType == REAL_PLAYER_TYPE_PLAYER)
//			slotsData[i].realPlayerType = REAL_PLAYER_TYPE_AI;
//	changePlayerData(playerID).realPlayerType = REAL_PLAYER_TYPE_PLAYER;
//	int aui=playerData(playerID).usersIdxArr[0];
//	xassert(aui>=0 && aui<NETWORK_PLAYERS_MAX);
//	xassert(usersData[aui].flag_userConnected);
//	setActivePlayerIdx(playerData(playerID).usersIdxArr[0]);
//}
void MissionDescription::switchActiveUser2OtherSlot(int slotIdx)
{
	xassert(slotIdx < playersAmountMax());
	int oldSlotIdx=findSlotIdx(activeUserIdx_);
	if(oldSlotIdx!=slotIdx){
		xassert(slotsData[oldSlotIdx].realPlayerType==REAL_PLAYER_TYPE_PLAYER);
		xassert(slotsData[slotIdx].realPlayerType==REAL_PLAYER_TYPE_AI);
		//for(int i=0; i<NETWORK_TEAM_MAX; i++){
		//	swap(slotsData[oldSlotIdx].usersIdxArr[i], slotsData[slotIdx].usersIdxArr[i]);
		//}
		//swap(slotsData[oldSlotIdx].realPlayerType, slotsData[slotIdx].realPlayerType);
		for(int i=0; i<NETWORK_TEAM_MAX; i++){
			slotsData[oldSlotIdx].usersIdxArr[i]=USER_IDX_NONE;
			slotsData[slotIdx].usersIdxArr[i]=USER_IDX_NONE;
		}
		slotsData[oldSlotIdx].realPlayerType=REAL_PLAYER_TYPE_AI;
		slotsData[slotIdx].realPlayerType=REAL_PLAYER_TYPE_PLAYER;
		slotsData[slotIdx].usersIdxArr[0]=activeUserIdx_;
	}
}

void MissionDescription::shufflePlayers()
{ 
	if(userSave())
		return;
	if(useMapSettings_){
		for(int i = 0; i < playersAmountMax(); i++)
			//playersData_[i].shuffleIndex = i;
			slotsData[i].shuffleIndex = i;
	}
	else{
		int indices[NETWORK_PLAYERS_MAX];
		for(int i = 0; i < playersAmountMax(); i++)
			indices[i] = i;
		srand(timeGetTime());
		random_shuffle(&indices[0], &indices[0] + playersAmountMax());
		//for(int i = 0; i < playersAmountMax(); i++)
		//	playersData_[i].shuffleIndex = indices[i];
		for(int i = 0; i < playersAmountMax(); i++)
			slotsData[i].shuffleIndex = indices[i];
	}
}

const SlotData& MissionDescription::playerData(int playerID) const 
{ 

	xassert(playerID>=0 && playerID<playersAmountMax()); 
	//return playersData_[index + cooperativeIndex*NETWORK_PLAYERS_MAX]; 
	//for(int i=0; i<playersAmountMax(); i++){
	//	if(slotsData[i].playerID==playerID){
	//		return slotsData[i];
	//	}
	//}
	//xassert(0 && "playerID not found!");
	//static SlotData sd;
	//sd.playerID=PlayerData::PLAYER_ID_NONE;
	//return sd;//playersData_[NETWORK_PLAYERS_MAX-1];
	return slotsData[playerID];
}

SlotData& MissionDescription::changePlayerData(int playerID) 
{ 
	setChanged(); 
	//xassert(index<NETWORK_PLAYERS_MAX); 
	//return playersData_[index + cooperativeIndex*NETWORK_PLAYERS_MAX]; 
	return const_cast<SlotData&> (playerData(playerID));
}

PlayerData& PlayerData::operator = (const SlotData& inData)
{
	static_cast<SlotData&>(*this)=inData;
	flag_playerStartReady=0;
	flag_playerGameLoaded=0;
	//compAndUserID=0;
	unid.setEmpty();//=0;
	playerName[0]=0;

	//randomScenario=0;
	//useBattleTriggers=0;

	return *this;
}

const PlayerData& MissionDescription::constructPlayerData(int _playerID, int cooperativeIndex, int newPlayerID) const
{
	static PlayerData spd;
	xassert(_playerID>=0 && _playerID<playersAmountMax()); 
	xassert(cooperativeIndex>=0 && cooperativeIndex<NETWORK_TEAM_MAX);
	spd=slotsData[_playerID];
	spd.playerID = newPlayerID==PLAYER_ID_NONE ? _playerID : newPlayerID;
	spd.teamIndex=cooperativeIndex;
	if(slotsData[_playerID].usersIdxArr[cooperativeIndex]!=USER_IDX_NONE)
		spd.setName(usersData[slotsData[_playerID].usersIdxArr[cooperativeIndex]].playerNameE);
	else
        spd.setName("");
	return spd;
}

const UserData& MissionDescription::getUserData(int _playerID, int _cooperativeIndex) const
{
	xassert(_playerID>=0 && _playerID<playersAmountMax()); 
	xassert(_cooperativeIndex>=0 && _cooperativeIndex<NETWORK_TEAM_MAX);
	if(_playerID>=0 && _playerID<playersAmountMax() && _cooperativeIndex>=0 && _cooperativeIndex<NETWORK_TEAM_MAX &&
		slotsData[_playerID].usersIdxArr[_cooperativeIndex]!=USER_IDX_NONE && 
		slotsData[_playerID].usersIdxArr[_cooperativeIndex]>=0 &&
		slotsData[_playerID].usersIdxArr[_cooperativeIndex]<NETWORK_PLAYERS_MAX ){
			return usersData[slotsData[_playerID].usersIdxArr[_cooperativeIndex]];
	}
	else {
		static UserData usd;
		return usd;
	}
}


const char* MissionDescription::playerName(int slotID, int cooperativeIdx) const 
{ 
	const SlotData& sd=playerData(slotID);
	//if(sd.playerID!=PlayerData::PLAYER_ID_NONE){
		int idx=sd.usersIdxArr[cooperativeIdx];
		if(idx!=USER_IDX_NONE){
			return usersData[idx].playerNameE;
		}
	//}
	return "";
}
void MissionDescription::setPlayerName(int slotID, int cooperativeIdx, const char* str)
{ 
	const SlotData& sd=playerData(slotID);
	//if(sd.playerID!=PlayerData::PLAYER_ID_NONE){
		int idx=sd.usersIdxArr[cooperativeIdx];
		if(idx!=USER_IDX_NONE){
			usersData[idx].setName(str);
		}
	//}
}

void MissionDescription::getDebugInfo(XBuffer& out) const
{
	if(isLoaded()){
		out < interfaceName() < ", WName: " < worldName_.c_str();
		out < ", SName: " < saveName_.c_str() < ", RName" < reelName_.c_str();
		out < " \n GameType: ";
		out < (gameType_ & GAME_TYPE_REEL ? "Replay+" : "");
		switch(gameType_ & GameType_MASK){
		case GAME_TYPE_SCENARIO:
			out < "Scenario";
			break;
		case GAME_TYPE_BATTLE:
			out < "Battle";
			break;
		case GAME_TYPE_MULTIPLAYER:
			out < "Multiplayer";
			break;
		case GAME_TYPE_MULTIPLAYER_COOPERATIVE:
			out < "Multiplayer+Cooperative";
			break;
		}
		out < ", useMapSettings: " <= useMapSettings() < ", isBattle: " <= isBattle() < ", isUserSave: " <= userSave()
			< ", sizeX=" <= worldSize_.x < ", sizeY=" <= worldSize_.y;
	}
	else
		out < "EMPTY";
}

ReplayHeader::ReplayHeader()
{
	ID=FilePlayReelID;
	version=FilePlayReelVersion;
	universeSignature = librarySignature = worldSignature = endQuant = 0;
}

void ReplayHeader::read(XBuffer& in)
{
	in.read(ID);
	in.read(version);
	in.read(universeSignature);
	in.read(librarySignature);
	in.read(worldSignature);
	in.read(endQuant);
}

void ReplayHeader::write(XBuffer& out)
{
	out.write(ID);
	out.write(version);
	out.write(universeSignature);
	out.write(librarySignature);
	out.write(worldSignature);
	out.write(endQuant);
}
bool ReplayHeader::valid() const 
{
	return version == FilePlayReelVersion && ID == FilePlayReelID; 
}

//////////////////////////////////////////////////////////////////////////
void MissionDescriptions::readFromDir(const char* path, GameType gameType)
{
	clear();

	std::string fixedPath = path;
	if(!fixedPath.empty() && fixedPath[fixedPath.size() - 1] != '\\')
		fixedPath += "\\";

	std::string mask = fixedPath + "*." + MissionDescription::getExtention(gameType);

	WIN32_FIND_DATA ffd;
	HANDLE hf = FindFirstFile(mask.c_str(), &ffd);

	if(hf != INVALID_HANDLE_VALUE){
		do {
			if(ffd.nFileSizeLow){
				string name = fixedPath;
				name += ffd.cFileName;
				MissionDescription mission(name.c_str(), gameType);
				if(mission.valid())
					push_back(mission);
			}
		} while(FindNextFile(hf, &ffd));
		FindClose(hf);
	}
}

void MissionDescriptions::readUserWorldsFromDir(const char* path)
{
	clear();
	std::string fixedPath = path;
	if(!fixedPath.empty() && fixedPath[fixedPath.size() - 1] != '\\')
		fixedPath += "\\";

	std::string mask = fixedPath + "*." + MissionDescription::getExtention(GAME_TYPE_MULTIPLAYER);

	WIN32_FIND_DATA ffd;
	HANDLE hf = FindFirstFile(mask.c_str(), &ffd);

	if(hf != INVALID_HANDLE_VALUE){
		do {
			if(ffd.nFileSizeLow){
				string name = fixedPath;
				name += ffd.cFileName;
				MissionDescription mission(name.c_str(), GAME_TYPE_MULTIPLAYER);
				if(mission.isBattle() && !qsWorldsMgr.isMissionPresent(mission.missionGUID()))
					push_back(mission);
			}
		} while(FindNextFile(hf, &ffd));
		FindClose(hf);
	}
}

void MissionDescriptions::add(const MissionDescription& mission)
{
	iterator i;
	FOR_EACH(*this, i)
		if(!stricmp(mission.interfaceName(), i->interfaceName())){
			*i = mission;
			return;
		}
	push_back(mission);
}

void MissionDescriptions::remove(const MissionDescription& mission)
{
	iterator i;
	FOR_EACH(*this, i)
		if(!stricmp(mission.interfaceName(), i->interfaceName())){
			erase(i);
			return;
		}
}

const MissionDescription* MissionDescriptions::find(const char* missionName) const
{
	const_iterator i;
	FOR_EACH(*this, i)
		if(!stricmp(missionName, i->interfaceName()))
			return &*i;
	return 0;
}

const MissionDescription* MissionDescriptions::find(const GUID& id) const
{
	const_iterator i;
	FOR_EACH(*this, i)
		if(i->missionGUID()==id)
			return &*i;
	return 0;
}

int MissionDescription::gamePopulation() const
{
	StaticMap<int, int> clans;
	for(int i = 0; i < playersAmountMax(); i++){
		const SlotData& slot = slotsData[i];
		if(slot.realPlayerType == REAL_PLAYER_TYPE_AI)
			return -1;
		if(slot.realPlayerType == REAL_PLAYER_TYPE_PLAYER){
			if(!clans.exists(slot.clan))
				clans[slot.clan] = 0;
			++clans[slot.clan];
		}
	}

	if(clans.size() < 2)
		return -1;
	if(clans.size() > 2){
		for(int i = 0; i < clans.size(); i++)
			if(clans[i] > 1)
				return -1;
		return 0;
	}

	int size = clans.begin()[0].second;
	if(size == clans.begin()[1].second){
		xassert(size < 4);
		return size;
	}
	else 
		return -1;
}
