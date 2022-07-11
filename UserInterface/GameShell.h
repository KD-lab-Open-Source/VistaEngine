#ifndef __GAME_SHELL_H__
#define __GAME_SHELL_H__

#include "Timers.h"
#include "NetPlayer.h"
#include "ReelManager.h"
#include "Render\Inc\fps.h"
#include "skey.h"
#include "VistaRender\StreamInterpolation.h"
#include "Game\Runtime.h"
#include "Util\Win32\KeyboardDriver.h"
#include "Render\Src\WinVideo.h"
#include "Network\quantTimeStatistic.h"
#include "UserInterface\Bubles\Blobs.h"
#include "Units\DirectControlMode.h"

class PNetCenter;
class ChatMessage;
enum e_PNCWorkMode;
struct SaveControlData;
class TriggerChain;
class Event;
class UI_Cursor;

class UnitBase;
typedef SwapVector<UnitBase*> VisibleUnits;
class UnitReal;

class UnitInterface;
typedef vector<UnitInterface*> UnitInterfaceList; 

template<typename scalar_type, class vect_type> struct Rect;
typedef Rect<float, Vect2f> Rectf;

//------------------------------------------
class GameShell : public Runtime, public IEventHandler
{
public:
	GameShell(HINSTANCE hInstance, bool useHT);
	~GameShell();

	void GameClose();
	void terminateMission() { terminateMission_ = true; }
	void terminate();

//	void startOnline(CommandLineData data);

	bool startMissionSuspended(const MissionDescription& mission, const char* preloadScreen = 0);

	bool universalSave(const char* name, bool userSave);
	void serializeUserSave(Archive& ar);
	
	void processEvents();
	void graphicsQuant();
	void Show(float dt);
	void drawDebugInfo();

	bool logicQuant();
	void logicQuantPause();
	void logicQuantST();
	void logicQuantHT();

	void selectionQuant();

	void setCountDownTime(int timeLeft);
	const string& getCountDownTime();
	string getTotalTime() const;

	const VisibleUnits& visibleUnits() const { MTG(); return visibleUnits_; }
	
//------------------------
	void onSetFocus(bool focus);
	void onSetCursor();
	void onClose();
	void onAbort();
	void eventHandler(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void handle(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void EventParser(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void KeyPressed(sKey &key, bool isAutoRepeat);
	void KeyUnpressed(sKey &key);
	void unicodeCharInput(wchar_t wch, bool isAutoRepeat);
	void charInput(int chr);
	bool eventProcessing_;
	bool DebugKeyPressed(sKey& Key);
	void debugApply();
	void editParameters();
	void ControlPressed(int key);
	void ControlUnpressed(int key);
	void MouseLeftPressed(const Vect2f& pos, int flags = 0);
	void MouseRightPressed(const Vect2f& pos, int flags = 0);
	void MouseRightUnpressed(const Vect2f& pos, int flags = 0);
	void MouseLeftUnpressed(const Vect2f& pos, int flags = 0);
	void checkEvent(const Event& event);

	void MouseMidPressed(const Vect2f& pos, int flags = 0);
	void MouseMidUnpressed(const Vect2f& pos, int flags = 0);
	void MouseMidDoubleClick(const Vect2f& pos, int flags = 0);

	void updateMouse(const Vect2f& pos);
	
	void MouseMove(const Vect2f& pos, int flags = 0);
	void MouseLeftDoubleClick(const Vect2f& pos, int flags = 0);
	void MouseRightDoubleClick(const Vect2f& pos,int flags = 0);
	void MouseWheel(int delta);
	void MouseLeave();
	void OnWindowActivate();

	void cameraQuant(float frameDeltaTime);

	Vect2f convert(int x, int y) const; // screen -> ([-0.5..0.5], [-0.5..0.5])
	Vect2i convertToScreenAbsolute(const Vect2f& pos); // Absolute screen for cursor positioning

	void updateMap();

//---------------------------------------
	void setSpeed(float speed);
	void decrSpeed() { setSpeed(game_speed*0.9f); }
	void incrSpeed() { setSpeed((game_speed + 0.01f)*1.1f); }
	void decrSpeedStep();
	void incrSpeedStep();
	float getSpeed() const { return game_speed; }

	enum PauseType { 
		PAUSE_BY_USER = 1,
		PAUSE_BY_MENU = 2,
		PAUSE_BY_ANY = PAUSE_BY_USER | PAUSE_BY_MENU
	};
	void pauseGame(PauseType pauseType);
	void resumeGame(PauseType pauseType);
	bool isPaused(int pauseType) const { return pauseType_ & pauseType; }
	bool recordMovie() const { return recordMovie_; }

	const Vect2f& mousePosition() const { return mousePosition_; }
	const Vect2f& mousePositionDelta() const { return mousePositionDelta_; }
	
	void setCursorPosition(const Vect2f& pos);
	void setCursorPosition(const Vect3f& posW);

	//void createNetClient(class ExternalNetTask_Init* ent); //e_PNCWorkMode _workMode
	//PNetCenter* getNetClient() { return NetClient; }
	//bool isNetClientConfigured(e_PNCWorkMode workMode);
	//void stopNetClient();

	bool GameActive;
	bool mainMenuEnabled_;

	/// отсылка команд управляемому юниту
	void directControlQuant();

	void setDirectControl(DirectControlMode mode, UnitInterface* unit, int transitionTime);

	// недетерминированные функции, пользоваться с осторожностью
	bool directControl() const { return directControl_; }
	bool underHalfDirectControl() const { return directControl_ & SYNDICATE_CONTROL_ENABLED; }
	bool underFullDirectControl() const { return directControl_ & DIRECT_CONTROL_ENABLED; }
	
	UnitReal* unitHover(const Vect3f& v0, const Vect3f& v1, float& distMin) const;
	// передается deviceCoords (-0.5f, -0.5f)-(0.5f, 0.5f)
	void unitsInArea(const Rectf& dev, UnitInterfaceList& out_list, UnitInterface* preferendUnit = 0, const AttributeBase* attr_filter = 0) const;

	int MouseMoveFlag;
	int MousePositionLock;
	
	bool   cameraMouseTrack;
	bool   cameraCursorInWindow;
	bool selectMouseTrack;

	MissionDescription CurrentMission;

	void showReelModal(const char* binkFileName, const char* soundFileName, bool localizedVideo = false, bool localizedVoice = false, bool stopBGMusic = true, int alpha = 255);
	void showPictureModal(const char* pictureFileName, bool localized, int stableTime);
	void showLogoModal(LogoAttributes logoAttributes, const cBlobsSetting& blobsSetting, bool localized, int stableTime,SoundLogoAttributes& soundAttributes);

	void setControlEnabled(bool controlEnabled) { controlEnabled_ = controlEnabled; }
	bool controlEnabled() { return controlEnabled_; }
	
	void setCutScene(bool isCutScene) { isCutScene_ = isCutScene; }
	bool isCutScene() const { return isCutScene_; }

	bool isVideoEnabled() const { return !disableVideo_; }

	//-----Network function-----
	//void NetQuant();

	void showConnectFailedInGame(const vector<string>& playerList);
	void hideConnectFailedInGame(bool connectionRestored = true);

	void playerDisconnected(string& playerName, bool disconnectOrExit);

	//void networkMessageHandler(enum eNetMessageCode message);

	void addStringToChatWindow(const ChatMessage& chatMessage);

	//-----end of network function----

	bool reelAbortEnabled;

	float logicFps() const { return logicFps_; }
	void logicFPSminmax(float& fpsmin,float& fpsmax,float& logic_time) const {
	  	fpsmin = logicFpsMin_; fpsmax = logicFpsMax_; logic_time = logicFpsTime_;
	}

	void ShowStatistic();
	int accessibleQuantPeriod() const { return quantTimeStatistic.accessibleQuantPeriod(); }

private:

	void GameLoad(const MissionDescription& mission);
	void GameRelaxLoading();

	struct WindowEvent 
	{
		WindowEvent(UINT msg, WPARAM wp, LPARAM lp) : uMsg(msg), wParam(wp), lParam(lp) {}
		UINT uMsg;
		WPARAM wParam;
		LPARAM lParam;
	};
	typedef vector<WindowEvent> WindowEventQueue;
	WindowEventQueue windowEventQueue_;

	bool needLogicCall_;
 	int interpolation_timer_;
	float sleepGraphicsTime_;
	bool sleepGraphics_;
	GraphicsTimer interfaceLogicCallTimer_;
	GraphicsTimer loadFpsTimer_;

	VisibleUnits visibleUnits_;
	StreamInterpolator streamInterpolator_;
	StreamInterpolator streamCommand_;

	FPSInterval logicFpsMeter_;
	float logicFps_;
	float logicFpsMin_;
	float logicFpsMax_;
	float logicFpsTime_;

	bool terminateMission_;
	bool startMissionSuspended_;
	MissionDescription missionToStart_;
	string preloadScreen_;
	bool controlEnabled_;
	bool isCutScene_;

	const UI_Cursor* cameraCursor_;

	Vect2f mousePosition_;
	Vect2f mousePositionDelta_;

	int activePlayerID_;

	float game_speed;
	int pauseType_;
	int currentMissionPopulation_;
	LogicTimer sendStatsTimer_;
	double ratingDelta_;

	//PNetCenter* NetClient;
	//bool stopNetClientSuspended;
	//void deleteNetClient();
		
 	string defaultSaveName_;

	int synchroByClock_;
	int framePeriod_;
	int StandartFrameRate_;
	float frameDeltaTimeAvr_;

	bool profilerStarted_;

	bool panoramicShotMode_;
	int panoramicShotNumber_;
	int shotNumber_;
	bool recordMovie_;
	int movieNumber_;

	int countDownTimeMillisLeft;
	int countDownTimeMillisLeftVisible;

	string countDownTimeLeft;
	string totalTimeLeft;

	KeyboardDriver kbDriver_;

	ReelManager reelManager;

	TriggerChain& globalTrigger_;

	bool disableVideo_;

	bool soundPushedByPause;
	int soundPushedPushLevel;

	sVideoWrite video;

	QuantTimeStatistic quantTimeStatistic;
	int gameReadyCounter_;

	/// прямое управление юнитом
	DirectControlMode directControl_;

	//---------------------------------
	void ShotsScan();
	void MakeShot();
	void makePanoramicShot(float draw_time);

	void startStopRecordMovie();
	void makeMovieShot();

	bool checkReel(UINT uMsg,WPARAM wParam,LPARAM lParam);

	//void startMPWithoutInterface(const char* missionName);
	//void startDWMPWithoutInterface(const char* missionName);

	bool isKeyEnabled(sKey key) const;

	void sendStats(bool final, bool win);

};

extern GameShell* gameShell;

extern int terShowFPS;

#endif
