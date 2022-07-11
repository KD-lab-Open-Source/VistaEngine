#ifndef __HYPERSPACE_H__
#define __HYPERSPACE_H__

#include "Universe.h"
#include "EventBufferDP.h"

enum NCEventID;
class InOutNetComBuffer;

extern const char * COMMAND_LINE_SAVE_PLAY_REEL;


class PNetCenter;

class UniverseX : public Universe
{
public:
	UniverseX(PNetCenter* net_client, MissionDescription& mission, XPrmIArchive* ia);
	~UniverseX();

	void Quant();

	bool SingleQuant();
	bool MultiQuant();
	bool PrimaryQuant();

	void SetServerSpeedScale(float scale){ ServerSpeedScale = scale; }

	void drawDebug2D() const;
	void select(UnitInterface* unit, bool shiftPressed = true, bool no_deselect = false);
	void deselect(UnitInterface* unit);
	void changeUnitNotify(UnitInterface* oldUnit, UnitInterface* newUnit);

	bool ReceiveEvent(NCEventID event, InOutNetComBuffer& in);
	void receiveCommand(const netCommandGame& command);
	void sendCommand(const netCommandGame& command);

	bool isMultiPlayer() const { return (!flag_stopMPGame) && (pNetCenter != 0) ; }
	void stopMultiPlayer() { flag_stopMPGame = true; }

	unsigned long getCurrentGameQuant() { return currentQuant; }
	unsigned long getConfirmQuant() { return confirmQuant; }

	bool loadPlayReel(const char* fname);
	bool savePlayReel(const char* fname);
	void autoSavePlayReel(void);
	void allSavePlayReel(void);
	bool flag_stopSavePlayReel;
	void stopPlayReel() { flag_stopSavePlayReel=true; }

	long getInternalLagQuant();
	unsigned long getNextQuantInterval();

	void sendListGameCommand2Host(unsigned int begQuant, unsigned int endQuant=ULONG_MAX);
	void putInputGameCommand2fullListGameCommandAndCheckAllowedRun(netCommandGame* pnc);

	void setActivePlayer(int playerID, int cooperativeIndex = 0);
	
	IntVariables& currentProfileIntVariables();
	float voiceFileDuration(const char* fileName, float duration);

	void stopNetCenter() { pNetCenter = 0; }

private:
	unsigned long libsSignature;
	PNetCenter* pNetCenter; 

	unsigned long currentQuant;
	unsigned long lagQuant;
	unsigned long dropQuant;
	unsigned long confirmQuant;
	bool flag_stopMPGame;

	unsigned int signatureGame;

	int  m_nLastPlayerReceived;

	bool netPause;
	bool isNetPause(void) { return netPause; }

	int EmptyCount;

	int TimeOffset;

	int EventTime;
	int EventDeltaTime;
	int EventWaitTime;
	int MaxEventTime;
	int RealMaxEventTime;
	int EventLagHole;

	int AverageEventTime;
	int AverageEventCount;
	int AverageEventSum;
	int AverageDampDelta;

	int StopServerTime;
	int StopServerMode;

	int MaxCorrectionTime;
	float MaxDeltaCorrectionTime;
	float MaxCorrection;

	int MinCorrectionTime;
	float MinDeltaCorrectionTime;
	float MinCorrection;

	float ServerSpeedScale;

	// Reel check
	enum ReelCheckMode {
		REEL_NO_CHECK,
		REEL_SAVE,
		REEL_VERIFY
	};
	ReelCheckMode reelCheckMode_;
	XStream reelCheckFile_;
	XBuffer reelCheckBuffer_;

	//Command History
	vector<netCommandGame*> fullListGameCommands;
	unsigned int lastQuant_inFullListGameCommands; // ванты считаютс€ с 1-го!
	unsigned int curGameComPosition;

	vector<netCommandGame*> replayListGameCommands;
	unsigned int endQuant_inReplayListGameCommands;
	vector<netCommandGame*>::iterator curRePlayPosition;

	static void clearListGameCommands(vector<netCommandGame*> & gcList);

	void stopGame_HostMigrate();

	bool flag_savePlayReel;
	bool flag_rePlayReel;
	bool flag_autoSavePlayReel;

	IntVariables currentProfileIntVariables_;

	typedef StaticMap<string, float> VoiceFileDurations;
	VoiceFileDurations voiceFileDurations_;

	//report log
	struct sLogElement{
		sLogElement(){
			pLog=0;
		}
		sLogElement(int _quant, XBuffer* _plog){
			quant=_quant;
			xassert(_plog);
			pLog=_plog;
		}
		~sLogElement(){
		}
		int quant;
		XBuffer* pLog;
	};
	list<sLogElement> logList;
	sLogElement* getLogElement(unsigned int quant){
		list<sLogElement>::iterator p;
		for(p=logList.begin(); p!=logList.end(); p++){
			if((*p).quant==quant) return &(*p);
		}
		return 0;
	}
	void pushBackLogList(int quant, XBuffer& xb){
		XBuffer* pb=new XBuffer(xb.tell()+1);
		pb->write(xb.buffer(), xb.tell());
		logList.push_back(sLogElement(quant, pb));
	}
	XBuffer* getLogInLogList(int quant){
		list<sLogElement>::iterator p;
		for(p=logList.begin(); p!=logList.end(); p++){
			if(p->quant==quant) return p->pLog;
		}
		return 0;
	}
	void clearLogList(){
		list<sLogElement>::iterator p;
		for(p=logList.begin(); p!=logList.end(); p++){
			if(p->pLog) delete p->pLog;
		}
		logList.clear();
	}
	void eraseLogListUntil(int quant){
		list<sLogElement>::iterator p;
		for(p=logList.begin(); p!=logList.end(); ){
			if(p->quant <= quant) {
				if(p->pLog) delete p->pLog;
				p=logList.erase(p);
			}
			else break;
		}
	}
	void writeLogList2File(XStream& file){
		list<sLogElement>::iterator p;
		for(p=logList.begin(); p!=logList.end(); p++){
			file.write(p->pLog->buffer(), p->pLog->tell());
		}
	}

	unsigned long clientGeneralCommandCounterInListCommand;// аналог fullListGameCommands.size()
	unsigned long lastRealizedQuant; //по идее это currentQuant
	unsigned long allowedRealizingQuant;
	unsigned long nextQuantInterval;
	float speedDelta_;

	unsigned long lastQuantAllowedTimeCommand;
	unsigned long generalCommandCounter4TimeCommand;

	InOutNetComBuffer debugCommandBuffer_;

	void logQuant();
	void sendLog(unsigned int quant);

	bool flag_HostMigrate;

	MTSection m_FullListGameCommandLock;

	static UniverseX* universeX_;
    
	friend PNetCenter;
	friend UniverseX* universeX();

	unsigned long generalCommandCounter;
};

inline UniverseX* universeX() { return UniverseX::universeX_; }

#endif //__HYPERSPACE_H__
