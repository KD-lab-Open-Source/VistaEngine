#include "StdAfx.h"

//#include "Region.h"
#include "CameraManager.h"
#include "SoundApp.h"
#include "GameShell.h"
#include "Squad.h"
#include "BaseUnit.h"
#include "UnitItemInventory.h"
#include "UnitItemResource.h"
#include "IronBuilding.h"
#include "IronBullet.h"
#include "UniverseX.h"

#include "CheatManager.h"
#include "controls.h"

#include "RenderObjects.h"

#include "P2P_interface.h"
#include "..\Water\Water.h"
#include "..\Util\Console.h"
#include "DebugPrm.h"
#include "..\Render\src\postEffects.h"
#include "..\Environment\Environment.h"

#include "Sound.h"
#include "SoundSystem.h"
#include "terra.h"

#include "ExternalShow.h"
#include "Triggers.h"
//#include "..\TriggerEditor\TriggerEditor.h"

#include "TextDB.h"
#include "..\ht\ht.h"
#include "PlayBink.h"
#include "SelectManager.h"

#include "EditArchive.h"
#include "XPrmArchive.h"
#include "SoundScript.h"
#include "..\Util\ConsoleWindow.h"

#include "UI_Render.h"
#include "UI_BackgroundScene.h"
#include "UserInterface.h"
#include "UI_Logic.h"
#include "UI_NetCenter.h"
#include "UI_CustomControls.h"
#include "GameOptions.h"
#include "CommonLocText.h"
#include "ShowHead.h"

#include "Lmcons.h"

#include "PFTrap.h"
#include "..\Water\CircleManager.h"
#include "UI_Logic.h"

#include "..\physics\crash\CrashSystem.h"

#include "..\Water\SkyObject.h"

#include "StreamCommand.h"
#include "..\Render\3dx\Lib3dx.h"
#include "..\Render\inc\IVisD3D.h"
#undef XREALLOC
#undef XFREE

#include "LogMsg.h"


extern float HardwareCameraFocus;

int terShowFPS = 0;

const char* terScreenShotsPath = "ScreenShots";
const char* terScreenShotName = "shot";
const char* terScreenShotExt = ".bmp";
const char* terMoviePath = "Movie";
const char* terMovieName = "Track";
const char* terMovieFrameName = "frame";

void updateResolution(int sx, int sy, bool change_size);

const char* COMMAND_LINE_MULTIPLAYER_HOST="mphost";
const char* COMMAND_LINE_MULTIPLAYER_CLIENT="mpclient";
const char* COMMAND_LINE_PLAY_REEL="reel";
const char* COMMAND_LINE_DW_HOST="dwhost";
const char* COMMAND_LINE_DW_CLIENT="dwclient";

//Scores scoresArray_[NETWORK_PLAYERS_MAX];

void abortWithMessage(const string& messageID) 
{
	FinitSound();
	ErrH.Abort(messageID.c_str());
}

void ErrorInitialize3D() {
	abortWithMessage(GET_LOC_STR(UI_COMMON_TEXT_ERROR_GRAPH_INIT));
}

void checkGameSpyCmdLineArg(const char* argument) {
	if (!argument) {
		abortWithMessage("Interface.Menu.Messages.GameSpyCmdLineError");
	}
}

//XBuffer qslog(1024*1024, true);

//------------------------
GameShell::GameShell() :
	NetClient(0), 
	windowClientSize_(800, 600),
	globalTrigger_(*new TriggerChain)
{
	lock_logic=&gb_VisGeneric->GetLockResetDevice();
	gameShell = this;
	eventProcessing_ = false;

	controlEnabled_ = true;
	isCutScene_ = false;

	currentMissionPopulation_ = 0;
	ratingDelta_ = 0;

	setLogicFp();

	needLogicCall_ = true;
	interpolation_timer_ = 0;
	sleepGraphicsTime_ = 0;
	sleepGraphics_ = false;

	gameReadyCounter_ = 0;

	terminateMission_ = false;
	startMissionSuspended_ = false;
	stopNetClientSuspended = false;

	logicFps_ = logicFpsMin_ = logicFpsMax_ = logicFpsTime_ = 1;

	if(!IniManager("Game.ini").getInt("Timer","SynchroByClock", synchroByClock_))
		synchroByClock_ = true;
	if(!IniManager("Game.ini").getInt("Timer","StandartFrameRate", StandartFrameRate_))
		StandartFrameRate_ = 24;
	framePeriod_ = 1000/StandartFrameRate_;

	check_command_line_parameter("synchro_by_clock", synchroByClock_);

	if(const char* s = check_command_line("fps"))
		framePeriod_ = 1000/atoi(s);

	global_time.set(0,logicTimePeriod);
	if(check_command_line("synchro_by_clock"))
		synchroByClock_ = 1;
	frame_time.set(synchroByClock_, framePeriod_, terMaxTimeInterval);
	scale_time.set(synchroByClock_, framePeriod_, terMaxTimeInterval);

	profilerStarted_ = false;

	///setSpeed(IniManager("Game.ini").getFloat("Game", "GameSpeed"));

	soundPushedByPause = false;
	soundPushedPushLevel=INT_MIN;

	globalTrigger_.load("Scripts\\Content\\GlobalTrigger.scr");

	alwaysRun_ = check_command_line("active");
	GameActive = false;
	GameContinue = true;

	disableVideo_ = false;
	IniManager("Game.ini").getBool("Game", "DisableVideo", disableVideo_);

	reelManager.sizeType = ReelManager::FULL_SCREEN;

	reelAbortEnabled = true;

	mainMenuEnabled_ = true;
	IniManager("Game.ini").getBool("Game", "MainMenu", mainMenuEnabled_);

	debug_allow_replay = 0;
	IniManager("Game.ini", false).getInt("Game", "EnableReplay", debug_allow_replay);

	check_command_line_parameter("mainmenu", mainMenuEnabled_);

	shotNumber_ = -1;
	recordMovie_ = false;
	movieShotNumber_ = 0;

	game_speed = 1;
	pauseType_ = 0;

	countDownTimeLeft = "";
	countDownTimeMillisLeft = -1;
	countDownTimeMillisLeftVisible = -1;
	
	cameraMouseTrack = false;
	cameraMouseZoom  = false;
	selectMouseTrack = false;

	directControl_ = DIRECT_CONTROL_DISABLED;
	
	MouseMoveFlag = 0;
	MousePositionLock = 0;
	
	mousePosition_ = Vect2f::ZERO;
	mousePositionDelta_ = Vect2f::ZERO;
	
	activePlayerID_ = 0;
	
	cameraCursor_ = 0;

	debugFont_ = gb_VisGeneric->CreateFont(default_font_name.c_str(), 15);
	gb_VisGeneric->SetUseTextureCache(true);
	gb_VisGeneric->SetUseMeshCache(true);

	EffectContainer::setTexturesPath("Resource\\FX\\Textures");
	for(EffectLibrary::Strings::const_iterator it = EffectLibrary::instance().strings().begin(); it != EffectLibrary::instance().strings().end(); ++it)
		if(it->get())
			it->get()->preloadLibrary();

	UI_Dispatcher::instance().init();

	Universe::setProgressCallback(loadProgressUpdate);

	const char* playerName=check_command_line("playerName");
	if(check_command_line("gamespy")) {
		const char* strPassword=check_command_line("password");
		if(strPassword==0) strPassword="";
		checkGameSpyCmdLineArg(playerName);
		////::MessageBox(0, playerName, "MSG CommandLine", MB_OK);
		const char* strIP=check_command_line("ip");
		if(check_command_line("host")){
			const char* roomName=check_command_line("room");
			checkGameSpyCmdLineArg(roomName);
			////::MessageBox(0, roomName, "MSG CommandLine", MB_OK);
//			startOnline(CommandLineData(true, playerName, false, "", GUID(), roomName, strPassword));
		}
		else if(check_command_line("client")){
			checkGameSpyCmdLineArg(strIP);
//			startOnline(CommandLineData(false, playerName, false, strIP, GUID(), "", strPassword));
		}
	}
	else if(check_command_line("p2p")){
		const char* strIP=check_command_line("ip");
		checkGameSpyCmdLineArg(playerName);
		if(check_command_line("host")){
//			startOnline(CommandLineData(true, playerName, true, "", GUID()));
		}
		else if(check_command_line("client")){
			checkGameSpyCmdLineArg(strIP);
//			startOnline(CommandLineData(false, playerName, true, strIP, GUID()));
		}
	}

	if(!mainMenuEnabled_ || check_command_line("open") || check_command_line("world") || check_command_line(COMMAND_LINE_PLAY_REEL)){
		string name = "Resource\\";

		if(check_command_line("open"))
			name = check_command_line("open");

		if(check_command_line("world"))
			name = string("Resource\\Worlds\\") + check_command_line("world");

		if(name == "")
			name = "XXX";

		GameType gameType = GAME_TYPE_SCENARIO;
		if(check_command_line("battle"))
			gameType = GAME_TYPE_BATTLE;

		name = setExtention(name.c_str(), MissionDescription::getExtention(gameType));

		if(check_command_line(COMMAND_LINE_PLAY_REEL)){
			const char* fname=check_command_line(COMMAND_LINE_PLAY_REEL);
			HTManager::instance()->GameStart(MissionDescription(fname, GAME_TYPE_REEL));
		}
		else {
			if( !check_command_line(COMMAND_LINE_MULTIPLAYER_CLIENT) && !XStream(0).open(name.c_str()) ){
				void RestoreGDI();
				RestoreGDI();
				if(openFileDialog(name, "Resourse\\Missions", MissionDescription::getExtention(gameType), "Mission Name")){
					size_t pos = name.rfind("RESOURCE\\");
					if(pos != string::npos)
						name.erase(0, pos);
				}
				else{
					GameContinue = false;
					return;
				}
				if(gb_RenderDevice)
					gb_RenderDevice->RestoreDeviceForce();
			}
			if(check_command_line(COMMAND_LINE_MULTIPLAYER_HOST) || check_command_line(COMMAND_LINE_MULTIPLAYER_CLIENT) )
				startMPWithoutInterface(name.c_str());
			else if(check_command_line(COMMAND_LINE_DW_HOST)|| check_command_line(COMMAND_LINE_DW_CLIENT))
				startDWMPWithoutInterface(name.c_str());
			else{

				MissionDescription missionDescription(name.c_str(), gameType);

				if(check_command_line("battle"))
					missionDescription.setUseMapSettings(false);

				HTManager::instance()->GameStart(missionDescription);
			}

			if(check_command_line("convert")){
				universalSave(name.c_str(), false);
				FinitSound();
				GameContinue = false;
				return;
			}
		}
	}
#ifndef _FINAL_VERSION_
	const char* pqstart=check_command_line("profile_start");
	if(pqstart){
		int profileStartQuant=atoi(pqstart);
		const char* pqend=check_command_line("profile_end");
		if(pqend){
			int profileEndQuant=atoi(pqend);
			xassert(profileStartQuant < profileEndQuant);
			Profiler::instance().setAutoMode(profileStartQuant, profileEndQuant, CurrentMission.interfaceName(), "profile", true);
		}
	}
#endif
}

void GameShell::terminate()
{
	UI_Dispatcher::instance().exitGamePrepare();
	GameContinue = false;
}

void GameShell::startMPWithoutInterface(const char* missionName)
{
	const int BUF_CN_SIZE=MAX_COMPUTERNAME_LENGTH + 1;
	DWORD cns = BUF_CN_SIZE;
	char cname[BUF_CN_SIZE];
	::GetComputerName(cname, &cns);
	const int BUF_UN_SIZE=UNLEN + 1;
	DWORD uns = BUF_UN_SIZE;
	char username[BUF_UN_SIZE];
	::GetUserName(username, &uns);

	if(check_command_line(COMMAND_LINE_MULTIPLAYER_HOST)){
		int numPlayers=atoi(check_command_line(COMMAND_LINE_MULTIPLAYER_HOST));
		createNetClient(PNCWM_LAN);
		getNetClient()->Configurate();
		string playerName=string("Host_")+cname+"_"+username;
		getNetClient()->CreateGame("HostGame", MissionDescription(missionName, GAME_TYPE_MULTIPLAYER), playerName.c_str(), Race(), 0, 1, "");
		while(getNetClient()->getCurrentMissionDescription().playersAmount() < numPlayers){
			getNetClient()->quant_th1();
			::Sleep(40);
		}
		getNetClient()->StartLoadTheGame();
	}
	else if(check_command_line(COMMAND_LINE_MULTIPLAYER_CLIENT)){
		string playerName=string("Client_")+cname+"_"+username;
		string ipstr=check_command_line(COMMAND_LINE_MULTIPLAYER_CLIENT);
		createNetClient(PNCWM_LAN);
		getNetClient()->Configurate();
		if(ipstr==""){
			getNetClient()->ResetAndStartFindHost();
			vector<sGameHostInfo> hl;
			getNetClient()->getGameHostList(hl);
			do{
				getNetClient()->quant_th1();
				getNetClient()->getGameHostList(hl);
			}while(hl.size()==0);
			getNetClient()->JoinGame(hl.begin()->gameHostGUID, playerName.c_str(), Race(), 1);
		}
		else {
			getNetClient()->JoinGame(ipstr.c_str(), playerName.c_str(), Race(), 1, "");
		}
		while(getNetClient()->getCurrentMissionDescription().playersAmount() < 1){ //Hint-овая проверка на то, что подключились
			getNetClient()->quant_th1();
			::Sleep(40);
		}
		getNetClient()->StartLoadTheGame();
	}
}
void GameShell::startDWMPWithoutInterface(const char* missionName)
{
	const char* playerName=check_command_line("playerName");
	xassert(playerName);
	const char* strPassword=check_command_line("password");
	if(strPassword==0) strPassword="";
	createNetClient(PNCWM_ONLINE_DW);
	getNetClient()->Configurate(playerName,  strPassword);
	if(check_command_line(COMMAND_LINE_DW_HOST)){
		int numPlayers=atoi(check_command_line(COMMAND_LINE_DW_HOST));
		getNetClient()->CreateGame("HostGame", MissionDescription(missionName, GAME_TYPE_MULTIPLAYER), playerName, Race(), 0, 1, strPassword);
		while(getNetClient()->getCurrentMissionDescription().playersAmount() < numPlayers){
			getNetClient()->quant_th1();
			::Sleep(40);
		}
		getNetClient()->StartLoadTheGame();

	}
	else if(check_command_line(COMMAND_LINE_DW_CLIENT)){
		string ipstr=check_command_line(COMMAND_LINE_DW_CLIENT);
		getNetClient()->ResetAndStartFindHost();
		//vector<sGameHostInfo*>& hl=getNetClient()->getGameHostList();
		//do{
		//	getNetClient()->quant_th1();
		//	hl=getNetClient()->getGameHostList();
		//}while(hl.size()==0);
		//getNetClient()->JoinGame((*hl.begin())->gameHostGUID, playerName, Race(), 1);

		if(ipstr==""){
			getNetClient()->ResetAndStartFindHost();
			vector<sGameHostInfo> hl;
			getNetClient()->getGameHostList(hl);
			do{
				getNetClient()->quant_th1();
				getNetClient()->getGameHostList(hl);
			}while(hl.size()==0);
			getNetClient()->JoinGame(hl.begin()->gameHostGUID, playerName, Race(), 1);
		}
		else {
			getNetClient()->JoinGame(ipstr.c_str(), playerName, Race(), 1, "");
		}
		while(getNetClient()->getCurrentMissionDescription().playersAmount() < 1){ //Hint-овая проверка на то, что подключились
			getNetClient()->quant_th1();
			::Sleep(40);
		}
		getNetClient()->StartLoadTheGame();

	}
	else
		xassert(0);
}


GameShell::~GameShell()
{
	delete &globalTrigger_;

	//if(soundPushedByPause)
	//	SNDPausePop();

	HTManager::instance()->GameClose();

	deleteNetClient();

	debugFont_->Release();
}

void GameShell::done()
{
	UI_Dispatcher::instance().releaseResources();
}

bool GameShell::isNetClientConfigured(e_PNCWorkMode workMode)
{
	if(NetClient && NetClient->getWorkMode() == workMode)
		return true;
	return false;
}

void GameShell::createNetClient(e_PNCWorkMode _workMode)
{
	deleteNetClient();
	NetClient = new PNetCenter(_workMode);
}

void GameShell::stopNetClient()
{
	if(!stopNetClientSuspended){
		if(universeX())
			universeX()->stopNetCenter();
		stopNetClientSuspended = true;
	}
}

void GameShell::deleteNetClient() 
{
	delete NetClient;
	NetClient = 0;
	stopNetClientSuspended = false;
}

void GameShell::GameStart(const MissionDescription& mission)
{
	{
	start_timer_auto();
	start_timer(0);
	setLogicFp();
	//qslog.init();

	UI_Dispatcher::instance().setLoadingScreen();
	UI_LogicDispatcher::instance().setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_WAITING));
	UI_Dispatcher::instance().setEnabled(true);
	UI_LogicDispatcher::instance().startLoadProgressSection(UI_LOADING_INITIAL);
	
	stop_timer(0);
	start_timer(1);

	terScene->Compact();

//	gb_RenderDevice->RestoreDeviceForce();//nvidia bug fix

	setControlEnabled(true);

	isCutScene_ = false;
	pauseType_ = 0;

	terminateMission_ = false;
	defaultSaveName_ = "";

	interpolation_timer_ = 0;

	cVisGeneric::SetAssertEnabled(false);
	CurrentMission = mission;

	currentMissionPopulation_ = CurrentMission.gamePopulation();
	for (int i = 0; i < CurrentMission.playersAmountMax(); i++) {
		SlotData& data = CurrentMission.changePlayerData(i);
		if(data.realPlayerType==REAL_PLAYER_TYPE_PLAYER){
			for(int j=0; j<NETWORK_TEAM_MAX; j++){
				if(data.usersIdxArr[j]!=USER_IDX_NONE){
					if(*(CurrentMission.playerName(i, j))==0)
						CurrentMission.setPlayerName(i, j, UI_LogicDispatcher::instance().currentPlayerDisplayName());
				}
			}
		}
	}

	start_timer(vmap);
	vMap.load(CurrentMission.worldName());
	stop_timer(vmap);

	loadProgressUpdate();
	
	stop_timer(1);
	start_timer(2);

	xassert( checkLogicFp() );
	terToolsDispatcher.prepare4World();
	
	float gameSpeed;
	if(!IniManager("Game.ini").getFloat("Game", "GameSpeed", gameSpeed))
		gameSpeed = 1.0f;
	setSpeed(gameSpeed);

	cameraManager->reset();
	//CurrentMission.packPlayerIDs();

	stop_timer(2);
	start_timer(3);

	UI_LogicDispatcher::instance().startLoadProgressSection(UI_LOADING_UNIVERSE);

	dassert(!selectManager);
	selectManager = new SelectManager;

	MissionDescription missionTemp = CurrentMission;
	XPrmIArchive ia;
	new UniverseX(missionTemp.isMultiPlayer() ? NetClient : 0, missionTemp, ia.open(missionTemp.saveName()) ? &ia : 0);
	universe()->setUseHT(HTManager::instance()->IsUseHT());
	if(universe()->userSave())
		serializeUserSave(ia);
	else{
		if(GlobalAttributes::instance().directControlMode){
			RealUnits::const_iterator ui;
			FOR_EACH(universe()->activePlayer()->realUnits(), ui){
				UnitReal* unit(*ui);
				if(unit->attr().defaultDirectControlEnabled)
					selectManager->selectUnit(unit, false);
			}
		}
	}

	stop_timer(3);
	start_timer(4);

	musicManager.OpenWorld();

//	CameraUpdateFocusPoint();
//	CameraQuant();
//	cameraManager->SetPosition(CameraPosition);
//	cameraManager->SetTarget(CameraAngle);

//overloading	terScene->Compact();

	//universe()->relaxLoading();

	universe()->setControlEnabled(!(CurrentMission.gameType() & GAME_TYPE_REEL));

	loadProgressUpdate();
	UI_Dispatcher::instance().setEnabled(mission.enableInterface);

	if(CurrentMission.gameSpeed)
		setSpeed(CurrentMission.gameSpeed);

	if(!CurrentMission.isMultiPlayer() && CurrentMission.isGamePaused())
		pauseGame(PAUSE_BY_USER);

	setCountDownTime(-1);

	setSilhouetteColors();
	
	UI_LogicDispatcher::instance().SetSelectPeram(universe()->activePlayer()->race()->selection_param);
	UI_LogicDispatcher::instance().createSignSprites();

	UI_LogicDispatcher::instance().showLoadProgress(1.f);

	if(CurrentMission.silhouettesEnabled)
		gb_VisGeneric->EnableSilhouettes(GameOptions::instance().getBool(OPTION_SILHOUETTE));		
	else
		gb_VisGeneric->EnableSilhouettes(false);		

	GameOptions::instance().gameSetup();

	relaxLoading();

	UI_LogicDispatcher::instance().hideCursor();

	GameActive = true;

	SNDSetGameActive(true);
	sndSystem.SetStandbyTime(1);
	SNDSetFade(true,3000);

	quantTimeStatistic.reset(HTManager::instance()->IsUseHT());

	stop_timer(4);
	start_timer(5);

	GetTexLibrary()->MarkAllLoaded();
	pLibrary3dx->MarkAllLoaded();
	pLibrarySimply3dx->MarkAllLoaded();

	ratingDelta_ = 0;
	if(NetClient && CurrentMission.isMultiPlayer() && isNetClientConfigured(PNCWM_ONLINE_DW) && currentMissionPopulation_ >= 0){
		sendStatsTimer_.start(2*60000);
		/*
		static Scores scores;
		scores = Scores();
		scores.setTotalConnections(1);
		PlayerVect::iterator pi;
		FOR_EACH(universe()->Players, pi){
			if((*pi)->realPlayerType() == REAL_PLAYER_TYPE_PLAYER && (*pi)->auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER){
				xassert((*pi)->playerID() < NETWORK_PLAYERS_MAX);
				Scores* scores = &scoresArray_[(*pi)->playerID()];
				*scores = Scores();
			}
		}
		*/
	}

	frame_time.skip();
	scale_time.skip();

	needLogicCall_ = true;
	sleepGraphicsTime_ = 0;
	sleepGraphics_ = false;

	stop_timer(5);
	}

#ifndef _FINAL_VERSION_
//	extern TimerData* loadTimer;
//	loadTimer->stop();
//	profiler_start_stop();
#endif
}

void GameShell::GameClose()
{
	start_timer_auto();

	if(NetClient && CurrentMission.isMultiPlayer()){
		sendStats(true, universe()->activePlayer()->isWin());
		NetClient->FinishGame();
	}
	
	GameActive = false;

	//SNDSetFade(false,1000);
	SNDStopAll();
	SNDSetFade(false,0); // после того как появится поток для загрузки вернуть значение времени
	SNDSetGameActive(false);
	UI_LogicDispatcher::instance().setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_WAITING));
	UI_LogicDispatcher::instance().profileSystem().saveState();
	
	//UI_LogicDispatcher::instance().releaseCursorEffect();

	UI_Dispatcher::instance().reset();
	
	visibleUnits_.clear();
	streamInterpolator_.clear();
	streamCommand_.process(0);
	streamCommand_.clear();

	if(universeX())
		delete universeX();

	UI_Dispatcher::instance().resetScreens();

	delete selectManager;
	selectManager = 0;

	scale_time.setSpeed(1);
	frame_time.skip();
	scale_time.skip();

//overloading	terScene->Compact();

	//if (soundPushedByPause) {
	//	soundPushedByPause = false;
	//	SNDPausePop();
	//	xassert(soundPushedPushLevel==SNDGetPushLevel());
	//	soundPushedPushLevel=INT_MIN;
	//}

	//SNDStopAll();

	//musicManager.CloseWorld();
	cVisGeneric::SetAssertEnabled(true);
}

void GameShell::sendStats(bool final, bool win)
{
	if(isNetClientConfigured(PNCWM_ONLINE_DW) && currentMissionPopulation_ >= 0){
		Player* activePlayer = universe()->activePlayer();
		/*
		const Scores& myScores = scoresArray_[activePlayer->playerID()];
		double Rme = myScores.getTotalRating();
		double W = 1;
		double K = 20;
		PlayerVect::iterator pi;
		FOR_EACH(universe()->Players, pi){
			if((*pi)->realPlayerType() == REAL_PLAYER_TYPE_PLAYER && (*pi)->auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER && (*pi)->clan() != activePlayer->clan()){
				double Rx = scoresArray_[(*pi)->playerID()].getTotalRating();
				W += pow(10., clamp((Rx - Rme)/K, -100., 100.));
			}
		}
		W = 1./W;
		double factor = 2./sqrt(Rme + 1.);

		static Scores scoresPre, scoresFinal;
		Scores& scores = final ? scoresFinal : scoresPre;
		scores = Scores();
		if(final){
			universe()->activePlayer()->playerStatistics().get(scores);
			scores.setTotalRating((win ? 1. - W : -W)*factor);
			scores.setTotalRatingDelta(-ratingDelta_);
		}
		else{
			double deltaPrev = myScores.getTotalRatingDelta();
			scores.setTotalRating(deltaPrev);
			scores.setTotalRatingDelta(-deltaPrev + (ratingDelta_ = -W*factor));
		}
		if(myScores.getTotalRating() + scores.getTotalRating() < 0)
			scores.setTotalRating(-myScores.getTotalRating());
		scores.setRating(round(myScores.getTotalRating() + scores.getTotalRating()) - myScores.getRating());
		*/
	}
}

bool GameShell::universalSave(const char* name, bool userSave)
{
	MTAuto lock(*lock_logic);
	MTAutoSkipAssert skip_assert;

	MissionDescription mission(CurrentMission);
	mission.setSaveName(name);

	if(userSave){
		mission.globalTime = global_time();
		mission.gameSpeed = game_speed;
		mission.setGamePaused(isPaused(PAUSE_BY_USER));
	}

	XPrmOArchive oa(mission.saveName());
	if(universe()->universalSave(mission, userSave, oa)){
		if(userSave)
			serializeUserSave(oa);
		if(oa.close())
			return true;
	}
	mission.deleteSave();
	return false;
}

void GameShell::serializeUserSave(Archive& ar)
{
	if(ar.openStruct("userInterface", "userInterface", "UserInterface")){
		UI_Dispatcher::instance().serializeUserSave(ar);
		ar.serialize(*selectManager, "selectManager", 0);
	}
	ar.closeStruct("userInterface");
}

void GameShell::NetQuant()
{
	if(NetClient){
		NetClient->quant_th1();
		if(stopNetClientSuspended)
			deleteNetClient();
	}
}

bool GameShell::logicQuant()
{
	start_timer_auto();

	setLogicFp();

	MTAuto lock(*lock_logic);

	int begQuantTime = xclock();
	if(universeX()->PrimaryQuant()){
		selectionQuant();
		directControlQuant();
		UI_Dispatcher::instance().logicQuant();

		SoundQuant();
		sndSystem.Update();

		if(sendStatsTimer_()){
			sendStats(false, false);
			sendStatsTimer_.stop();
		}

		quantTimeStatistic.putLogic(xclock() - begQuantTime);
		interfaceLogicCallTimer_.start(110);

		logicFpsMeter_.quant_begin(begQuantTime);
		logicFpsMeter_.quant_end(xclock());
		logicFps_ = logicFpsMeter_.GetFPS();
		logicFpsMeter_.GetFPSminmax(logicFpsMin_, logicFpsMax_);
		float f1,f2;
		logicFpsMeter_.GetInterval(logicFpsTime_,f1,f2);

		return true;
	}
	
	return false;
}

void GameShell::logicQuantPause()
{
	if((needLogicCall_ || !universe()->isMultiPlayer()) && !interfaceLogicCallTimer_){
		start_timer_auto();
		interfaceLogicCallTimer_.start(100);
		UI_Dispatcher::instance().logicQuant(true);
		universe()->triggerQuant(true);
		sndSystem.Update();
	}
}

void GameShell::logicQuantST()
{
	start_timer_auto();

	if(!GameActive)
		return;

	if(needLogicCall_ && logicQuant()){
		universe()->interpolationQuant();
		needLogicCall_ = false;
	}
	logicQuantPause();
}

void GameShell::logicQuantHT()
{
	start_timer_auto();

	if(!GameActive)
		return;

	if(logicQuant()){
		while(!needLogicCall_ && !HTManager::instance()->terminateLogicThread()){
			Sleep(1);
			logicQuantPause();
		}
		universe()->interpolationQuant();
		needLogicCall_ = false;
	}
	logicQuantPause();
}

void GameShell::selectionQuant()
{
	selectManager->quant();
}

void GameShell::relaxLoading()
{
	universe()->relaxLoading();

	UI_Dispatcher::instance().relaxLoading();

	UI_Dispatcher::instance().preloadScreen(!CurrentMission.screenToPreload().empty() ? UI_ScreenReference(CurrentMission.screenToPreload()).screen() :
		universe()->activePlayer()->race()->screenToPreload.screen(), terScene, universe()->activePlayer());

	for(int j = 0; j < 4; j++){
		universe()->Quant();
		universe()->interpolationQuant();
		universe()->streamCommand.process(0);
		universe()->streamCommand.clear();
		universe()->streamInterpolator.process(0);
		universe()->streamInterpolator.clear();
		gb_VisGeneric->SetGraphLogicQuant(universe()->quantCounter());
		//UI_Dispatcher::instance().logicQuant();
		//UI_Dispatcher::instance().quant(0.1f);
	}
}

void GameShell::processEvents()
{
	eventProcessing_ = true;
	WindowEventQueue::const_iterator it;
	FOR_EACH(windowEventQueue_, it)
		EventParser(it->uMsg, it->wParam, it->lParam);
	windowEventQueue_.clear();
	eventProcessing_ = false;
}

void GameShell::GraphQuant()
{
	start_timer_auto();

	if(GameActive && HTManager::instance()->IsUseHT()){
		if(quantTimeStatistic.logicTime() > max(logicTimePeriod, quantTimeStatistic.graphicsTime() + round(sleepGraphicsTime_)))
			sleepGraphicsTime_ += 0.1f;
		else
			sleepGraphicsTime_ = max(sleepGraphicsTime_ - 0.1f, 0.f);

		statistics_add(LG, universe()->quantCounter() - gb_VisGeneric->GetGraphLogicQuant());

		if(sleepGraphicsTime_ > 1 && sleepGraphics_ && universe()->quantCounter() - gb_VisGeneric->GetGraphLogicQuant() < 1){
			sleepGraphics_ = false;
			::Sleep(round(sleepGraphicsTime_));
			statistics_add(sleepGraphicsTimem, sleepGraphicsTime_);
			return;
		}
		else
			sleepGraphics_ = true;
	}

	if(!isPaused(PAUSE_BY_USER | PAUSE_BY_MENU))
		scale_time.next_frame();
	else
		scale_time.skip();

	if(cameraManager)
		cameraManager->SetFrustum();

	frame_time.next_frame();

	unsigned int begQuantTime = xclock();
	bool firstGraphFrame = false;
	if(universe()){
		interpolation_timer_ += scale_time.delta();
		if(interpolation_timer_ > logicTimePeriod){
			if(!needLogicCall_){
				interpolation_timer_ -= logicTimePeriod;
				interpolation_timer_ = clamp(interpolation_timer_, 0, 3*logicTimePeriod);

				streamLock.lock();
				streamCommand_.put(universe()->streamCommand);
				universe()->streamCommand.clear();

				streamInterpolator_.clear();
				streamInterpolator_.put(universe()->streamInterpolator);
				universe()->streamInterpolator.clear();

				visibleUnits_.swap(universe()->visibleUnits);
				streamLock.unlock();

				firstGraphFrame = true;
				needLogicCall_ = true;
			}
			else if(!universeX()->isMultiPlayer() || universeX()->getInternalLagQuant() > 0){
				start_timer(wait);
				interpolation_timer_ -= scale_time.delta();
				scale_time.revert();
				frame_time.revert();
				if(HTManager::instance()->IsUseHT())
		            ::Sleep(1);
				stop_timer(wait);
				return;
			}
		}

		universe()->circleManager()->clear();
		universe()->circleManagerTeam()->clear();

		//xassert(interpolation_timer_ <= logicTimePeriod);
		float interpolationFactor = clamp(interpolation_timer_, 0, logicTimePeriod)*logicTimePeriodInv;

		streamLock.lock();
		gb_VisGeneric->SetGraphLogicQuant(universe()->quantCounter());
		gb_VisGeneric->SetInterpolationFactor(interpolationFactor);
		streamLock.unlock();

		streamCommand_.process(interpolationFactor);
		streamCommand_.clear();

		streamInterpolator_.process(interpolationFactor);

		float scaleDeltaTime = scale_time.delta()/1000.f;
		float frameDeltaTime = frame_time.delta()/1000.f;

		processEvents();
		UI_Dispatcher::instance().quant(frameDeltaTime);
	
		cameraQuant((frameDeltaTime + scaleDeltaTime)/2.0f);
		Show(scaleDeltaTime);

		if(terminateMission_)
			UI_Dispatcher::instance().exitMissionPrepare();
		if(terminateMission_ && UI_Dispatcher::instance().canExit())
			HTManager::instance()->GameClose();	
	}
	else{ // MainMenu, только графический поток
		tls_is_graph = MT_GRAPH_THREAD | MT_LOGIC_THREAD;

		interpolation_timer_ += scale_time.delta();
		if(interpolation_timer_ > logicTimePeriod){
			needLogicCall_ = true;
			global_time.next_frame();
			interpolation_timer_ -= logicTimePeriod;
		}

		SoundQuant();
		sndSystem.Update();

		processEvents();
		UI_Dispatcher::instance().quant(frame_time.delta()/1000.0f);
		UI_Dispatcher::instance().logicQuant();
		globalTrigger_.quant(false);

		Show(scale_time.delta()/1000.0f);
	}

	if(startMissionSuspended_)
		UI_Dispatcher::instance().exitMissionPrepare();
	if(startMissionSuspended_ && UI_Dispatcher::instance().canExit()){
		startMissionSuspended_ = false;
		HTManager::instance()->GameStart(missionToStart_);
		gameReadyCounter_ = 5;
	}

	if(NetClient && universe() && universe()->isMultiPlayer() && !--gameReadyCounter_){
        NetClient->GameIsReady();
		quantTimeStatistic.reset(HTManager::instance()->IsUseHT());
	}

	mousePositionDelta_ = Vect2f::ZERO;
	cameraCursor_ = 0;

	if(firstGraphFrame)
		quantTimeStatistic.putGraph(xclock() - begQuantTime);
}

void fSendDirectKeysCommand(void* data)
{
	int* command_data = (int*)data;
	if(!selectManager->isSelectionEmpty())
//		selectManager->makeCommand(UnitCommand(COMMAND_ID_DIRECT_KEYS,  0, UI_LogicDispatcher::instance().aimPosition(), *command_data));
		selectManager->makeCommand(UnitCommand(COMMAND_ID_DIRECT_KEYS,  UI_LogicDispatcher::instance().hoverUnit(), UI_LogicDispatcher::instance().aimPosition(), *command_data));
	else
		UI_LogicDispatcher::instance().disableDirectControl();

}

void GameShell::directControlQuant()
{
	UI_LogicDispatcher::instance().directShootQuant();

	if(!underFullDirectControl())
		return;

	int direction = 0;

	if(ControlManager::instance().key(CTRL_DIRECT_STRAFE_LEFT).pressed())
		direction |= DIRECT_KEY_STRAFE_LEFT;
	if(ControlManager::instance().key(CTRL_DIRECT_STRAFE_RIGHT).pressed())
		direction |= DIRECT_KEY_STRAFE_RIGHT;
	if(ControlManager::instance().key(CTRL_DIRECT_LEFT).pressed())
		direction |= DIRECT_KEY_TURN_LEFT;
	if(ControlManager::instance().key(CTRL_DIRECT_RIGHT).pressed())
		direction |= DIRECT_KEY_TURN_RIGHT;
	if(ControlManager::instance().key(CTRL_DIRECT_UP).pressed())
		direction |= DIRECT_KEY_MOVE_FORWARD;
	if(ControlManager::instance().key(CTRL_DIRECT_DOWN).pressed())
		direction |= DIRECT_KEY_MOVE_BACKWARD;

	if(!UI_LogicDispatcher::instance().hoverControl()){
		if(UI_LogicDispatcher::instance().isMouseFlagSet(MK_LBUTTON))
			direction |= DIRECT_KEY_MOUSE_LBUTTON;
		if(UI_LogicDispatcher::instance().isMouseFlagSet(MK_RBUTTON))
			direction |= DIRECT_KEY_MOUSE_RBUTTON;
		if(UI_LogicDispatcher::instance().isMouseFlagSet(MK_MBUTTON))
			direction |= DIRECT_KEY_MOUSE_MBUTTON;
	}

	uiStreamCommand.set(fSendDirectKeysCommand) << direction;
}

void fCommandSetDirectControl(XBuffer& stream)
{
	DirectControlMode mode;
	stream.read(mode);
	UnitInterface* unit;
	stream.read(unit);
	int transitionTime;
	stream.read(transitionTime);
	gameShell->setDirectControl(mode, unit, transitionTime);
}

void GameShell::setDirectControl(DirectControlMode mode, UnitInterface* unit, int transitionTime)
{
	if(MT_IS_GRAPH()){
		if(mode && GameOptions::instance().getBool(OPTION_CAMERA_UNIT_FOLLOW))
			switch(mode){
			case DIRECT_CONTROL_ENABLED:
				cameraManager->erasePath();
				cameraManager->enableDirectControl(unit, transitionTime);
				break;
			case SYNDICATE_CONTROL_ENABLED:
				cameraManager->disableDirectControl();
				cameraManager->erasePath();
				cameraManager->SetCameraFollow(unit, transitionTime);
				break;
			}
		else
			if(cameraManager->unitFollow() == unit || cameraManager->unitFollowPrev() == unit)
				cameraManager->disableDirectControl();

		directControl_ = mode;
	}else
		streamLogicCommand.set(fCommandSetDirectControl) << mode << unit << transitionTime;
}

void GameShell::Show(float realGraphDT)
{
	start_timer_auto();
	if(HTManager::instance()->IsUseHT())
		profiler_quant();

	start_timer(1);
	Console::instance().graphQuant();
	stop_timer(1);

	if(GameActive){
		start_timer(2);

		terScene->SetDeltaTime(realGraphDT * 1000);

		gbCircleShow->Quant(realGraphDT);
		
		sColor4c& Color = environment->environmentTime()->GetCurFoneColor();
		gb_RenderDevice->Fill(Color.r,Color.g,Color.b);
		gb_RenderDevice->BeginScene();

		gb_RenderDevice->SetRenderState(RS_FILLMODE, debugWireFrame ? FILL_WIREFRAME : FILL_SOLID);
		
		UI_LogicDispatcher::instance().selectCursor(cameraCursor_);
		UI_LogicDispatcher::instance().moveCursorEffect();

		stop_timer(2);
		start_timer(3);

		environment->graphQuant(realGraphDT);

		cameraManager->GetCamera()->SetAttr(ATTRCAMERA_CLEARZBUFFER);//Потому как в небе могут рисоваться планеты в z buffer.
		terScene->Draw(cameraManager->GetCamera());

		environment->drawPostEffects(realGraphDT);
		
		gb_RenderDevice->SetRenderState(RS_FILLMODE, FILL_SOLID);

		stop_timer(3);
		start_timer(4);

		if(!isCutScene()){
			start_timer(1);
			VisibleUnits::iterator ui;
			FOR_EACH(visibleUnits_, ui)
				if(!(*ui)->dead())
					(*ui)->graphQuant(realGraphDT);
			stop_timer(1);

			universe()->graphQuant(realGraphDT);

			environment->drawUI(realGraphDT);
		}

		UI_Dispatcher::instance().drawDebugInfo();

		gb_RenderDevice->SetDrawTransform(cameraManager->GetCamera());
		gb_RenderDevice->FlushPrimitive3D();

		gb_RenderDevice->SetClipRect(0,0,gb_RenderDevice->GetSizeX(),gb_RenderDevice->GetSizeY());

		UI_Dispatcher::instance().redraw();

		environment->drawBlackBars();

		universe()->drawDebug2D();
	
		stop_timer(4);
		start_timer(5);

		gb_VisGeneric->DrawInfo();

#ifndef _FINAL_VERSION_
		if(universe()->activePlayer()->isAI())
			UI_Render::instance().outText(Rectf(0.9f,0,0,0), "AI");
#endif

		if(recordMovie_)
			makeMovieShot();

		gb_RenderDevice->EndScene();

		gb_RenderDevice->Flush();

		if(debugShowWatch)
			show_watch();

		stop_timer(5);
	}
	else{
		//draw
		gb_RenderDevice->Fill(0,0,0);
		gb_RenderDevice->BeginScene();

		UI_Dispatcher::instance().redraw();

		gb_RenderDevice->EndScene();
		gb_RenderDevice->Flush();
	}
}

//--------------------------------------------------------
inline int IsMapArea(const Vect2f& pos)
{
	return 1;
}

Vect2f GameShell::convert(int x, int y) const 
{
	return Vect2f(windowClientSize().x ? float(x)/float(windowClientSize().x) - 0.5f : 0, 
		windowClientSize().y ? float(y)/float(windowClientSize().y) - 0.5f : 0);
}

Vect2i GameShell::convertToScreenAbsolute(const Vect2f& pos)
{
	POINT pt = { round((pos.x + 0.5f)*windowClientSize().x), round((pos.y + 0.5f)*windowClientSize().y) };
	ClientToScreen(gb_RenderDevice->GetWindowHandle(), &pt);
	return Vect2i(pt.x, pt.y);
}

bool GameShell::checkReel(UINT uMsg,WPARAM wParam,LPARAM lParam) {
	if (reelManager.isVisible()) {
		if (reelAbortEnabled) {
			switch (uMsg) {
				case WM_KEYDOWN:
					//if (sKey(wParam, true).fullkey != VK_SPACE) {
					//	return true;
					//}
				case WM_LBUTTONDOWN:
				case WM_MBUTTONDOWN:
					reelManager.hide();
					break;
				default:
					return true;
			}
		}
	}
	return false;
}

void GameShell::EventHandler(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if(eventProcessing_)
		return;

	if (checkReel(uMsg, wParam, lParam)) {
		return;
	}

    switch(uMsg){
	case WM_MOUSELEAVE:
	case WM_MOUSEMOVE: 
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP: 
	case WM_CHAR:
	case WM_MOUSEWHEEL:
    case WM_ACTIVATEAPP:
		break;
	default:
		return;
    }

	windowEventQueue_.push_back(WindowEvent(uMsg, wParam, lParam));
}

void GameShell::EventParser(UINT uMsg,WPARAM wParam,LPARAM lParam)
{

    switch(uMsg){
	case WM_MOUSELEAVE:
		MouseLeave();
		break;
	case WM_MOUSEMOVE: 
		MouseMove(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		break;
	case WM_LBUTTONDOWN:
		MouseLeftPressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		break;
	case WM_LBUTTONUP:
		MouseLeftUnpressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		break;
	case WM_MBUTTONDOWN:
		MouseMidPressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		break;
	case WM_MBUTTONUP:
		MouseMidUnpressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		break;
	case WM_LBUTTONDBLCLK:
		MouseLeftPressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		MouseLeftUnpressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		MouseLeftDoubleClick(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		break;
	case WM_RBUTTONDOWN:
		MouseRightPressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		break;
	case WM_RBUTTONUP:
		MouseRightUnpressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		break;
	case WM_RBUTTONDBLCLK:
		MouseRightPressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		MouseRightUnpressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		MouseRightDoubleClick(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		break;
	case WM_MBUTTONDBLCLK:
		MouseMidPressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		MouseMidUnpressed(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		MouseMidDoubleClick(convert(LOWORD(lParam), HIWORD(lParam)), wParam);
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:	{
		sKey key(wParam, true);
		if(isKeyEnabled(key))
			KeyPressed(key, lParam & 0x40000000);
		break;			}
	case WM_KEYUP:
	case WM_SYSKEYUP:	{
		sKey key(wParam);
		if(isKeyEnabled(key))
			KeyUnpressed(key);
		break;			}
	case WM_CHAR:
		charInput(wParam);
		break;
	case WM_MOUSEWHEEL:
		MouseWheel(wParam);
		break;
    case WM_ACTIVATEAPP:
		if(wParam)
			OnWindowActivate();
		break;
    }
}

void GameShell::charInput(int chr)
{
	UI_InputEvent event(UI_INPUT_CHAR, mousePosition_);
	event.setCharInput(chr);

	UI_Dispatcher::instance().handleInput(event);
}
void GameShell::ShowStatistic()
{
	MTAuto lock(*lock_logic);
	void ShowGraphicsStatistic();
	ShowGraphicsStatistic();
}

bool GameShell::DebugKeyPressed(sKey& Key)
{
	MTAutoSkipAssert mtAutoSkipAssert;

	switch(Key.fullkey){
	case VK_SCROLL:
		ConsoleWindow::instance().show(!ConsoleWindow::instance().isVisible());
		break;
	case VK_RETURN | KBD_MENU: 
		GameOptions::instance().setOption(OPTION_FULL_SCREEN, !GameOptions::instance().getOption(OPTION_FULL_SCREEN));
		GameOptions::instance().userApply(true);
		UI_LogicDispatcher::instance().handleMessageReInitGameOptions();
		break;
	case VK_F9:
		cameraManager->spline().push_back(cameraManager->coordinate());
		break;
	 case VK_F9 | KBD_SHIFT:
		if(!cameraManager->spline().empty())
			cameraManager->spline().pop_back();
		break;
	case VK_F9 | KBD_CTRL | KBD_SHIFT:
		cameraManager->erasePath();
		cameraManager->spline().setPose(Se3f::ID, true);
		break;
	case VK_F9 | KBD_CTRL: {
		if(!cameraManager->isPlayingBack()){
			if(const char* name = cameraManager->popupCameraSplineName()){
				if(CameraSpline* spline = cameraManager->findSpline(name)){
					cameraManager->loadPath(*spline, false);
					cameraManager->startReplayPath(4000, 10);
				}
			}
		}
		else
			cameraManager->stopReplayPath();
		break;
		}
	case VK_F9 | KBD_CTRL | KBD_MENU: {
		XBuffer name;
		name < "Camera" <= cameraManager->splines().size();
		CameraSpline* spline = new CameraSpline(cameraManager->spline());
		spline->setName(editText(name));
		cameraManager->addSpline(spline);
		} break; 
		
	case KBD_CTRL | VK_F11:
		startStopRecordMovie();
		break;

	case VK_F2:
		debugShowWatch = !debugShowWatch;
		break;
	case VK_F3:
		debugShowEnabled = !debugShowEnabled;
		break;

	case VK_F2 | KBD_SHIFT:
		UI_Dispatcher::instance().updateDebugControl();
		break;

	case VK_F4 | KBD_SHIFT:
	case VK_F4:
		if(isAltPressed())
			break;
		editParameters();
		break;

	case 'R' | KBD_CTRL:
		if(cameraManager->isAutoRotationMode())
			cameraManager->disableAutoRotationMode();
		else {
			CameraCoordinate coord = cameraManager->coordinate();
			coord.position() = UI_LogicDispatcher::instance().hoverPosition();
			cameraManager->setCoordinate(coord);
			cameraManager->enableAutoRotationMode(0.01f);
		}
		break;

	case VK_LEFT | KBD_CTRL | KBD_SHIFT:{
			Vect3f axisZ, axisX, axisY;
			cameraManager->getCameraAxis(axisZ, axisX, axisY);
			cameraManager->startOscillation(2000, 1.0f, axisX);
		}
		break;

	case VK_UP | KBD_CTRL | KBD_SHIFT:{
			Vect3f axisZ, axisX, axisY;
			cameraManager->getCameraAxis(axisZ, axisX, axisY);
			cameraManager->startOscillation(2000, 1.0f, axisY);
		}
		break;

	case VK_RIGHT | KBD_CTRL | KBD_SHIFT:{
			Vect3f axisZ, axisX, axisY;
			cameraManager->getCameraAxis(axisZ, axisX, axisY);
			cameraManager->startOscillation(2000, 1.0f, -axisX);
		}
		break;

	case VK_DOWN | KBD_CTRL | KBD_SHIFT:{
			Vect3f axisZ, axisX, axisY;
			cameraManager->getCameraAxis(axisZ, axisX, axisY);
			cameraManager->startOscillation(2000, 1.0f, -axisY);
		}
		break;

	case VK_LEFT | KBD_CTRL:
		if(cameraManager->isAutoRotationMode())
			cameraManager->enableAutoRotationMode(cameraManager->getRotationAngleDelta() + 0.01f);
		break;

	case VK_RIGHT | KBD_CTRL:
		if(cameraManager->isAutoRotationMode())
			cameraManager->enableAutoRotationMode(cameraManager->getRotationAngleDelta() - 0.01f);
		break;

	case VK_RETURN | KBD_SHIFT: 
		if(const AttributeBase* sel = selectManager->selectedAttribute()){
			AttributeBase& attr = const_cast<AttributeBase&>(*sel);
			EditArchive ea(0, TreeControlSetup(0, 0, 500, 500, "Scripts\\TreeControlSetups\\unitAttribute"));
			ea.setTranslatedOnly(false);
			if(ea.edit(attr)){
				AttributeLibrary::instance().saveLibrary();
				universe()->refreshAttribute();
			}
		}
		break;
		
	case VK_RETURN | KBD_SHIFT | KBD_MENU: 
		if(selectManager->selectionSize() == 1 && selectManager->uniform()){
			UnitBase* unit = selectManager->selectedUnit();
			if(unit->attr().isSquad())
				unit = safe_cast<UnitSquad*>(unit)->getUnitReal();
			EditArchive ea(0, TreeControlSetup(0, 0, 500, 500, "Scripts\\TreeControlSetups\\unit"));
			ea.setTranslatedOnly(false);
			ea.edit(*unit);
		}
		break;


/*
#ifndef _FINAL_VERSION_
	case VK_RETURN | KBD_CTRL: 
	case VK_RETURN | KBD_CTRL | KBD_SHIFT: {
		gb_RenderDevice->Flush();
		ShowCursor(1);
		//setUseAlternativeNames(true);
		static TriggerEditor triggetEditor(triggerInterface());
		TriggerChain* triggerChain = &globalTrigger_;
		if(universe()){
			Player* player = universe()->activePlayer();
			if(isShiftPressed()){
				XBuffer nameAlt;
				nameAlt < "Игрок (0-" <= universe()->Players.size() - 1 < ")";
				int playerID = player->playerID();
				EditArchive ea(0, TreeControlSetup(0, 0, 500, 300, "Scripts\\TreeControlSetups\\chooseTrigger"));
				static_cast<EditOArchive&>(ea).serialize(playerID,"playerID", nameAlt);
				if(ea.edit()){
					static_cast<EditIArchive&>(ea).serialize(playerID, "playerID", nameAlt);
					player = universe()->Players[clamp(playerID, 0, universe()->Players.size() - 1)];
				}
			}
			triggerChain = player->getStrategyToEdit();
		}
		if(triggerChain && triggetEditor.run(*triggerChain, gb_RenderDevice->GetWindowHandle())){
			triggerChain->save();
			triggerChain->buildLinks();
			triggerChain->setAIPlayerAndTriggerChain(universe()->activePlayer());
		}

		cameraManager->setFocus(HardwareCameraFocus);
		ShowCursor(0);				
		restoreFocus();									
		break; }
#endif
*/

	case VK_F6: 
	case VK_F6 | KBD_CTRL: 
	case VK_F6 | KBD_SHIFT: 
		if(!profilerStarted_)
			profiler_start_stop(isShiftPressed() ? PROFILER_MEMORY : (isControlPressed() ? PROFILER_ACCURATE : PROFILER_REBUILD));
		else{
			gb_RenderDevice->Flush();
			{
				MTAuto lock(*lock_logic);
				profiler_start_stop();
			}
			restoreFocus();
		}
		profilerStarted_ = !profilerStarted_;
		break;

	case VK_F8:
		UI_Dispatcher::instance().setEnabled(!UI_Dispatcher::instance().isEnabled());
		break;

	case 'M' | KBD_CTRL | KBD_MENU | KBD_SHIFT:
		cameraManager->setRestriction(!cameraManager->restricted());
		break;

	case '1' | KBD_CTRL | KBD_SHIFT:
		environment->water()->SetRainConstant(+0.03f);
		break;
	case '2' | KBD_CTRL | KBD_SHIFT:
		environment->water()->SetRainConstant(-0.03f);
		break;

	case 'S' | KBD_CTRL | KBD_SHIFT: {
		string saveName = CurrentMission.saveName();
		if(saveFileDialog(saveName, "Resource\\Saves", MissionDescription::getExtention(CurrentMission.gameType()), "Mission Name")){
			size_t pos = saveName.rfind("RESOURCE\\");
			if(pos != string::npos)
				saveName.erase(0, pos);
			universalSave(saveName.c_str(), true);
			defaultSaveName_ = saveName;
		}
		break;
		}

	case 'O' | KBD_CTRL | KBD_SHIFT: {
		string saveName = CurrentMission.saveName();
		if(openFileDialog(saveName, "Resource\\Saves", MissionDescription::getExtention(CurrentMission.gameType()), "Mission Name")){
			size_t pos = saveName.rfind("RESOURCE\\");
			if(pos != string::npos)
				saveName.erase(0, pos);
			if(startMissionSuspended(MissionDescription(saveName.c_str())))
				UI_Dispatcher::instance().selectScreen(0);
		}
		break;
		}

	case 'O' | KBD_CTRL | KBD_MENU | KBD_SHIFT: 
		startMissionSuspended(CurrentMission);
		break;

		
	case VK_PAUSE|KBD_SHIFT:
		alwaysRun_ = !alwaysRun_;
		break;

	case 'X' | KBD_CTRL:
		if(universe()){
			universe()->setActivePlayer(++activePlayerID_ %= universe()->Players.size() - 1);
			checkEvent(*EventHandle(new Event(Event::CHANGE_ACTIVE_PLAYER)));
		}
		break;
	case 'X' | KBD_SHIFT:
		if(universe())
			checkEvent(*EventHandle(new EventChangePlayerAI(Event::CHANGE_PLAYER_AI, universe()->activePlayer()->playerID())));
		break;
	case 'F':
		terShowFPS ^= 1;
		break;

	case 'P':
		ShowStatistic();
		break;

	case 'U' | KBD_CTRL:
		{
			static AttributeReference attr;
			int playerID = universe()->activePlayer()->playerID();
			static int number = 1;
			static bool inTheSameSquad = false;
			EditArchive ea(0, TreeControlSetup(0, 0, 500, 300, "Scripts\\TreeControlSetups\\createUnit"));
			static_cast<EditOArchive&>(ea).serialize(attr,"Unit","Юнит");
			static_cast<EditOArchive&>(ea).serialize(playerID,"playerID","Игрок");
			static_cast<EditOArchive&>(ea).serialize(number,"number","Количество");
			static_cast<EditOArchive&>(ea).serialize(inTheSameSquad,"inTheSameSquad","В одном скваде");
			if(ea.edit()){
				static_cast<EditIArchive&>(ea).serialize(attr,"Unit","Юнит");
				static_cast<EditIArchive&>(ea).serialize(playerID,"playerID","Игрок");
				static_cast<EditIArchive&>(ea).serialize(number,"number","Количество");
				static_cast<EditIArchive&>(ea).serialize(inTheSameSquad,"inTheSameSquad","В одном скваде");
				playerID = clamp(playerID, 0, universe()->Players.size()-1);
				UnitSquad* squad = 0;
				if(attr){
					for(int i = 0; i < number; i++){
						Player* player = universe()->Players[playerID];
						UnitBase* unit = player->buildUnit(attr);
						Vect3f pos;
						cameraManager->cursorTrace(mousePosition(), pos);
						if(number > 1)
							pos += Mat3f(2*M_PI*i/number, Z_AXIS)*Vect3f(unit->radius()*2, 0, 0);
						unit->setPose(Se3f(QuatF::ID, pos),true);
						if(unit->attr().isLegionary()){
							UnitLegionary* legionary = safe_cast<UnitLegionary*>(unit);
							if(!squad || !inTheSameSquad){
								squad = safe_cast<UnitSquad*>(player->buildUnit(&*legionary->attr().squad));
								squad->setPose(unit->pose(), true);
							}
							squad->addUnit(legionary, false);
						}
					}
				}
			}
			break;
		}
	
	case 'T' | KBD_CTRL:
		if(environment && !universe()->isMultiPlayer()){
			float cur_time = environment->environmentTime()->GetTime(); 
			if (cur_time > 12 && cur_time < 24)
				environment->environmentTime()->SetTime(1);
			else
				environment->environmentTime()->SetTime(13);

		}
		break;

	default:
		if(!DebugPrm::instance().forceDebugKeys)
			return false;
	}

	// Конфликтующие клавиши
	switch(Key.fullkey){
	case 'D':
		if(selectManager)
			selectManager->explodeUnit();
		break;
	case 'A':
		incrSpeed();
		break;
	case 'Z':
		decrSpeed();
		break;
	case 'A'|KBD_SHIFT:
		setSpeed(1);
		break;
	case 'Z'|KBD_SHIFT:
		setSpeed(0.05f);
		break;

	default:
		return false;
	}

	return true;
}

void GameShell::KeyPressed(sKey& Key, bool isAutoRepeat)
{
	cheatManager.keyPressed(Key);

#ifndef _FINAL_VERSION_
	if(Key.fullkey == (VK_F1|KBD_SHIFT|KBD_CTRL)){
		DebugPrm::instance().enableKeyHandlers ^= 1;
		DebugPrm::instance().saveLibrary();
	}
	else if(Key.fullkey == (VK_F1|KBD_SHIFT|KBD_CTRL|KBD_MENU)){
		DebugPrm::instance().forceDebugKeys ^= 1;
		DebugPrm::instance().saveLibrary();
	}
#endif

	if(DebugPrm::instance().enableKeyHandlers && DebugPrm::instance().forceDebugKeys && universe() && DebugKeyPressed(Key))
		return;

	if(!getNetClient() && !isPaused(PAUSE_BY_ANY) && !isAutoRepeat)
		checkEvent(*EventHandle(new EventKeyBoardClick(Event::KEYBOARD_CLICK, Key.fullkey)));

	UI_InputEvent event(UI_INPUT_KEY_DOWN, mousePosition());
	event.setKeyCode(Key.fullkey);

	if(UI_Dispatcher::instance().handleInput(event) || isAutoRepeat)
		return;

	if(DebugPrm::instance().enableKeyHandlers && !DebugPrm::instance().forceDebugKeys && universe() && DebugKeyPressed(Key))
		return;

	ControlPressed(Key.fullkey);
	ControlManager::instance().handleKeypress(Key.fullkey);
}

void GameShell::ControlPressed(int key)
{
	if(!controlEnabled())
		return;

	InterfaceGameControlID ctrl = ControlManager::instance().control(key);

	switch(ctrl)
	{
		case CTRL_PAUSE:
			if(!CurrentMission.isMultiPlayer() && CurrentMission.enablePause) {
				if(!isPaused(PAUSE_BY_USER))
					pauseGame(PAUSE_BY_USER);
				else
					resumeGame(PAUSE_BY_USER);
			}
			break;

		case CTRL_TIME_NORMAL:
			if(!CurrentMission.isMultiPlayer()){
				setSpeed(1);
			}
			break;
		case CTRL_TIME_DEC:
			if(!CurrentMission.isMultiPlayer()){
				decrSpeed();
			}
			break;
		case CTRL_TIME_INC:
			if(!CurrentMission.isMultiPlayer()){
				incrSpeed();
			}
			break;

		case CTRL_CAMERA_MOUSE_LOOK:
			if(IsMapArea(mousePosition()))
				cameraMouseTrack = true;
			break;

		case CTRL_SHOW_UNIT_PARAMETERS:
			UI_LogicDispatcher::instance().toggleShowAllParametersMode(true);
			break;

		case CTRL_TOGGLE_MUSIC:
			InitSound(terSoundEnable, !terMusicEnable, gb_RenderDevice->GetWindowHandle(), getLocDataPath());
			if(terMusicEnable)
				musicManager.Resume();
			else
				musicManager.Pause();
			GameOptions::instance().setOption(OPTION_MUSIC_ENABLE, !terMusicEnable);
			GameOptions::instance().userApply(true);
			UI_LogicDispatcher::instance().handleMessageReInitGameOptions();
			break;
		case CTRL_TOGGLE_SOUND:
			InitSound(!terSoundEnable, terMusicEnable, gb_RenderDevice->GetWindowHandle(), getLocDataPath());
			GameOptions::instance().setOption(OPTION_SOUND_ENABLE, !terSoundEnable);
			GameOptions::instance().userApply(true);
			UI_LogicDispatcher::instance().handleMessageReInitGameOptions();
			break;

		case CTRL_MAKESHOT:
			MakeShot();
			break;

	}
}

void GameShell::KeyUnpressed(sKey& Key)
{
	UI_InputEvent event(UI_INPUT_KEY_UP, mousePosition());
	event.setKeyCode(Key.fullkey);
	if(UI_Dispatcher::instance().handleInput(event))
		return;

	ControlUnpressed(Key.fullkey);
}

void GameShell::ControlUnpressed(int key)
{
	if(!ControlManager::instance().key(CTRL_CAMERA_MOUSE_LOOK).pressed()){
		cameraMouseTrack = false;
		UI_LogicDispatcher::instance().showCursor();
	}

	if(!ControlManager::instance().key(CTRL_SHOW_UNIT_PARAMETERS).pressed())
		UI_LogicDispatcher::instance().toggleShowAllParametersMode(false);
}

void GameShell::updateMouse(const Vect2f& pos)
{
	mousePositionDelta_ = pos - mousePosition_;
	mousePosition_ = pos;
}

void GameShell::MouseMove(const Vect2f& pos, int flags)
{
	cameraCursorInWindow = true;

	updateMouse(pos);

	if(MousePositionLock)
		mousePositionDelta_ = Vect2f::ZERO;
	else
		MouseMoveFlag = 1;

	if(!cameraMouseZoom && !cameraMouseTrack)
		UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_MOVE, mousePosition(), flags));
}

void GameShell::MouseMidPressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	ControlPressed(sKey(VK_MBUTTON, true).fullkey);
	UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_MBUTTON_DOWN, mousePosition(), flags));
}
void GameShell::MouseMidUnpressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	ControlUnpressed(VK_MBUTTON);
	UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_MBUTTON_UP, mousePosition(), flags));
}

void GameShell::MouseMidDoubleClick(const Vect2f& pos, int flags)
{
	//ControlPressed(sKey(VK_MBUTTON, true).fullkey);
	//UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_MBUTTON_DOWN, pos + Vect2f(0.5f, 0.5f), flags));
}

void GameShell::MouseLeftPressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	if(!cameraMouseZoom && !cameraMouseTrack){
		if(UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_LBUTTON_DOWN, mousePosition(), flags)))
			return;
	}
	
	ControlPressed(sKey(VK_LBUTTON, true).fullkey);
}

void GameShell::MouseRightPressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	if(!cameraMouseZoom){
		if(UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_RBUTTON_DOWN, mousePosition(), flags)))
			return;
	}
	
	ControlPressed(sKey(VK_RBUTTON, true).fullkey);
}

void GameShell::MouseLeftUnpressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_LBUTTON_UP, mousePosition(), flags));
	
	ControlUnpressed(VK_LBUTTON);
}

void GameShell::MouseRightUnpressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_RBUTTON_UP, mousePosition(), flags));
	
	ControlUnpressed(VK_RBUTTON);
}

void GameShell::MouseWheel(int delta)
{
	if(UI_Dispatcher::instance().handleInput(UI_InputEvent(delta > 0 ? UI_INPUT_MOUSE_WHEEL_UP : UI_INPUT_MOUSE_WHEEL_DOWN, mousePosition())))
		return;

	if(GameActive && controlEnabled())
		cameraManager->mouseWheel(delta);

	ControlPressed(sKey(delta > 0 ? VK_WHEELUP : VK_WHEELDN, true).fullkey);
}

void GameShell::MouseLeave()
{
	cameraCursorInWindow = false;
}

void GameShell::OnWindowActivate()
{
	cameraMouseTrack = false;
	UI_LogicDispatcher::instance().showCursor();
}

void GameShell::MouseLeftDoubleClick(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	if(UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_LBUTTON_DBLCLICK, mousePosition(), flags)))
		return;
	
	ControlPressed(sKey(VK_LDBL, true).fullkey);
}

void GameShell::MouseRightDoubleClick(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	if(UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_RBUTTON_DBLCLICK, mousePosition(), flags)))
		return;
	
	ControlPressed(sKey(VK_RDBL, true).fullkey);
}

//----------------------------------

void GameShell::ShotsScan()
{
	shotNumber_ = 0;

	_mkdir(terScreenShotsPath);

	const char* name = win32_findfirst((string(terScreenShotsPath) + "\\*" + terScreenShotExt).c_str());
	if(name){
		do{
			const char* p = strstr(name, terScreenShotName);
			if(p){
				p += strlen(terScreenShotName);
				if(isdigit(*p)){
					int t = atoi(p) + 1;
					if(shotNumber_ < t)
						shotNumber_ = t;
				}
			}
			name = win32_findnext();
		} while(name);
	}
}

void GameShell::MakeShot()
{
	if(shotNumber_ == -1)
		ShotsScan();
	XBuffer fname(MAX_PATH);
	fname < terScreenShotsPath < "\\" < terScreenShotName <= shotNumber_/1000 % 10 <= shotNumber_/100 % 10 <= shotNumber_/10 % 10 <= shotNumber_ % 10 < terScreenShotExt;
	shotNumber_++;
	gb_RenderDevice->SetScreenShot(fname);
}

#define SAVE_TO_AVI
void GameShell::startStopRecordMovie()
{
	recordMovie_ = !recordMovie_;

	if(recordMovie_){
		frame_time.set(0, framePeriod_, terMaxTimeInterval);
		scale_time.set(0, framePeriod_, terMaxTimeInterval);

#ifndef SAVE_TO_AVI
		movieShotNumber_ = 0;
#endif
		movieStartTime_ = frame_time();
		
		_mkdir(terMoviePath);
		
		int movieNumber = 0;
		const char* name = win32_findfirst((string(terMoviePath) + "\\" + terMovieName + "*").c_str());
		if(name){
			do{
				const char* p = strstr(name, terMovieName);
				if(p){
					p += strlen(terMovieName);
					if(isdigit(*p)){
						int t = atoi(p) + 1;
						if(movieNumber < t)
							movieNumber = t;
					}
				}
				name = win32_findnext();
			} while(name);
		}
		XBuffer buffer;
		buffer < terMoviePath < "\\" < terMovieName <= movieNumber/10 % 10 <= movieNumber % 10;
		movieName_ = buffer;
		_mkdir(movieName_.c_str());
#ifdef SAVE_TO_AVI
		movieShotNumber_++;
		XBuffer fname(MAX_PATH);
		fname < movieName_.c_str() < "\\" < terMovieFrameName <= movieShotNumber_/1000 % 10 <= movieShotNumber_/100 % 10 <= movieShotNumber_/10 % 10 <= movieShotNumber_ % 10 < ".avi";
		int dx=gb_RenderDevice->GetSizeX(),dy=gb_RenderDevice->GetSizeY();
		video.Open(fname,dx,dy,StandartFrameRate_);
#endif
	} 
	else{
		frame_time.set(synchroByClock_, framePeriod_, terMaxTimeInterval);
		scale_time.set(synchroByClock_, framePeriod_, terMaxTimeInterval);
	}
}

void GameShell::makeMovieShot()
{
	bool ok=false;
#ifdef SAVE_TO_AVI
	ok=video.WriteFrame();//Make avi file.

	if(video.GetDataSize()>1500000000u)
	{
		movieShotNumber_++;
		XBuffer fname(MAX_PATH);
		fname < movieName_.c_str() < "\\" < terMovieFrameName <= movieShotNumber_/1000 % 10 <= movieShotNumber_/100 % 10 <= movieShotNumber_/10 % 10 <= movieShotNumber_ % 10 < ".avi";
		video.Open(fname,video.GetSize().x,video.GetSize().y,StandartFrameRate_);
	}
#else

	XBuffer fname(MAX_PATH);
	fname < movieName_.c_str() < "\\" < terMovieFrameName <= movieShotNumber_/1000 % 10 <= movieShotNumber_/100 % 10 <= movieShotNumber_/10 % 10 <= movieShotNumber_ % 10 < terScreenShotExt;
	movieShotNumber_++;
	ok=gb_RenderDevice->SetScreenShot(fname);
#endif
	int dx=gb_RenderDevice->GetSizeX();
	int offset=10,size=10;
	gb_RenderDevice->DrawRectangle(dx-offset-size,offset,size,size,ok?sColor4c(0,255,0):sColor4c(255,0,0));
}


//----------------------------

void GameShell::cameraQuant(float frameDeltaTime)
{
	cameraCursor_ = 0;

	//сдвиг когда курсор у края окна
	if(!selectMouseTrack && !cameraMouseTrack && cameraCursorInWindow && !cameraMouseZoom && controlEnabled()){
		if(int dir = cameraManager->mouseQuant(mousePosition()))
			cameraCursor_ = UI_GlobalAttributes::instance().getMoveCursor(dir);
	}
	
	MousePositionLock = 0;
	bool needLockMouse = false;
	
	static int lockState;
	//поворот мышью
	if(cameraMouseTrack && (MouseMoveFlag || lockState)){
		if(MouseMoveFlag && controlEnabled()){
			needLockMouse = true;
			cameraManager->tilt(mousePositionDelta());
		}
		cameraCursor_ = underFullDirectControl() ? UI_GlobalAttributes::instance().cursor(UI_CURSOR_ROTATE_DIRECT_CONTROL) : UI_GlobalAttributes::instance().cursor(UI_CURSOR_ROTATE);
	}
	else if(underFullDirectControl() && !cameraManager->interpolation())
		if(MouseMoveFlag && controlEnabled()){
			if(!isPaused(PAUSE_BY_ANY)){
				needLockMouse = true;
				cameraManager->rotate(mousePositionDelta());
			}
		}
	
	if(needLockMouse){
		MousePositionLock = 1;
		setCursorPosition(Vect2f::ZERO);
	}
	else
		MousePositionLock = 0;

	lockState = cameraMouseTrack;

	cameraManager->quant(mousePositionDelta().x, mousePositionDelta().y, frameDeltaTime);
	
	MouseMoveFlag = 0;

	if(controlEnabled() && !UI_Dispatcher::instance().hasFocusedControl())
		cameraManager->controlQuant(
			Vect2i(
				(ControlManager::instance().key(CTRL_CAMERA_MOVE_RIGHT).pressed() ? 1 : 0) 
				- (ControlManager::instance().key(CTRL_CAMERA_MOVE_LEFT).pressed() ? 1 : 0), 
				(ControlManager::instance().key(CTRL_CAMERA_MOVE_DOWN).pressed() ? 1 : 0)
				- (ControlManager::instance().key(CTRL_CAMERA_MOVE_UP).pressed() ? 1 : 0)),
			Vect2i(
				(ControlManager::instance().key(CTRL_CAMERA_ROTATE_LEFT).pressed() ? 1 : 0) 
				- (ControlManager::instance().key(CTRL_CAMERA_ROTATE_RIGHT).pressed() ? 1 : 0),
				(ControlManager::instance().key(CTRL_CAMERA_ROTATE_DOWN).pressed() ? 1 : 0) 
				- (ControlManager::instance().key(CTRL_CAMERA_ROTATE_UP).pressed() ? 1 : 0)), 
			(ControlManager::instance().key(CTRL_CAMERA_ZOOM_DEC).pressed() ? 1 : 0) 
			- (ControlManager::instance().key(CTRL_CAMERA_ZOOM_INC).pressed() ? 1 : 0));
}

//------------------------------------------
void GameShell::setSpeed(float d)
{
	MTAuto mtlock(*lock_logic);
	game_speed = clamp(d, 0, 10);
	scale_time.setSpeed(game_speed);
	gb_VisGeneric->SetLogicTimePeriodInv(game_speed*logicTimePeriodInv);

	UI_LogicDispatcher::instance().setGamePause(game_speed < FLT_EPS);
	//if (game_speed!=0 && soundPushedByPause) {
	//	soundPushedByPause = false;
	//	SNDPausePop();
	//	xassert(soundPushedPushLevel==SNDGetPushLevel());
	//	soundPushedPushLevel=INT_MIN;
	//} else if (game_speed==0 && !soundPushedByPause) {
	//	soundPushedByPause = true;
	//	xassert(soundPushedPushLevel==INT_MIN);
	//	soundPushedPushLevel=SNDGetPushLevel();
	//	SNDPausePush();
	//}
}

void GameShell::setCursorPosition(const Vect2f& pos)
{
	mousePosition_ = pos;
	Vect2i ps = convertToScreenAbsolute(pos);
	SetCursorPos(ps.x, ps.y);
}

void GameShell::setCursorPosition(const Vect3f& posW)
{
	Vect3f v, e;
	cameraManager->GetCamera()->ConvertorWorldToViewPort(&posW, &v, &e);
	
	if(!terFullScreen)
	{
		e.x *= float(windowClientSize().x)/gb_RenderDevice->GetSizeX();
		e.y *= float(windowClientSize().y)/gb_RenderDevice->GetSizeY();
		
		POINT pt = {e.x, e.y};
		::ClientToScreen(gb_RenderDevice->GetWindowHandle(), &pt);	
		
		e.x = pt.x; e.y = pt.y;
	}
	else{
		e.x *= float(windowClientSize().x)/gb_RenderDevice->GetSizeX();
		e.y *= float(windowClientSize().y)/gb_RenderDevice->GetSizeY();
	}
	
	SetCursorPos(e.x, e.y);
}

void GameShell::setCountDownTime(int timeLeft) {
	countDownTimeMillisLeft = timeLeft;
}

const string& GameShell::getCountDownTime() {
	if (countDownTimeMillisLeft != countDownTimeMillisLeftVisible) {
		countDownTimeMillisLeftVisible = countDownTimeMillisLeft;
		countDownTimeLeft = formatTimeWithoutHour(countDownTimeMillisLeftVisible);
	}
	return countDownTimeLeft;
}

string GameShell::getTotalTime() const {
	return formatTimeWithHour(gameTimer());
}

void GameShell::updateMap()
{
	//if (terMapPoint) {
	//	terMapPoint->UpdateMap(Vect2i(0,0), Vect2i((int)vMap.H_SIZE-1,(int)vMap.V_SIZE-1) );
	//}
	vMap.WorldRender();
}

void GameShell::showReelModal(const char* binkFileName, const char* soundFileName, bool localizedVideo, bool localizedVoice, bool stopBGMusic, int alpha)
{
	string path;
	string pathSound;
	if (localizedVoice) 
		pathSound = string(getLocDataPath()) + string("Voice\\") + soundFileName;
	else 
		pathSound = soundFileName;

	if (localizedVideo)
		path = string(getLocDataPath()) + string("Video\\") + binkFileName;
	else
		path = binkFileName;

	reelManager.showModal(path.c_str(), pathSound.c_str(), stopBGMusic, alpha);
	if (stopBGMusic) {
		musicManager.Stop();
	}
}

void GameShell::showPictureModal(const char* pictureFileName, bool localized, int stableTime) {
	string path;
	if (localized) {
		path = string(getLocDataPath()) + string("Video\\") + pictureFileName;
	} else {
		path = pictureFileName;
	}
	reelManager.showPictureModal(path.c_str(), stableTime);
}

void GameShell::showLogoModal(LogoAttributes logoAttributes, const cBlobsSetting& blobsSetting, bool localized, int stableTime,SoundLogoAttributes& soundAttributes)

{
	//string path = pictureFileName;

	reelManager.showLogoModal(logoAttributes, blobsSetting, stableTime,soundAttributes);
	//if(gb_RenderDevice3D->IsPS20())
	//	reelManager.showLogoModal(logoAttributes, blobsSetting, stableTime,soundAttributes);
	//else	
	//	reelManager.showPictureModal(logoAttributes.bkgName.c_str(), stableTime);
}


/*void GameShell::preLoad() {
	string path = string(getLocDataPath()) + "Text\\Texts.btdb";
	#ifdef _FINAL_VERSION_
		qdTextDB::instance().load(path.c_str(), 0 );
	#else
		qdTextDB::instance().load(path.c_str(), "RESOURCE\\Texts_comments.btdb");
	#endif
}*/
void GameShell::debugApply()
{
	if(environment)
		environment->setShowFogOfWar(!debugDisableFogOfWar);
}

void GameShell::editParameters()
{
	gb_RenderDevice->Flush();
	ShowCursor(1);

	bool reloadParameters = false;
    
	const char* header = "Заголовок миссии";
	const char* mission = "Миссия";
	const char* missionAll = "Миссия все данные";
	const char* debugPrmTitle = "Debug.prm";
	const char* global = "Глобальные параметры";
	const char* attribute = "Атрибуты";
	const char* sounds = "Звуки";
	const char* interface_ = "Интерфейс";
	const char* physics = "Физические параметры";
	const char* unitAttributes = "Параметры юнитов";
	const char* explode = "Параметры взрывов";
	const char* sources = "Источники";
	const char* projectileAttributes = "Аттрибуты снарядов";
	const char* gameSettings = "Game settins";
	const char* keySettings = "Настройки клавиатуры";
	const char* separator = "--------------";

	vector<const char*> items;
	items.push_back(header);
	items.push_back(mission);
	items.push_back(separator);
	items.push_back(global);
	items.push_back(gameSettings);
	items.push_back(keySettings);
	items.push_back(attribute);
	items.push_back(sounds);
	items.push_back(interface_);
	items.push_back(unitAttributes);
	items.push_back(projectileAttributes);
	items.push_back(physics);
	items.push_back(sources);
	items.push_back(explode);
	items.push_back(separator);
	items.push_back(missionAll);
	items.push_back(debugPrmTitle);

	const char* item = popupMenu(items);
	if(!item)
		return;
	else if(item == header){
		if(EditArchive(0, TreeControlSetup(0, 0, 500, 500, "Scripts\\TreeControlSetups\\currentMission")).edit(CurrentMission)){
			//SavePrm data;
			//CurrentMission.loadMission(data);
			//CurrentMission.saveMission(data, false);
		}
	}
	else if(item == mission){
		//if(EditArchive().edit(universe()->savePrm(), CurrentMission.saveName())){
			//SavePrm data = universe()->savePrm();
			//CurrentMission.saveMission(data, false);
		//}
	}
	else if(item == missionAll){
		//if(EditArchive().edit(universe()->savePrm(), CurrentMission.saveName())){
			//SavePrm data = universe()->savePrm();
			//CurrentMission.saveMission(data, false);
		//}
	}
	else if(item == debugPrmTitle){
		DebugPrm::instance().editLibrary();
		debugApply();
	}
	else if(item == attribute){
		AttributeLibrary::instance().editLibrary();
		universe()->refreshAttribute();
	}
	else if(item == global){
		GlobalAttributes::instance().editLibrary();
	}
	else if(item == sounds){
		SoundAttributeLibrary::instance().editLibrary();
		ApplySoundParameters();
	}
	else if(item == interface_){
		UI_Dispatcher::instance().editLibrary();
	}
	else if(item == physics){
		RigidBodyPrmLibrary::instance().editLibrary();
		universe()->refreshAttribute();
	}
	else if(item == sources){
		SourcesLibrary::instance().editLibrary();
	}
	else if(item == explode){
		ExplodeTable::instance().editLibrary();
	}
	else if(item == unitAttributes){
		AttributeLibrary::instance().editLibrary();
	}
	else if(item == projectileAttributes){
		AttributeProjectileTable::instance().editLibrary();
	}
	else if(item == gameSettings){
		if(GameOptions::instance().editLibrary())
			GameOptions::instance().userApply(true);
	}
	else if(item == keySettings){
		ControlManager::instance().editLibrary();
	}


	if(reloadParameters){
		if(universe())
			universe()->refreshAttribute();
	}

	cameraManager->setFocus(HardwareCameraFocus);
	ShowCursor(0);				
	restoreFocus();
}

bool GameShell::startMissionSuspended(const MissionDescription& mission)
{
	if(mission.valid()){
		terminateMission_ = true;
		startMissionSuspended_ = true;
		missionToStart_ = mission;
		return true;
	}
	else{
		UI_Dispatcher::instance().messageBox(GET_LOC_STR(UI_COMMON_TEXT_ERROR_OPEN));
		return false;
	}
}

void GameShell::decrSpeedStep() 
{
	if (game_speed > 0) {
		if (game_speed <= 1) {
			setSpeed(0.5f);
		} else if (game_speed <= 2) {
			setSpeed(1);
		}
	}
}

void GameShell::incrSpeedStep() 
{
	if (game_speed >= 1) {
		setSpeed(2);
	} else if (game_speed >= 0.5f) {
		setSpeed(1);
	}
}

void GameShell::pauseGame(PauseType pauseType) 
{
	if(universe() && universe()->isMultiPlayer())
		getNetClient()->setPause(true);
	else 
		pauseType_ |= pauseType;

	sndSystem.Mute3DSounds(pauseType_);
	voiceManager.Pause();
}

void GameShell::resumeGame(PauseType pauseType) 
{
	if(universe() && universe()->isMultiPlayer())
		getNetClient()->setPause(false);
	else 
		pauseType_ &= ~pauseType;
	
	sndSystem.Mute3DSounds(pauseType_);
	voiceManager.Resume();
}

void GameShell::checkEvent(const Event& event)
{
	globalTrigger_.checkEvent(event);

	if(universe()){
		switch(event.type()){
		case Event::KEYBOARD_CLICK:
		case Event::UI_BUTTON_CLICK_LOGIC:
		case Event::CHANGE_ACTIVE_PLAYER:
		case Event::CHANGE_PLAYER_AI:
			universe()->sendCommand(netCommand4G_Event(event));
			break;
		default:
			universe()->checkEvent(event);
		}
	}
}

void GameShell::loadProgressUpdate()
{
	UI_LogicDispatcher::instance().loadProgressUpdate();
}

void GameShell::SetFontDirectory()
{
}

class HoverUnitOperator
{
	UnitReal* nearestUnit_;
	bool wasExactHit_; 
	const Vect3f &v0, &v1;
	float& nearestDistance_;
	const Player* currentPlayer_;

public:
	HoverUnitOperator(const Vect3f& _v0, const Vect3f& _v1, float& distMin, bool requiredExactHit, const Player* player)
		: v0(_v0)
		, v1(_v1)
		, nearestDistance_(distMin)
		, wasExactHit_(requiredExactHit)
		, nearestUnit_(0)
		, currentPlayer_(player)
		{}

		void operator() (UnitReal* unit){
			Vect3f ret;
			if(unit && unit->selectAble() && !unit->hiddenGraphic() && (!unit->isUnseen() || unit->player() == currentPlayer_)){
				if(const cObject3dx* model = unit->model()){
					float dist = unit->position().distance2(v0);
					if(wasExactHit_){
						if(dist < nearestDistance_
							&& (unit->modelLogic() ? unit->modelLogic()->IntersectBound(v0, v1) : model->IntersectBound(v0, v1))
							&& model->Intersect(v0, v1))
						{
							nearestDistance_ = dist;
							nearestUnit_ = unit;
						}
					} else if((nearestUnit_ || dist < nearestDistance_)
						&& (unit->attr().selectBySphere
							? (unit->modelLogic() ? unit->modelLogic()->IntersectSphere(v0, v1) : model->IntersectSphere(v0, v1))
							: (unit->modelLogic() ? unit->modelLogic()->IntersectBound(v0, v1) : model->IntersectBound(v0, v1))
							))
					{
						if((wasExactHit_ = model->Intersect(v0, v1)) || dist < nearestDistance_){
							nearestDistance_ = dist;
							nearestUnit_ = unit;
						}
					}
				}
				else{
					Vect3f v0x = unit->position() - v0;
					Vect3f v01 = v1 - v0;
					Vect3f v_normal, v_tangent;
					decomposition(v01, v0x, v_normal, v_tangent);
					float dist = v_normal.norm2();
					if(dist < nearestDistance_ && v_tangent.norm2() < sqr(unit->radius()) ){
						nearestDistance_ = dist;
						nearestUnit_ = unit;
					}
				}
			}
		}

		UnitReal* getUnit() const { return nearestUnit_; }
};

UnitReal* GameShell::unitHover(const Vect3f& v0, const Vect3f& v1, float& distMin) const
{
	MTG();
	start_timer_auto();

	xassert(universe());
	HoverUnitOperator hoverUnitOp(v0, v1, distMin, underFullDirectControl(), universe()->activePlayer());

	VisibleUnits::const_iterator ui;
	FOR_EACH(visibleUnits_, ui)
		if((*ui)->attr().isObjective())
			hoverUnitOp(safe_cast<UnitObjective*>(*ui));

	return hoverUnitOp.getUnit();
}

class UnitsInAreaOp
{
public:
	UnitsInAreaOp(const Rectf& dev, UnitInterfaceList& out_list)
	: out_list_(out_list)
	{
		area = UI_Render::instance().device2screenCoords(dev);
	}

	void operator() (UnitInterface* unit, bool vip = false)
	{
		if(unit->alive() && unit->selectAble()){
			Vect3f scr_pos;
			int scr_radius;
			cameraManager->GetCamera()->ConvertorWorldToViewPort(&unit->position(), unit->radius() / (vip ? .5f : 2.f) , &scr_pos, &scr_radius);
			if(area.rect_overlap(Recti(scr_pos.xi() - scr_radius / 2, scr_pos.yi() - scr_radius / 2, scr_radius, scr_radius)))
				out_list_.push_back(unit);
		}
	}
	

private:
	UnitInterfaceList& out_list_;
	Recti area;
};

void GameShell::unitsInArea(const Rectf& dev, UnitInterfaceList& out_list, UnitInterface* preferendUnit, const AttributeBase* attr_filter) const
{
	MTG();

	UnitsInAreaOp op(dev, out_list);

	if(preferendUnit)
		op(preferendUnit, true);

	Player* player = universe()->activePlayer();
	xassert(player);

	if(attr_filter){
		CUNITS_LOCK(player);
		const RealUnits& unit_list=player->realUnits(attr_filter);
		RealUnits::const_iterator it;
		FOR_EACH(unit_list, it)
			if((*it)->attr().isActing())
				op(safe_cast<UnitInterface*>(*it));
	}
	else {
		VisibleUnits::const_iterator it;
		FOR_EACH(visibleUnits_, it)
			if((*it)->player() == player && (*it)->attr().isActing())
				op(safe_cast<UnitInterface*>(*it));
	}
}

//---------------------------------------------------

void GameShell::playerDisconnected(string& playerName, bool disconnectOrExit)
{
	string txt = GET_LOC_STR(UI_COMMON_TEXT_PLAYER_DISCONNECTED);
	txt += playerName;
	UI_LogicDispatcher::instance().handleChatString(ChatMessage(txt.c_str(), -1));
}

void GameShell::showConnectFailedInGame(const vector<string>& playerList)
{
	UI_LogicDispatcher::instance().getNetCenter().setPausePlayerList(playerList);
}

void GameShell::hideConnectFailedInGame(bool connectionRestored)
{
	UI_LogicDispatcher::instance().getNetCenter().setPausePlayerList(ComboStrings());
}

void GameShell::addStringToChatWindow(const ChatMessage& chatMessage)
{
	UI_LogicDispatcher::instance().handleChatString(chatMessage);
}

void GameShell::networkMessageHandler(eNetMessageCode message)
{
	if(NetClient)
		LogMsg("Handling net message-%s\n", NetClient->getStrNetMessageCode(message) );
	UI_LogicDispatcher::instance().handleNetwork(message);
}

bool GameShell::isKeyEnabled(sKey key) const
{
	switch(key.key){
	case VK_CAPITAL:
	case VK_NUMLOCK:
	case VK_SCROLL:
	case VK_LWIN:
	case VK_RWIN:
	case VK_APPS:
		return false;
	case VK_F4:
		if(key.menu)
			return false;
		break;
	}

	return true;
}
