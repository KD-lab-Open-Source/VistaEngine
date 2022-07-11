#include "StdAfx.h"
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
#include "ZipConfig.h"

#include "CheatManager.h"
#include "controls.h"

#include "RenderObjects.h"

#include "P2P_interface.h"
#include "Water\Water.h"
#include "Render\src\Scene.h"
#include "Render\src\VisGeneric.h"
#include "Console.h"
#include "DebugPrm.h"
#include "VistaRender\postEffects.h"
#include "Environment\Environment.h"
#include "Environment\SourceManager.h"
#include "VistaRender\FieldOfView.h"
#include "FileUtils\FileUtils.h"
#include "Serialization\SerializationFactory.h"
#include "UnicodeConverter.h"
#include "Joystick.h"

#include "Sound.h"
#include "SoundSystem.h"
#include "vmap.h"
#include "EnginePrm.h"

#include "Triggers.h"
#include "TriggerEditor\TriggerEditor.h"

#include "TextDB.h"
#include "PlayBink.h"
#include "SelectManager.h"

#include "kdw/PropertyEditor.h"
#include "Serialization\XPrmArchive.h"
#include "ConsoleWindow.h"

#include "GameLoadManager.h"
#include "UI_Render.h"
#include "UI_BackgroundScene.h"
#include "UserInterface.h"
#include "UI_Logic.h"
#include "UI_NetCenter.h"
#include "UI_Minimap.h"
#include "GameOptions.h"
#include "CommonLocText.h"
#include "ShowHead.h"
#include "WBuffer.h"
#include "AI\PFTrap.h"
#include "UI_Logic.h"
#include "UI_StreamVideo.h"
extern Singleton<UI_StreamVideo> streamVideo;

#include "Physics\crash\CrashSystem.h"

#include "Water\SkyObject.h"
#include "Terra\terTools.h"

#include "StreamCommand.h"
#include "Render\3dx\Lib3dx.h"
#include "Render\Src\TexLibrary.h"
#include "Render\D3D\D3DRender.h"
#undef XREALLOC
#undef XFREE
#include "Game\IniFile.h"
#include "kdw/LibraryEditorDialog.h"

#include "LogMsg.h"

Vect3f G2S(const Vect3f &vg, Camera* camera);

extern float HardwareCameraFocus;

GameShell* gameShell = 0;

int terShowFPS = 0;

const char* terScreenShotsPath = "ScreenShots";
const char* terPanoScreenShotsPath = "Pano";
const char* terScreenShotName = "shot";
const char* terScreenShotExt = ".bmp";
const char* terMoviePath = "Movie";
const char* terMovieName = "Track";
const char* terMovieFrameName = "frame";

class PathInitializer
{
public:
	PathInitializer(const char* path){
		if(path)
			SetCurrentDirectory(path);
	}
};

#pragma warning(disable: 4073 4355)
#pragma init_seg(lib)
static PathInitializer pathInitializer(check_command_line("project_path"));

void checkGameSpyCmdLineArg(const char* argument) 
{
	if(!argument) 
		ErrH.Abort("Interface.Menu.Messages.GameSpyCmdLineError");
}

Runtime* createRuntime(HINSTANCE hInstance)
{
	start_timer_auto();
	return new GameShell(hInstance, iniFile.HT);
}

//------------------------
GameShell::GameShell(HINSTANCE hInstance, bool useHT) :
	kbDriver_(this),
	Runtime(hInstance, useHT),
	globalTrigger_(*new TriggerChain)
{
	start_timer(1);
	gameShell = this;

	if(iniFile.ZIP)
		ZipConfig::initArchives();

	UI_Render::instance().init();

	loadAllLibraries();
	GameOptions::instance().gameSetup();

	InitDirectInput(gb_RenderDevice->GetWindowHandle());

	eventProcessing_ = false;

	controlEnabled_ = true;
	isCutScene_ = false;

	currentMissionPopulation_ = 0;
	ratingDelta_ = 0;
	frameDeltaTimeAvr_ = 0.02f;

	setLogicFp();

	needLogicCall_ = true;
	interpolation_timer_ = 0;
	sleepGraphicsTime_ = 0;
	sleepGraphics_ = false;

	gameReadyCounter_ = 0;

	terminateMission_ = false;
	startMissionSuspended_ = false;
	//stopNetClientSuspended = false;

	logicFps_ = logicFpsMin_ = logicFpsMax_ = logicFpsTime_ = 1;

	synchroByClock_ = iniFile.SynchroByClock;
	StandartFrameRate_ = iniFile.StandartFrameRate;
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

	soundPushedByPause = false;
	soundPushedPushLevel=INT_MIN;

	gb_VisGeneric->SetUseTextureCache(true);
	gb_VisGeneric->SetUseMeshCache(true);

	globalTrigger_.load("Scripts\\Content\\Triggers\\GlobalTrigger.scr");

	GameActive = false;

	disableVideo_ = iniFile.DisableVideo;
	reelManager.sizeType = ReelManager::FULL_SCREEN;

	reelAbortEnabled = true;

	mainMenuEnabled_ = iniFile.MainMenu;

	debug_allow_replay = iniFile.EnableReplay;

	check_command_line_parameter("mainmenu", mainMenuEnabled_);

	panoramicShotMode_ = false;
	panoramicShotNumber_ = -1;
	shotNumber_ = -1;
	recordMovie_ = false;
	movieNumber_ = 0;

	game_speed = 1;
	pauseType_ = 0;

	countDownTimeLeft = "";
	countDownTimeMillisLeft = -1;
	countDownTimeMillisLeftVisible = -1;
	
	cameraMouseTrack = false;
	selectMouseTrack = false;

	directControl_ = DIRECT_CONTROL_DISABLED;
	
	MouseMoveFlag = 0;
	MousePositionLock = 0;
	
	mousePosition_ = Vect2f::ZERO;
	mousePositionDelta_ = Vect2f::ZERO;
	
	activePlayerID_ = 0;
	
	cameraCursor_ = 0;

	stop_timer(1);
	start_timer(2);
	EffectContainer::setTexturesPath("Resource\\FX\\Textures");
	for(EffectLibrary::Strings::const_iterator it = EffectLibrary::instance().strings().begin(); it != EffectLibrary::instance().strings().end(); ++it)
		if(it->get())
			it->get()->preloadLibrary();
	stop_timer(2);

	start_timer(3);
	UI_Dispatcher::instance().init();
	stop_timer(3);

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
			GameStart(MissionDescription(fname, GAME_TYPE_REEL));
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
				PNetCenter::startMPWithoutInterface(name.c_str());
			else if(check_command_line(COMMAND_LINE_DW_HOST)|| check_command_line(COMMAND_LINE_DW_CLIENT))
				PNetCenter::startDWMPWithoutInterface(name.c_str());
			else{

				MissionDescription missionDescription(name.c_str(), gameType);

				if(check_command_line("battle"))
					missionDescription.setUseMapSettings(false);
				GameStart(missionDescription);
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

GameShell::~GameShell()
{
	//if(soundPushedByPause)
	//	SNDPausePop();

	GameClose();

	//deleteNetClient();
	PNetCenter::destroyNetCenter();

	UI_Dispatcher::instance().releaseResources();

//	ZIPClose();

	FreeDirectInput();

	delete &globalTrigger_;

	gameShell = 0;
}

void GameShell::terminate()
{
	UI_Dispatcher::instance().exitGamePrepare();
	GameContinue = false;
}

void GameShell::onClose()
{
	if(debugDisableSpecialExitProcess)
		terminate();
	else
		checkEvent(Event(Event::GAME_CLOSE));
}

//bool GameShell::isNetClientConfigured(e_PNCWorkMode workMode)
//{
//	if(isNetCenterCreated && PNetCenter::instance()->getWorkMode() == workMode)
//		return true;
//	return false;
//}

//void GameShell::createNetClient(ExternalNetTask_Init* entInit)
//{
//	deleteNetClient();
//	NetClient = new PNetCenter(entInit);
//}

//void GameShell::stopNetClient()
//{
//	if(!stopNetClientSuspended){
//		if(universeX())
//			universeX()->stopNetCenter();
//		stopNetClientSuspended = true;
//	}
//}
//
//void GameShell::deleteNetClient() 
//{
//	delete NetClient;
//	NetClient = 0;
//	stopNetClientSuspended = false;
//}

void showLoadProgress()
{
	UI_Dispatcher::instance().quickRedraw();
}

void GameShell::GameLoad(const MissionDescription& mission)
{
	start_timer_auto();

	GameLoadManager::instance().startLoad(DebugPrm::instance().debugLoadTime, useHT() ? 0 : showLoadProgress);
	
	start_timer(1);

	terScene->Compact();

	setControlEnabled(true);

	isCutScene_ = false;
	pauseType_ = 0;

	terminateMission_ = false;
	defaultSaveName_ = "";

	interpolation_timer_ = 0;

	//cVisGeneric::SetAssertEnabled(false);
	CurrentMission = mission;

	WBuffer buf;
	currentMissionPopulation_ = CurrentMission.gamePopulation();
	for (int i = 0; i < CurrentMission.playersAmountMax(); i++) {
		SlotData& data = CurrentMission.changePlayerData(i);
		if(data.realPlayerType==REAL_PLAYER_TYPE_PLAYER){
			for(int j=0; j<NETWORK_TEAM_MAX; j++){
				if(data.usersIdxArr[j]!=USER_IDX_NONE){
					if(*(CurrentMission.getPlayerName(buf, i, j)) == 0)
						CurrentMission.setPlayerName(i, j, UI_LogicDispatcher::instance().currentPlayerDisplayName(buf));
				}
			}
		}
	}

	stop_timer(1);

	start_timer(2);
	vMap.load(CurrentMission.worldName());
	stop_timer(2);

	start_timer(3);

	xassert( checkLogicFp() );
	terToolsDispatcher.prepare4World();
	
	setSpeed(iniFile.GameSpeed);

	cameraManager->reset();

	stop_timer(3);
	start_timer(4);

	dassert(!selectManager);
	selectManager = new SelectManager;

	GameLoadManager::instance().setProgressAndStartSub(.05f, .2f);

	MissionDescription missionTemp = CurrentMission;
	XPrmIArchive ia;
	new UniverseX(missionTemp, ia.open(missionTemp.saveName()) ? &ia : 0);

	universe()->setUseHT(useHT());
	if(universe()->userSave())
		serializeUserSave(ia);
	else{
		if(GlobalAttributes::instance().directControlMode){
			RealUnits::const_iterator ui;
			FOR_EACH(universe()->activePlayer()->realUnits(), ui){
				UnitReal* unit(*ui);
				if(unit->attr().defaultDirectControlEnabled){
					selectManager->selectUnit(unit, false);
					safe_cast<UnitActing*>(unit)->setActiveDirectControl(GlobalAttributes::instance().directControlMode, 0);
				}
			}
		}
	}

	stop_timer(4);
	start_timer(5);

	musicManager.OpenWorld();

	universe()->setControlEnabled(!(CurrentMission.gameType() & GAME_TYPE_REEL));

	UI_Dispatcher::instance().setEnabled(mission.enableInterface);

	if(CurrentMission.gameSpeed)
		setSpeed(CurrentMission.gameSpeed);

	if(!CurrentMission.isMultiPlayer() && CurrentMission.isGamePaused())
		pauseGame(PAUSE_BY_USER);

	setCountDownTime(-1);

	setSilhouetteColors();
	
	UI_LogicDispatcher::instance().SetSelectPeram(universe()->activePlayer()->race()->selection_param);
	UI_LogicDispatcher::instance().createSignSprites();

	if(CurrentMission.silhouettesEnabled)
		gb_VisGeneric->EnableSilhouettes(GameOptions::instance().getBool(OPTION_SILHOUETTE));		
	else
		gb_VisGeneric->EnableSilhouettes(false);		

	GameOptions::instance().gameSetup();

	GameLoadManager::instance().finishAndStartSub(1.f);

	universe()->relaxLoading();

	UI_Dispatcher::instance().relaxLoading();

	UI_Dispatcher::instance().preloadScreen(!preloadScreen_.empty()
		? UI_ScreenReference(preloadScreen_).screen()
		: (CurrentMission.isBattle() ? universe()->activePlayer()->race()->screenToPreload.screen() : 0),
		terScene, universe()->activePlayer());

	preloadScreen_.clear();

	stop_timer(5);

	GameLoadManager::instance().finishLoad();
}

void GameShell::GameRelaxLoading()
{
	MTL();
	MTG();

	start_timer_auto();

	for(int j = 0; j < 4; j++){
		universe()->Quant();
		universe()->interpolationQuant();
		universe()->streamCommand.process(0);
		universe()->streamCommand.clear();
		universe()->streamInterpolator.process(0);
		universe()->streamInterpolator.clear();
		gb_VisGeneric->SetGraphLogicQuant(universe()->quantCounter());
	}

	UI_LogicDispatcher::instance().hideCursor();

	GameActive = true;

	SNDSetGameActive(true);
	sndSystem.SetStandbyTime(1);
	SNDSetFade(true,3000);

	quantTimeStatistic.reset(useHT());

	GetTexLibrary()->MarkAllLoaded();
	pLibrary3dx->MarkAllLoaded();
	pLibrarySimply3dx->MarkAllLoaded();

	ratingDelta_ = 0;
	if(PNetCenter::isNCConfigured(PNCWM_ONLINE_DW) && CurrentMission.isMultiPlayer() && currentMissionPopulation_ >= 0){
	}

	frame_time.skip();
	scale_time.skip();
	loadFpsTimer_.stop();

	needLogicCall_ = true;
	sleepGraphicsTime_ = 0;
	sleepGraphics_ = false;
}

void GameShell::GameClose()
{
	start_timer_auto();

	Runtime::GameClose();

	if(PNetCenter::isNCCreated() && CurrentMission.isMultiPlayer()){
		sendStats(true, universe()->activePlayer()->isWin());
		PNetCenter::instance()->FinishGame();
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

	UI_Dispatcher::instance().reset();

	destroyScene();
	createScene();

	UI_Dispatcher::instance().resetScreens();

	delete selectManager;
	selectManager = 0;

	scale_time.setSpeed(1);
	frame_time.skip();
	scale_time.skip();

	//if (soundPushedByPause) {
	//	soundPushedByPause = false;
	//	SNDPausePop();
	//	xassert(soundPushedPushLevel==SNDGetPushLevel());
	//	soundPushedPushLevel=INT_MIN;
	//}

	//SNDStopAll();

	//musicManager.CloseWorld();
//	cVisGeneric::SetAssertEnabled(true);
}

void GameShell::sendStats(bool final, bool win)
{
	if(PNetCenter::isNCConfigured(PNCWM_ONLINE_DW) && currentMissionPopulation_ >= 0){
	}
}

bool GameShell::universalSave(const char* name, bool userSave)
{
	MTAuto lock(gb_RenderDevice3D->resetDeviceLock());
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
	if(ar.openStruct(*this, "userInterface", "userInterface")){
		UI_Dispatcher::instance().serializeUserSave(ar);
		ar.serialize(*selectManager, "selectManager", 0);
	}
	ar.closeStruct("userInterface");
}

//void GameShell::NetQuant()
//{
//	if(NetClient){
//		NetClient->quant_th1();
//		if(stopNetClientSuspended)
//			deleteNetClient();
//	}
//}

bool GameShell::logicQuant()
{
	start_timer_auto();

	setLogicFp();

	MTAuto lock(gb_RenderDevice3D->resetDeviceLock());

	int begQuantTime = xclock();
	if(universeX()->PrimaryQuant()){
		selectionQuant();
		directControlQuant();
		UI_Dispatcher::instance().logicQuant();

		SoundQuant();
		sndSystem.Update();

		if(sendStatsTimer_.finished()){
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
	if((needLogicCall_ || !universe()->isMultiPlayer()) && !interfaceLogicCallTimer_.busy()){
		start_timer_auto();
		interfaceLogicCallTimer_.start(100);
		UI_Dispatcher::instance().logicQuant();
		universe()->triggerQuant(true);
		sndSystem.Update();
	}
}

void GameShell::logicQuantST()
{
	start_timer_auto();
	
	//NetQuant();
	PNetCenter::netQuant();

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

	PNetCenter::netQuant();

	if(!GameActive)
		return;

	if(logicQuant()){
		while(!needLogicCall_ && !terminateLogicThread()){
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

void GameShell::processEvents()
{
	eventProcessing_ = true;
	WindowEventQueue::const_iterator it;
	FOR_EACH(windowEventQueue_, it)
		EventParser(it->uMsg, it->wParam, it->lParam);
	windowEventQueue_.clear();
	eventProcessing_ = false;
}

void GameShell::graphicsQuant()
{
	start_timer_auto();

	UpdateDirectInputState();

	if(!GameActive && useHT() && !load_mode)
		PNetCenter::netQuant(); //NetQuant();

	if(GameActive && useHT()){
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

	if(cameraManager && !load_mode)
		cameraManager->SetFrustumGame();

	frame_time.next_frame();

	unsigned int begQuantTime = xclock();
	bool firstGraphFrame = false;
	if(universe() && !load_mode){
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
				if(useHT())
		            ::Sleep(1);
				stop_timer(wait);
				return;
			}
		}

		//xassert(interpolation_timer_ <= logicTimePeriod);
		float interpolationFactor = clamp(interpolation_timer_, 0, logicTimePeriod)*logicTimePeriodInv;

		streamLock.lock();
		gb_VisGeneric->	SetGraphLogicQuant(universe()->quantCounter());
		streamLock.unlock();

		streamCommand_.process(interpolationFactor);
		streamCommand_.clear();

		streamInterpolator_.process(interpolationFactor);

		float scaleDeltaTime = scale_time.delta()/1000.f;
		float frameDeltaTime = frame_time.delta()/1000.f;

		processEvents();
	
		average(frameDeltaTimeAvr_, frameDeltaTime, GlobalAttributes::instance().frameTimeAvrTau);
		cameraQuant(frameDeltaTimeAvr_);

		UI_Dispatcher::instance().quant(frameDeltaTime);

		Show(scaleDeltaTime);

		if(panoramicShotMode_){
			makePanoramicShot(0.f);
			panoramicShotMode_ = false;
		}

		if(terminateMission_)
			UI_Dispatcher::instance().exitMissionPrepare();
		if(terminateMission_ && UI_Dispatcher::instance().canExit())
			GameClose();	
	}
	else if(!loadFpsTimer_.busy()){ // MainMenu, только графический поток
		MT_SET_TLS(MT_GRAPH_THREAD | MT_LOGIC_THREAD);
		loadFpsTimer_.start(1000 / 15); // 15 fps при отсутствии логического потока

		interpolation_timer_ += scale_time.delta();
		if(interpolation_timer_ > logicTimePeriod){
			needLogicCall_ = true;
			if(!load_mode)
				global_time.next_frame();
			interpolation_timer_ -= logicTimePeriod;
		}

		if(!load_mode){
			SoundQuant();
			sndSystem.Update();
		}

		processEvents();
		UI_Dispatcher::instance().quant(frame_time.delta()/1000.0f, load_mode);
		UI_Dispatcher::instance().logicQuant();
		
		if(!load_mode)
			globalTrigger_.quant(false);

		Show(scale_time.delta()/1000.0f);
	}

	if(!load_mode){
		if(startMissionSuspended_)
			UI_Dispatcher::instance().exitMissionPrepare();
		if(startMissionSuspended_ && UI_Dispatcher::instance().canExit()){
			startMissionSuspended_ = false;
			GameStart(missionToStart_);
			gameReadyCounter_ = 5;
		}

		if(PNetCenter::isNCCreated() && universe() && universe()->isMultiPlayer() && !--gameReadyCounter_){
			PNetCenter::instance()->setGameIsReady();
			quantTimeStatistic.reset(useHT());
		}
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
	if(!isPaused(GameShell::PAUSE_BY_ANY))
		UI_LogicDispatcher::instance().directShootQuant();

//	if(!underFullDirectControl())
	if(!directControl())
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

	if(JoystickSetup::instance().isEnabled()){
		Vect2i v = Vect2i(joystickState.controlState(JoystickSetup::instance().controlID(JOY_MOVEMENT_X)),
			joystickState.controlState(JoystickSetup::instance().controlID(JOY_MOVEMENT_Y)));

		if(abs(v.x) > JoystickSetup::instance().axisDeltaMin())
			direction |= v.x < 0 ? DIRECT_KEY_TURN_LEFT : DIRECT_KEY_TURN_RIGHT;
		if(abs(v.y) > JoystickSetup::instance().axisDeltaMin())
			direction |= v.y < 0 ? DIRECT_KEY_MOVE_FORWARD : DIRECT_KEY_MOVE_BACKWARD;

		if(joystickState.isControlPressed(JoystickSetup::instance().controlSetup(JOY_PRIMARY_WEAPON)))
			direction |= DIRECT_KEY_MOUSE_LBUTTON;
		if(joystickState.isControlPressed(JoystickSetup::instance().controlSetup(JOY_SECONDARY_WEAPON)))
			direction |= DIRECT_KEY_MOUSE_RBUTTON;
	}

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
			if(cameraManager->directControlUnit() == unit || cameraManager->directControlUnitPrev() == unit)
				cameraManager->disableDirectControl();

		directControl_ = mode;
	}else
		streamLogicCommand.set(fCommandSetDirectControl) << mode << unit << transitionTime;
}

void GameShell::Show(float realGraphDT)
{
	start_timer_auto();
	if(useHT())
		profiler_quant();

	start_timer(1);
	Console::instance().graphQuant();
	stop_timer(1);

	if(GameActive){
		start_timer(2);

		terScene->SetDeltaTime(realGraphDT * 1000);

		Color4c& Color = environment->environmentTime()->GetCurFoneColor();
		gb_RenderDevice->Fill(Color.r,Color.g,Color.b);
		gb_RenderDevice->BeginScene();

		gb_RenderDevice->SetRenderState(RS_FILLMODE, debugWireFrame ? FILL_WIREFRAME : FILL_SOLID);
		
		UI_LogicDispatcher::instance().selectCursor(cameraCursor_);
		UI_LogicDispatcher::instance().moveCursorEffect();

		stop_timer(2);
		start_timer(3);

		environment->graphQuant(realGraphDT, cameraManager->GetCamera());

		cameraManager->GetCamera()->setAttribute(ATTRCAMERA_CLEARZBUFFER);//Потому как в небе могут рисоваться планеты в z buffer.
		terScene->Draw(cameraManager->GetCamera());

		environment->drawPostEffects(realGraphDT, cameraManager->GetCamera());
		
		gb_RenderDevice->SetRenderState(RS_FILLMODE, FILL_SOLID);

		stop_timer(3);
		start_timer(4);

		UI_LogicDispatcher::instance().clearSquadSelectSigns();

		if(!isCutScene()){
			bool showEnabled = UI_Dispatcher::instance().isEnabled();
			start_timer(1);
			VisibleUnits::iterator ui;
			FOR_EACH(visibleUnits_, ui){
				UnitBase* unit = *ui;
				//if(!unit->dead())
					if(unit->attr().isEnvironment() || showEnabled && !(unit->hiddenGraphic() & ~UnitBase::HIDE_BY_UPGRADE))
						unit->graphQuant(realGraphDT);
			}
			stop_timer(1);

			universe()->graphQuant(realGraphDT);

			sourceManager->drawUI(realGraphDT);
		}
		
		if(debugShowEnabled)
			drawDebugInfo();

		gb_RenderDevice->SetDrawTransform(cameraManager->GetCamera());
		gb_RenderDevice->FlushPrimitive3D();

		gb_RenderDevice->SetClipRect(0,0,gb_RenderDevice->GetSizeX(),gb_RenderDevice->GetSizeY());

		UI_Dispatcher::instance().redraw();

		cameraManager->drawBlackBars();

		universe()->drawDebug2D();
	
		stop_timer(4);
		start_timer(5);

#ifndef _FINAL_VERSION_
		if(DebugPrm::instance().showCurrentAI && universe()->activePlayer()->isAI() && UI_Dispatcher::instance().isEnabled()){
			WBuffer buffer;
			buffer < L"AI " < universe()->activePlayer()->difficulty()->name();
			UI_Render::instance().outText(Rectf(0.9f,0,0,0), buffer);
		}
#endif

		if(recordMovie_)
			makeMovieShot();

		if(DebugPrm::instance().showFieldOfViewMap)
			environment->fieldOfViewMap()->debugDraw(cameraManager->GetCamera());
	
		gb_RenderDevice->EndScene();

		gb_RenderDevice->Flush();

		stop_timer(5);
	}
	else{
		//draw
		gb_RenderDevice->Fill(0,0,0);
		gb_RenderDevice->BeginScene();

		UI_LogicDispatcher::instance().selectCursor(0);
		UI_Dispatcher::instance().redraw();

		gb_RenderDevice->EndScene();
		gb_RenderDevice->Flush();
	}
}

void GameShell::drawDebugInfo()
{
	Camera* camera = cameraManager->GetCamera();
	
	show_dispatcher.draw(camera);

	if(showDebugUnitBase.visibleUnit){
		const UnitBase* unit = 0;
		VisibleUnits::const_iterator ui;
		FOR_EACH(visibleUnits_, ui)
			if((unit = *ui) && !unit->dead()){
				Vect3f vs(G2S(unit->position(), camera));
				if(vs.z > 1)
					gb_RenderDevice->OutText(vs.xi() - 5, vs.yi() - 5, unit->attr().isSquad() ? "squad" : (unit->attr().isReal() ? "real" : "unit"), Color4f(0.f, 1.f, 0.f, 1.f));
			}
	}

	UI_Dispatcher::instance().drawDebugInfo();
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

bool GameShell::checkReel(UINT uMsg,WPARAM wParam,LPARAM lParam) 
{
	if(reelManager.isVisible()) {
		if(reelAbortEnabled) {
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

void GameShell::onSetFocus(bool focus)
{
	__super::onSetFocus(focus);

	if(PNetCenter::isNCCreated() && !alwaysRun_)
		PNetCenter::instance()->setPause(!focus);
}

void GameShell::onSetCursor()
{
	__super::onSetCursor();

	UI_LogicDispatcher::instance().updateCursor();
}

void GameShell::eventHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(eventProcessing_)
		return;

	if(checkReel(uMsg, wParam, lParam)) {
		return;
	}

	kbDriver_.handleKeyMessage(uMsg, wParam, lParam);

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

	handle(uMsg, wParam, lParam);
}

void GameShell::handle(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	xassert(!eventProcessing_);
	windowEventQueue_.push_back(WindowEvent(uMsg, wParam, lParam));
}

void GameShell::EventParser(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		sKey key(addModifiersState(wParam));
		if(isKeyEnabled(key))
			KeyPressed(key, lParam & 0x40000000);
		break;			}
	case WM_KEYUP:
	case WM_SYSKEYUP:	{
		if(wParam == VK_SNAPSHOT){ // WM_KEYDOWN для PrintScreen не приходит никогда
			sKey key(addModifiersState(wParam));
			if(isKeyEnabled(key))
				KeyPressed(key, false);
		}

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

	case WM_UNICHAR:
	case WM_USER + WM_CHAR:
		if((lParam & 1<<31) == 0)
			unicodeCharInput(wchar_t(LOWORD(wParam)), (lParam & 0xFF) != 0);
		break;
	}
}

void GameShell::unicodeCharInput(wchar_t chr, bool isAutoRepeat)
{
	UI_InputEvent event(UI_INPUT_CHAR, mousePosition_);
	event.setCharInput(chr);

	UI_Dispatcher::instance().handleInput(event);
}

void GameShell::charInput(int chr)
{
}

void GameShell::ShowStatistic()
{
	MTAuto lock(gb_RenderDevice3D->resetDeviceLock());
	void ShowGraphicsStatistic();
	ShowGraphicsStatistic();
}

class DebugUnitGenerator{
public:
	DebugUnitGenerator()
	: playerID_(0)
	, number_(1)
	, inTheSameSquad_(false)
	, position_(Vect3f::ZERO)
	, mousePosition_(Vect2f::ZERO)
	{}
	void set(int playerID, Vect3f position, Vect2f mousePosition)
	{
		playerID_ = playerID;
		position_ = position;
		mousePosition_ = mousePosition;
	}
	void serialize(Archive& ar){
		ar.serialize(attr_, "attr", "Юнит");
		ar.serialize(playerID_,"playerID","Игрок");
		ar.serialize(number_,"number","Количество");
		ar.serialize(inTheSameSquad_,"inTheSameSquad","В одном скваде");
	}
	void generate(){
		playerID_ = clamp(playerID_, 0, universe()->Players.size()-1);
		UnitSquad* squad = 0;
		if(attr_){
			for(int i = 0; i < number_; i++){
				Player* player = universe()->Players[playerID_];
				if(UnitBase* unit = player->buildUnit(attr_)){
					cameraManager->cursorTrace(mousePosition_, position_);
					if(number_ > 1)
						position_ += Mat3f(2*M_PI*i/number_, Z_AXIS)*Vect3f(unit->radius()*2, 0, 0);
					unit->setPose(Se3f(QuatF::ID, position_),true);
					if(unit->attr().isLegionary()){
						UnitLegionary* legionary = safe_cast<UnitLegionary*>(unit);
						if(!squad || !inTheSameSquad_ || !squad->checkFormationConditions(legionary->attr().formationType)){
							squad = safe_cast<UnitSquad*>(player->buildUnit(&*legionary->attr().squad));
							squad->setPose(unit->pose(), true);
						}
						squad->addUnit(legionary);
					}
				}
			}
		}
	}			
	AttributeReference attr_;
	int playerID_;
	int number_;
	bool inTheSameSquad_;
	Vect2f mousePosition_;
	Vect3f position_;
};
bool GameShell::DebugKeyPressed(sKey& Key)
{
	MTAutoSkipAssert mtAutoSkipAssert;

	switch(Key.fullkey){
	case VK_SCROLL:
		ConsoleWindow::instance().show(!ConsoleWindow::instance().isVisible());
		break;
	case VK_F11 | KBD_CTRL | KBD_MENU: // make panoramic screenshot
#ifndef _FINAL_VERSION_
		panoramicShotMode_ = true;
#endif
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

			if(kdw::edit(Serializer(attr), "Scripts\\TreeControlSetups\\unitAttributeState", 0, hWnd()))
				AttributeLibrary::instance().saveLibrary();
		}
		break;
		
	case VK_RETURN | KBD_SHIFT | KBD_MENU: 
		if(selectManager->selectionSize() == 1 && selectManager->uniform()){
			UnitBase* unit = selectManager->selectedUnit();
			if(unit->attr().isSquad())
				unit = safe_cast<UnitSquad*>(unit)->getUnitReal();
			kdw::edit(Serializer(*unit), "Scripts\\TreeControlSetups\\unitState", 0, hWnd());
		}
		break;


#ifndef _FINAL_VERSION_
	case VK_RETURN | KBD_CTRL: 
	case VK_RETURN | KBD_CTRL | KBD_SHIFT: {
		gb_RenderDevice->Flush();
		ShowCursor(1);
		TriggerChain* triggerChain = &globalTrigger_;
		if(universe()){
			Player* player = universe()->activePlayer();
			if(isShiftPressed()){
				XBuffer nameAlt;
				nameAlt < "Игрок (0-" <= universe()->Players.size() - 1 < ")";
				int playerID = player->playerID();
				if(kdw::edit(Serializer(playerID, "playerID", nameAlt), "Scripts\\TreeControlSetups\\chooseTrigger", 0, hWnd())){
					player = universe()->Players[clamp(playerID, 0, universe()->Players.size() - 1)];
				}
			}
			triggerChain = player->getStrategyToEdit();
		}
		if(triggerChain && TriggerEditor(*triggerChain, gb_RenderDevice->GetWindowHandle()).edit()){
			triggerChain->save();
			triggerChain->buildLinks();
			triggerChain->setAIPlayerAndTriggerChain(universe()->activePlayer());
		}

		cameraManager->setFocus(HardwareCameraFocus);
		ShowCursor(0);				
		restoreFocus();									
		break; }
#endif

	case VK_F6: 
	case VK_F6 | KBD_CTRL: 
	case VK_F6 | KBD_SHIFT: 
		if(!profilerStarted_)
			profiler_start_stop(isShiftPressed() ? PROFILER_MEMORY : (isControlPressed() ? PROFILER_ACCURATE : PROFILER_REBUILD));
		else{
			gb_RenderDevice->Flush();
			{
				MTAuto lock(gb_RenderDevice3D->resetDeviceLock());
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
		environment->water()->SetRainConstant(+0.3f);
		break;
	case '2' | KBD_CTRL | KBD_SHIFT:
		environment->water()->SetRainConstant(-0.3f);
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
	case 'F' | KBD_SHIFT | KBD_CTRL:
		{
			UnitInterface* unit = selectManager->selectedUnit();
			if(unit && unit->getUnitReal())
				unit->getUnitReal()->setRiseFur(2000);
		}
		break;

	case 'P':
		ShowStatistic();
		break;

	case 'P' | KBD_CTRL:
		//if(selectManager){
		//	UnitActing* unit = dynamic_cast<UnitActing*>(selectManager->selectedUnit());
		//	if(unit){
		//		static CommandsQueueReference queue;
		//		if(kdw::edit(Serializer(queue, "queue", "Очередь команд"), "Scripts\\TreeControlSetups\\commandsQueueState", 0, hWnd()))
		//			unit->executeCommandsQueue(*queue);
		//	}
		//}
		break;

	case 'U' | KBD_CTRL:
		{
			static DebugUnitGenerator unitGenerator;
			Vect3f pos;
			cameraManager->cursorTrace(mousePosition(), pos);
			unitGenerator.set(universe()->activePlayer()->playerID(), pos, mousePosition());
			if(kdw::edit(Serializer(unitGenerator), "Scripts\\TreeControlSetups\\createUnitState", 0, hWnd()))
				unitGenerator.generate();
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

	case 'T' | KBD_CTRL | KBD_SHIFT:
		if(environment && !universe()->isMultiPlayer()){
			float cur_time = environment->environmentTime()->GetTime(); 
			environment->environmentTime()->SetTime((floor(cur_time * 2.f) + 1.f)/2.f);
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

	if(!PNetCenter::isNCCreated() && !isPaused(PAUSE_BY_ANY) && !isAutoRepeat)
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
			if(!CurrentMission.isMultiPlayer()) {
				if(CurrentMission.enablePause) {
					if(!isPaused(PAUSE_BY_USER))
						pauseGame(PAUSE_BY_USER);
					else
						resumeGame(PAUSE_BY_USER);
				}
			}
            else if(universe() && PNetCenter::isNCCreated() && universe()->isMultiPlayer())
				PNetCenter::instance()->setPause(!PNetCenter::instance()->isPause());
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
		
		case CTRL_CAMERA_TO_EVENT:
			minimap().cameraToEvent();
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
	if(cameraMouseTrack && !ControlManager::instance().key(CTRL_CAMERA_MOUSE_LOOK).pressed()){
		cameraMouseTrack = false;
		UI_LogicDispatcher::instance().showCursor();
	}
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

	if(!cameraMouseTrack)
		UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_MOVE, mousePosition(), flags));
}

void GameShell::MouseMidPressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	if(UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_MBUTTON_DOWN, mousePosition(), flags)))
		return;

	ControlPressed(addModifiersState(VK_MBUTTON));
}

void GameShell::MouseMidUnpressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	ControlUnpressed(VK_MBUTTON);
	UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_MBUTTON_UP, mousePosition(), flags));
}

void GameShell::MouseMidDoubleClick(const Vect2f& pos, int flags)
{
	//ControlPressed(addModifiersState(VK_MBUTTON));
	//UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_MBUTTON_DOWN, pos + Vect2f(0.5f, 0.5f), flags));
}

void GameShell::MouseLeftPressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	if(!cameraMouseTrack){
		if(UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_LBUTTON_DOWN, mousePosition(), flags)))
			return;
	}
	
	ControlPressed(addModifiersState(VK_LBUTTON));
}

void GameShell::MouseRightPressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	if(UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_RBUTTON_DOWN, mousePosition(), flags)))
		return;
	
	ControlPressed(addModifiersState(VK_RBUTTON));
}

void GameShell::MouseLeftUnpressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	//UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_LBUTTON_UP, mousePosition(), flags));
	
	ControlUnpressed(VK_LBUTTON);
}

void GameShell::MouseRightUnpressed(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	//UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_RBUTTON_UP, mousePosition(), flags));
	
	ControlUnpressed(VK_RBUTTON);
}

void GameShell::MouseWheel(int delta)
{
	if(UI_Dispatcher::instance().handleInput(UI_InputEvent(delta > 0 ? UI_INPUT_MOUSE_WHEEL_UP : UI_INPUT_MOUSE_WHEEL_DOWN, mousePosition())))
		return;

	if(GameActive && controlEnabled())
		cameraManager->mouseWheel(delta);

	ControlPressed(addModifiersState(delta > 0 ? VK_WHEELUP : VK_WHEELDN));
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
	
	ControlPressed(addModifiersState(VK_LDBL));
}

void GameShell::MouseRightDoubleClick(const Vect2f& pos, int flags)
{
	updateMouse(pos);

	if(UI_Dispatcher::instance().handleInput(UI_InputEvent(UI_INPUT_MOUSE_RBUTTON_DBLCLICK, mousePosition(), flags)))
		return;
	
	ControlPressed(addModifiersState(VK_RDBL));
}

//----------------------------------

void GameShell::ShotsScan()
{
	shotNumber_ = 0;

	_mkdir(terScreenShotsPath);

	DirIterator it((string(terScreenShotsPath) + "\\*" + terScreenShotExt).c_str());
	while(it != DirIterator::end){
		const char* p = strstr(*it, terScreenShotName);
		if(p){
			p += strlen(terScreenShotName);
			if(isdigit(*p)){
				int t = atoi(p) + 1;
				if(shotNumber_ < t)
					shotNumber_ = t;
			}
		}
		++it;
	}

	panoramicShotNumber_ = 0;

	DirIterator it1((string(terScreenShotsPath) + "\\" + terPanoScreenShotsPath + "*").c_str());
	while(it1 != DirIterator::end){
		const char* p = *it1;
		p += strlen(terPanoScreenShotsPath);
		if(isdigit(*p)){
			int t = atoi(p) + 1;
			if(panoramicShotNumber_ < t)
				panoramicShotNumber_ = t;
		}
		++it1;
	}
}

void GameShell::MakeShot()
{
	if(shotNumber_ == -1)
		ShotsScan();
	XBuffer fname(MAX_PATH);
	fname < terScreenShotsPath < "\\";

	if(panoramicShotMode_)
		fname < terPanoScreenShotsPath <= panoramicShotNumber_/100 % 10 <= panoramicShotNumber_/10 % 10 <= panoramicShotNumber_ % 10 < "\\";

	fname < terScreenShotName <= shotNumber_/1000 % 10 <= shotNumber_/100 % 10 <= shotNumber_/10 % 10 <= shotNumber_ % 10 < terScreenShotExt;

	shotNumber_++;
	gb_RenderDevice->SetScreenShot(fname);
}

void GameShell::makePanoramicShot(float draw_time)
{
	if(panoramicShotNumber_ == -1)
		ShotsScan();

	XBuffer fname(MAX_PATH);
	fname < terScreenShotsPath < "\\" < terPanoScreenShotsPath <= panoramicShotNumber_/100 % 10 <= panoramicShotNumber_/10 % 10 <= panoramicShotNumber_ % 10;
	_mkdir(fname.c_str());

	int shot_num = shotNumber_;
	shotNumber_ = 0;

	cameraManager->fixEyePosition();

	CameraCoordinate coord = cameraManager->coordinate();
	coord.psi() = 0.f;
	coord.theta() = G2R(panoScreenshotSetup.add_theta) + (M_PI - G2R(panoScreenshotSetup.delta_theta) * float(panoScreenshotSetup.count_theta))/2.f;
	cameraManager->setCoordinate(coord);
	cameraManager->update();

	Show(draw_time);
	cameraManager->rotateAroundEye(Vect2f::ZERO);
	cameraManager->update();
	Show(draw_time);
	cameraManager->rotateAroundEye(Vect2f::ZERO);
	cameraManager->update();
	Show(draw_time);
	cameraManager->rotateAroundEye(Vect2f::ZERO);
	cameraManager->update();

	for(int j = 0; j < panoScreenshotSetup.count_theta; j++){
		for(int i = 0; i < panoScreenshotSetup.count_psi; i++){
			MakeShot();
			cameraManager->rotateAroundEye(Vect2f(G2R(panoScreenshotSetup.delta_psi), 0.f));
			cameraManager->update();
			Show(draw_time);
		}
		cameraManager->rotateAroundEye(Vect2f(0.f, G2R(panoScreenshotSetup.delta_theta)));
		cameraManager->update();
		Show(draw_time);
	}

	shotNumber_ = shot_num;
	panoramicShotNumber_++;
}

void GameShell::startStopRecordMovie()
{
	recordMovie_ = !recordMovie_;

	if(recordMovie_){
		frame_time.set(0, framePeriod_, terMaxTimeInterval);
		scale_time.set(0, framePeriod_, terMaxTimeInterval);

		_mkdir(terMoviePath);
		
		DirIterator it((string(terMoviePath) + "\\" + terMovieName + "*").c_str());
		while(it != DirIterator::end){
			const char* p = strstr(*it, terMovieName);
			if(p){
				p += strlen(terMovieName);
				if(isdigit(*p)){
					int t = atoi(p) + 1;
					if(movieNumber_ < t)
						movieNumber_ = t;
				}
			}
			++it;
		}
		XBuffer buffer;
		buffer < terMoviePath < "\\" < terMovieName <= movieNumber_++ < ".avi";
		video.Open(buffer, gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY(), StandartFrameRate_);
	} 
	else{
		video.Close();
		frame_time.set(synchroByClock_, framePeriod_, terMaxTimeInterval);
		scale_time.set(synchroByClock_, framePeriod_, terMaxTimeInterval);
	}
}

void GameShell::makeMovieShot()
{
	bool ok = video.WriteFrame();//Make avi file.

	if(video.GetDataSize()>1500000000u){
		XBuffer buffer;
		buffer < terMoviePath < "\\" < terMovieName <= movieNumber_++ < ".avi";
		video.Open(buffer, gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY(), StandartFrameRate_);
	}

	int dx=gb_RenderDevice->GetSizeX();
	int offset=10,size=10;
	gb_RenderDevice->DrawRectangle(dx-offset-size,offset,size,size,ok ? Color4c(0,255,0):Color4c(255,0,0));
}


//----------------------------

void GameShell::cameraQuant(float frameDeltaTime)
{
	cameraCursor_ = 0;

	bool mouse_move = MouseMoveFlag;
	Vect2f delta = mousePositionDelta();
	if(JoystickSetup::instance().isEnabled()){
		Vect2i v = Vect2i(joystickState.controlState(JoystickSetup::instance().controlID(JOY_CAMERA_X)),
			joystickState.controlState(JoystickSetup::instance().controlID(JOY_CAMERA_Y)));

		if(abs(v.x) > JoystickSetup::instance().axisDeltaMin()){
			mouse_move = true;
			delta.x += float(v.x) * JoystickSetup::instance().cameraTurnSpeed() / 1000.f;
		}
		if(abs(v.y) > JoystickSetup::instance().axisDeltaMin()){
			mouse_move = true;
			delta.y += float(v.y) * JoystickSetup::instance().cameraTurnSpeed() / 1000.f;
		}
	}

	//сдвиг когда курсор у края окна
	if(!selectMouseTrack && !cameraMouseTrack && cameraCursorInWindow && controlEnabled()){
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
		if(mouse_move && controlEnabled()){
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

	float theta = clamp(cameraManager->coordinate().theta(), 0.01f, M_PI_2);
	universe()->setMaxSptiteVisibleDistance(M_PI * GlobalAttributes::instance().unitSpriteMaxDistance / (2.f * theta));
}

//------------------------------------------
void GameShell::setSpeed(float d)
{
	MTAuto mtlock(gb_RenderDevice3D->resetDeviceLock());
	game_speed = clamp(d, 0, 10);
	scale_time.setSpeed(game_speed);

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
	return formatTimeWithHour(global_time());
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
		pathSound = getLocDataPath("Voice\\") + soundFileName;
	else 
		pathSound = soundFileName;

	if (localizedVideo)
		path = getLocDataPath("Video\\") + binkFileName;
	else
		path = binkFileName;

	reelManager.showModal(path.c_str(), pathSound.c_str(), stopBGMusic, alpha);
	if (stopBGMusic) {
		musicManager.Stop();
	}
}

void GameShell::showPictureModal(const char* pictureFileName, bool localized, int stableTime) 
{
	string path = localized ? getLocDataPath("Video\\") + pictureFileName : pictureFileName;
	reelManager.showPictureModal(path.c_str(), stableTime);
}

void GameShell::showLogoModal(LogoAttributes logoAttributes, const cBlobsSetting& blobsSetting, bool localized, int stableTime,SoundLogoAttributes& soundAttributes)
{
	reelManager.showLogoModal(logoAttributes, blobsSetting, stableTime,soundAttributes);
}

void GameShell::debugApply()
{
	if(environment)
		universe()->setShowFogOfWar(!debugDisableFogOfWar);
	updateDefaultFont();
}

void GameShell::editParameters()
{
	gb_RenderDevice->Flush();
	ShowCursor(1);

	const char* libraryEditor = "Редактор библиотек (войск)";
	const char* enginePrm = "EnginePrm";
	const char* visGeneric = "VisGeneric";
	const char* debugPrmTitle = "Debug.prm";
	const char* globalAttribute = "Глобальные параметры";
	const char* globalEnvironment = "Глобальные параметры окружения";
	const char* sounds = "Звуки";
	const char* interfaceAttribute = "Интерфейс";
	const char* physics = "Физические параметры";
	const char* explode = "Параметры взрывов";
	const char* gameSettings = "Game settins";
	const char* keySettings = "Настройки клавиатуры";
	const char* joystickSettings = "Настройки джойстика";
	const char* separator = "--------------";

	vector<const char*> items;
	items.push_back(libraryEditor);
	items.push_back(debugPrmTitle);
	items.push_back(enginePrm);
	items.push_back(visGeneric);
	items.push_back(gameSettings);
	items.push_back(globalAttribute);
	items.push_back(globalEnvironment);
	items.push_back(physics);
	items.push_back(explode);
	items.push_back(keySettings);
	items.push_back(joystickSettings);
	items.push_back(sounds);
	items.push_back(interfaceAttribute);

	const char* item = popupMenu(items);
	if(!item)
		return;
	else if(item == libraryEditor){
		kdw::LibraryEditorDialog libraryEditor(hWnd());

		const char* configFileName = "Scripts\\TreeControlSetups\\LibraryEditorState";
		XPrmIArchive ia;
		if(ia.open(configFileName)){
			ia.setFilter(kdw::SERIALIZE_STATE);
			ia.serialize(libraryEditor, "libraryEditor", 0);
			ia.close();
		}
		if(libraryEditor.showModal("AttributeLibrary") == kdw::RESPONSE_OK)
			saveAllLibraries();

		XPrmOArchive oa(configFileName);
		oa.setFilter(kdw::SERIALIZE_STATE);
		oa.serialize(libraryEditor, "libraryEditor", 0);
		oa.close();
	}
	else if(item == debugPrmTitle){
		if(DebugPrm::instance().editLibrary())
			debugApply();
	}
	else if(item == visGeneric){
		gb_VisGeneric->editOption();
	}
	else if(item == globalAttribute){
		GlobalAttributes::instance().editLibrary();
		cameraManager->setCameraRestriction(GlobalAttributes::instance().cameraRestriction);
	}
	else if(item == sounds){
		SoundAttributeLibrary::instance().editLibrary();
		ApplySoundParameters();
	}
	else if(item == interfaceAttribute){
		UI_Dispatcher::instance().editLibrary();
	}
	else if(item == physics){
		RigidBodyPrmLibrary::instance().editLibrary();
	}
	else if(item == explode){
		ExplodeTable::instance().editLibrary();
	}
	else if(item == gameSettings){
		if(GameOptions::instance().editLibrary())
			GameOptions::instance().userApply(true);
	}
	else if(item == keySettings){
		ControlManager::instance().editLibrary();
	}
	else if(item == joystickSettings){
		JoystickSetup::instance().editLibrary();
	}
	else if(item == enginePrm){
		EnginePrm::instance().editLibrary();
	}
	else if(item == globalEnvironment){
		string setupName = string("Scripts\\TreeControlSetups\\") + globalEnvironment + "State";
		if(kdw::edit(Serializer(*environment, "", "", SERIALIZE_GLOBAL_DATA), setupName.c_str(), 0, hWnd()))
			environment->saveGlobalParameters();
	}

	cameraManager->setFocus(HardwareCameraFocus);
	ShowCursor(0);				
	restoreFocus();
}

bool GameShell::startMissionSuspended(const MissionDescription& mission, const char* preloadScreen)
{
	if(terminateMission_ || startMissionSuspended_ || load_mode)
		return false;

	if(mission.valid()){
		missionToStart_ = mission;
		if(preloadScreen && *preloadScreen)
			preloadScreen_ = preloadScreen;
		else
			preloadScreen_.clear();
		if(!missionToStart_.isBattle() && !missionToStart_.userSave() && !missionToStart_.isReplay())
			missionToStart_.setDifficulty(UI_LogicDispatcher::instance().currentProfile().difficulty);
		terminateMission_ = true;
		startMissionSuspended_ = true;
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
	if(universe() && PNetCenter::isNCCreated() && universe()->isMultiPlayer())
		PNetCenter::instance()->setPause(true);
	else 
		pauseType_ |= pauseType;

	sndSystem.Mute3DSounds(pauseType_);
	voiceManager().Pause();
	streamVideo().pause(true);
}

void GameShell::resumeGame(PauseType pauseType) 
{
	if(universe()&& PNetCenter::isNCCreated() && universe()->isMultiPlayer())
		PNetCenter::instance()->setPause(false);
	else 
		pauseType_ &= ~pauseType;
	
	sndSystem.Mute3DSounds(pauseType_);
	voiceManager().Resume();
	streamVideo().pause(false);
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

UnitReal* GameShell::unitHover(const Vect3f& v0, const Vect3f& v1, float& distMin) const
{
	MTG();
	start_timer_auto();
	xassert(universe());

	bool exactHit = underFullDirectControl();
	Player* player = universe()->activePlayer();
	UnitReal* unitMin = 0;
	// visibleUnits_ отсортированы по убыванию глубины, эффективннее проверять начиная с ближайших
	VisibleUnits::const_reverse_iterator ui = visibleUnits_.rbegin();
	VisibleUnits::const_reverse_iterator ui_end = visibleUnits_.rend();
	for(; ui != ui_end; ++ui){
		if(!(*ui)->attr().isObjective())
			continue;
		UnitReal* unit = safe_cast<UnitReal*>(*ui);
		if(unit->selectAble() && !unit->hiddenGraphic() && (!unit->isUnseen() || unit->player() == player)){
			Vect3f v;
			if(const cObject3dx* model = unit->model()){
				float dist = unit->position().distance2(v0);
				if(exactHit){
					if(dist < distMin
						&& (unit->modelLogic() ? unit->modelLogic()->IntersectBound(v0, v1) : model->IntersectBound(v0, v1))
						&& unit->intersect(v0, v1, v)){
						distMin = dist;
						unitMin = unit;
					}
				} 
				else if((unitMin || dist < distMin)
				  && (unit->attr().selectBySphere
				  ? (unit->modelLogic() ? unit->modelLogic()->IntersectSphere(v0, v1) : model->IntersectSphere(v0, v1))
				  : (unit->modelLogic() ? unit->modelLogic()->IntersectBound(v0, v1) : model->IntersectBound(v0, v1)))){
					if((exactHit = unit->intersect(v0, v1, v)) || dist < distMin){
						distMin = dist;
						unitMin = unit;
					}
				}
			}
		}
	}
	return unitMin;
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
			float scr_radius;
			cameraManager->GetCamera()->ConvertorWorldToViewPort(unit->position(), unit->radius() / (vip ? .5f : 2.f) , scr_pos, scr_radius);
			if(area.rect_overlap(Recti(scr_pos.xi() - round(scr_radius/2), scr_pos.yi() - round(scr_radius/2), round(scr_radius), round(scr_radius))))
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
	WBuffer msg;
	msg < GET_LOC_STR(UI_COMMON_TEXT_PLAYER_DISCONNECTED) < playerName.c_str();
	
	UI_LogicDispatcher::instance().handleSystemMessage(msg);
}

void GameShell::showConnectFailedInGame(const vector<string>& playerList)
{
	ComboWStrings wstrs;
	vector<string>::const_iterator it;
	FOR_EACH(playerList, it)
		wstrs.push_back(a2w(*it));
	UI_LogicDispatcher::instance().getNetCenter().setPausePlayerList(wstrs);
}

void GameShell::hideConnectFailedInGame(bool connectionRestored)
{
	UI_LogicDispatcher::instance().getNetCenter().setPausePlayerList(ComboWStrings());
}

void GameShell::addStringToChatWindow(const ChatMessage& chatMessage)
{
	UI_LogicDispatcher::instance().handleChatString(chatMessage);
}

//void GameShell::networkMessageHandler(eNetMessageCode message)
//{
//	if(PNetCenter::isNCCreated())
//		LogMsg("Handling net message-%s\n", PNetCenter::instance()->getStrNetMessageCode(message) );
//	UI_LogicDispatcher::instance().handleNetwork(message);
//}

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

void GameShell::onAbort()
{
	if(universeX()) 
		universeX()->allSavePlayReel();
	logMsgCenter.saveLog();
}

