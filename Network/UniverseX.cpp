#include "StdAfx.h"
#include "UniverseX.h"
#include "RenderObjects.h"
#include "Runtime.h"
#include "UnitAttribute.h"
#include "Universe.h"
#include "P2P_interface.h"
#include "GameShell.h"
#include "CameraManager.h"
#include "..\Environment\Environment.h"
#include "..\Water\Water.h"
#include "Triggers.h"

#include "Lmcons.h"
#include "terra.h"
#include "SelectManager.h"
#include "CommonEvents.h"
#include "..\UserInterface\UI_Logic.h"
#include "..\UserInterface\UI_NetCenter.h"
#include "..\Game\SoundApp.h"
#include "..\Sound\SoundSystem.h"
#include "XPrmArchive.h"

#include "LogMsg.h"

UniverseX* UniverseX::universeX_=0;

const char* autoSavePlayReelDir = "AUTOSAVE";

const char * COMMAND_LINE_SAVE_PLAY_REEL="saveplay";

const unsigned int INTERNAL_BUILD_VERSION=1003;

#ifndef _FINAL_VERSION_
class cMonowideFont {
	cFont* pfont;
public:
	cMonowideFont(){
		pfont=0;
	}
	~cMonowideFont(){
		if(pfont) pfont->Release();
	}
	cFont* getFont(){
		if(!pfont) {
			pfont=gb_VisGeneric->CreateFont("Scripts\\Resource\\fonts\\Courier New.font", 16, 1);//Тихая ошибка
			if(!pfont)
				pfont=gb_VisGeneric->CreateFont("Scripts\\Resource\\fonts\\Arial.font", 16);
		}
		return pfont;
	}
};
cMonowideFont* pMonowideFont;
#endif _FINAL_VERSION_

UniverseX::UniverseX(PNetCenter* net_client, MissionDescription& mission, XPrmIArchive* ia)
: Universe(mission, ia), reelCheckFile_(0), debugCommandBuffer_(1024, 1)
{
	universeX_ = this;

	flag_HostMigrate=false;
	flag_stopSavePlayReel=false;
	netPause=true;

	pNetCenter = net_client;
	flag_stopMPGame=false;

	m_nLastPlayerReceived = 0;

	TimeOffset = 0;

	EmptyCount = 0;

	EventTime = 0;
	EventDeltaTime = 0;
	RealMaxEventTime = MaxEventTime = 0;
	EventWaitTime = 0;

	AverageEventTime = 0;
	AverageEventCount = 0;
	AverageEventSum = 0;
	AverageDampDelta = 10;

	MaxCorrectionTime = 3;
	MaxDeltaCorrectionTime = 5.0f;
	MaxCorrection = 3.0f;

	MinCorrectionTime = 1;
	MinDeltaCorrectionTime = 5.0f;
	MinCorrection = -0.5f;

	StopServerTime = 30;
	StopServerMode = 0;

	ServerSpeedScale = 1.0f;

	currentQuant=0;
	lagQuant=0;
	dropQuant=0;
	confirmQuant=0;
	signatureGame=startCRC32;

#ifndef _FINAL_VERSION_
	pMonowideFont= new cMonowideFont();
#endif _FINAL_VERSION_

	//Очистка списков команд
	{
		//Lock!
		MTAuto lock(m_FullListGameCommandLock);
		clearListGameCommands(fullListGameCommands);
	}
	lastQuant_inFullListGameCommands=0;
	clearListGameCommands(replayListGameCommands);
	endQuant_inReplayListGameCommands=0;


	clientGeneralCommandCounterInListCommand=0;
	lastRealizedQuant=0;
	allowedRealizingQuant=0;

	speedDelta_ = 0;
	nextQuantInterval=0;
	lastQuantAllowedTimeCommand=0;
	generalCommandCounter4TimeCommand=0;
	generalCommandCounter=0;

	//calc library crc
	unsigned long tcrc;
	tcrc=TerToolsLibrary::instance().crc();
	libsSignature=crc32((unsigned char*)&tcrc, sizeof(tcrc), startCRC32);
	tcrc=AttributeLibrary::instance().crc();
	libsSignature=crc32((unsigned char*)&tcrc, sizeof(tcrc), libsSignature);
	tcrc=SourcesLibrary::instance().crc();
	libsSignature=crc32((unsigned char*)&tcrc, sizeof(tcrc), libsSignature);

	currentProfileIntVariables_ = UI_LogicDispatcher::instance().currentProfile().intVariables;

	flag_autoSavePlayReel = flag_rePlayReel=flag_savePlayReel=false;
	flag_savePlayReel = true;
	if(mission.gameType() & GAME_TYPE_REEL){
		flag_rePlayReel = true;
		loadPlayReel(mission.reelName());
	}
	if(IniManager("Game.ini").getInt("Game","AutoSavePlayReel")!=0){
		flag_autoSavePlayReel=true;
		//поиск автосэйв каталога и создание, если его нет
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind;
		hFind = FindFirstFile(autoSavePlayReelDir, &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE) {
			bool createAutoSaveDirResult=CreateDirectory(autoSavePlayReelDir, NULL);
			xassert(createAutoSaveDirResult);
		} 
		else {
			xassert((FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0);
			FindClose(hFind);
		}
	}

	currentQuant=0;

	curGameComPosition=0;

	selectManager->setPlayer(activePlayer());

	net_log_buffer.init();
	net_log_mode = false;
	reelCheckMode_ = REEL_NO_CHECK;

#ifndef _FINAL_VERSION_
	reelCheckFile_.close();
	if(check_command_line("check_replay") && mission.enablePause)
		if(!(mission.gameType() & GAME_TYPE_REEL)){
			if(reelCheckFile_.open("replay", XS_OUT)){
				reelCheckMode_ = REEL_SAVE;
				net_log_mode = true;
			}
		}
		else if(reelCheckFile_.open(setExtention(mission.reelName(), "log").c_str(), XS_IN)){
            reelCheckMode_ = REEL_VERIFY;
			net_log_mode = true;
		}
#endif

	if(net_client) // && IniManager("Network.ini").getInt("General","NetLog")
		net_log_mode = true;

	log_var(TerToolsLibrary::instance().crc());
	log_var(AttributeLibrary::instance().crc());
	log_var(SourcesLibrary::instance().crc());
	log_var(RigidBodyPrmLibrary::instance().crc());
	mission.logVar();
}

IntVariables& UniverseX::currentProfileIntVariables()
{
	return !flag_rePlayReel ? UI_LogicDispatcher::instance().currentProfile().intVariables : currentProfileIntVariables_;
}

float UniverseX::voiceFileDuration(const char* fileName, float duration)
{
	if(!flag_rePlayReel){
		voiceFileDurations_[fileName] = duration;
		return duration;
	}
	else{
		VoiceFileDurations::iterator i = voiceFileDurations_.find(fileName);
		xassert(i != voiceFileDurations_.end());
		return i != voiceFileDurations_.end() ? i->second : 0;
	}
}

bool UniverseX::loadPlayReel(const char* fname)
{
	XStream fi(fname, XS_IN);
	XBuffer buffer(fi.size());
	fi.read(buffer.buffer(), fi.size());
	
	ReplayHeader fph;
	fph.read(buffer);

	xassert(fph.valid() && "Incompatible replay (old version)"); 
	xassert(fph.universeSignature == missionSignature && "Incompatible replay (universeSignature)"); 
	xassert(fph.librarySignature == libsSignature && "Incompatible replay (librarySignature)"); 
	xassert(fph.worldSignature == vMap.currentGameMapCRC && "Incompatible replay (worldSignature)"); 

	MissionDescription temp;
	temp.loadReplayInfoInBuf(buffer);

	string name;
	int varsSize;
	buffer > varsSize;
	currentProfileIntVariables_.clear();
	for(; varsSize; varsSize--){
		int value;
		buffer > StringInWrapper(name) > value;
		currentProfileIntVariables_[name] = value;
	}

	buffer > varsSize;
	for(; varsSize; varsSize--){
		float value;
		buffer > StringInWrapper(name) > value;
		voiceFileDurations_[name] = value;
	}

	buffer.read(endQuant_inReplayListGameCommands);

	int dataSize = buffer.size() - buffer.tell();
	xassert(dataSize >= 0);
	InOutNetComBuffer in_buffer(dataSize, false);
	in_buffer.putBufferPacket((unsigned char*)buffer.buffer() + buffer.tell(), dataSize);

	while(in_buffer.currentNetCommandID()!=NETCOM_ID_NONE) {
		NCEventID event = (NCEventID)in_buffer.currentNetCommandID();
		switch(event){
		case NETCOM_4G_ID_UNIT_COMMAND: 
			{
				netCommand4G_UnitCommand*  pnc= new netCommand4G_UnitCommand(in_buffer);
				replayListGameCommands.push_back(pnc);
			}
			break;
		case NETCOM_4G_ID_UNIT_LIST_COMMAND: 
			{
				netCommand4G_UnitListCommand*  pnc= new netCommand4G_UnitListCommand(in_buffer);
				replayListGameCommands.push_back(pnc);
			}
			break;
		case NETCOM_4G_ID_PLAYER_COMMAND: 
			{
				netCommand4G_PlayerCommand*  pnc= new netCommand4G_PlayerCommand(in_buffer);
				replayListGameCommands.push_back(pnc);
			}
			break;
		case NETCOM_4G_ID_FORCED_DEFEAT:
			{
				netCommand4G_ForcedDefeat* pnc=new netCommand4G_ForcedDefeat(in_buffer);
				replayListGameCommands.push_back(pnc);
			}
			break;
		case NETCOM_4G_ID_EVENT:
			{
				netCommand4G_Event* pnc=new netCommand4G_Event(in_buffer);
				replayListGameCommands.push_back(pnc);
			}
			break;
		default:
			xassert(0&&"Incorrect commanf in playReel file!");
			break;
		}
		in_buffer.nextNetCommand();
	}
	curRePlayPosition=replayListGameCommands.begin();
	return true;
}

bool UniverseX::savePlayReel(const char* _fname)
{
#ifndef _FINAL_VERSION_
	if(reelCheckMode_ == REEL_SAVE){
		reelCheckFile_.close();
		DeleteFile(setExtention(_fname, "log").c_str());
		rename("replay", setExtention(_fname, "log").c_str());
	}
#endif

	XBuffer buffer(512, 1);

	ReplayHeader fph;
	fph.universeSignature = missionSignature;
	fph.librarySignature = libsSignature;
	fph.worldSignature = vMap.currentGameMapCRC;
	fph.endQuant = lastQuant_inFullListGameCommands;
	fph.write(buffer);

	MissionDescription mission(gameShell->CurrentMission);
	mission.setInterfaceName(_fname);
	//mission.writeNet(buffer);
	mission.saveReplay(buffer);


	buffer < currentProfileIntVariables_.size();
	IntVariables::iterator i;
	FOR_EACH(currentProfileIntVariables_, i)
		buffer < StringOutWrapper(i->first) < i->second;

	buffer < voiceFileDurations_.size();
	VoiceFileDurations::iterator vi;
	FOR_EACH(voiceFileDurations_, vi)
		buffer < StringOutWrapper(vi->first) < vi->second;

	InOutNetComBuffer out_buffer(1024,1);
	{
		//Lock!
		MTAuto lock(m_FullListGameCommandLock);
		vector<netCommandGame*>::iterator p;
		for(p=fullListGameCommands.begin(); p!=fullListGameCommands.end(); p++)
			out_buffer.putNetCommand((*p));
	}


	XStream fo(0);
	if(fo.open(setExtention(_fname, MissionDescription::getExtention(GAME_TYPE_REEL)).c_str(), XS_OUT)){
		fo.write(buffer.buffer(), buffer.tell());
		fo.write(lastQuant_inFullListGameCommands);
		fo.write(out_buffer.buffer(), out_buffer.filled_size);
	}

	if(!fo.ioError())
		return true;
	else{
		return false;
		mission.deleteSave();
	}
}

void UniverseX::autoSavePlayReel()
{
	//autosave

	SYSTEMTIME st;
	::GetLocalTime (&st);
	char fnbuf[MAX_PATH];
	sprintf(fnbuf, "%s\\autosaveFrom_%02d-%02d-%02d__%02d=%02d=%02d", autoSavePlayReelDir, (int)st.wMonth, (int)st.wDay, (int)st.wYear,
		(int)st.wHour, (int)st.wMinute, (int)st.wSecond);
	savePlayReel(fnbuf);
}

void UniverseX::allSavePlayReel()
{
	if(flag_savePlayReel){
		const char* fname=check_command_line(COMMAND_LINE_SAVE_PLAY_REEL);
		if(fname) 
			savePlayReel(fname);
	}

	if(flag_autoSavePlayReel){
		autoSavePlayReel();
	}
}

UniverseX::~UniverseX()
{
	if (selectManager)
		selectManager->clearPlayer();

	allSavePlayReel();

	//Очистка логов
	clearLogList();
	//Очистка списков команд
	{
		//Lock!
		MTAuto lock(m_FullListGameCommandLock);
		clearListGameCommands(fullListGameCommands);
	}
	lastQuant_inFullListGameCommands=0;
	clearListGameCommands(replayListGameCommands);
	endQuant_inReplayListGameCommands=0;


#ifndef _FINAL_VERSION_
	delete pMonowideFont;
#endif _FINAL_VERSION_

	xassert(this == universeX());
	universeX_ = 0;
}

bool UniverseX::PrimaryQuant()
{
	//setLogicFp();
	xassert( checkLogicFp() );
	if(!isMultiPlayer())
		return SingleQuant();
	else
		return MultiQuant();
}

void UniverseX::Quant()
{
	log_var(currentQuant);

	Universe::Quant();

	log_var(logicRnd.get());
	log_var(vMap.getGridCRC(false, currentQuant));
	log_var(vMap.getChAreasInformationCRC());
	
	logQuant();

	debugCommandBuffer_.reset();
}

long UniverseX::getInternalLagQuant(void)
{
	return allowedRealizingQuant-currentQuant;
}

unsigned long UniverseX::getNextQuantInterval()
{
	return nextQuantInterval;
}

bool UniverseX::MultiQuant()
{
	bool flag_quantExecuted=false;

	if(allowedRealizingQuant > lastRealizedQuant){
		//Начало кванта
		currentQuant++;		//currentQuant=lastQuant_inFullListGameCommands;

		lastQuant_inFullListGameCommands=currentQuant;
		//поиск первой команды в последнем кванте
		vector<netCommandGame*>::iterator p;
		p=fullListGameCommands.end();
		while(p!=fullListGameCommands.begin()) {
			p--;
			if( (*p)->curCommandQuant_ < currentQuant ){
				p++; break;
			}
		}
		// p сейчас указывает на первую команду кванта
		for(; p!=fullListGameCommands.end(); p++) {
			if((*p)->curCommandQuant_ != currentQuant ) break; //проверка на конец команд кванта
			generalCommandCounter++; //only information
			receiveCommand(**p);
		}

		lagQuant=getInternalLagQuant(); //Для визуализации

		Quant();

		lastRealizedQuant=currentQuant;
		//return true;
		flag_quantExecuted=true;
	}
	else {
		dropQuant++;
		//return false;
		flag_quantExecuted=false;
	}

	//speed regulation
	if(getInternalLagQuant() > 0){
		if(speedDelta_ < 0)
			speedDelta_ = 0;
		average(speedDelta_, 3., 0.1f);
		statistics_add(incrSpeed, 1);
	}
	else if(getInternalLagQuant() <= 0){
		if(!flag_quantExecuted){
			speedDelta_ = -min(-getInternalLagQuant(), 6)/6.f;
			statistics_add(decrSpeed, 1);
		}
		else
			speedDelta_ = 0;
	}

	if(getNextQuantInterval())
		gameShell->setSpeed(clamp((float)logicTimePeriod/(float)getNextQuantInterval() + speedDelta_, 0.2, 3));

	statistics_add(getNextQuantInterval, getNextQuantInterval());
	statistics_add(speedDelta_, speedDelta_);
	statistics_add(gameSpeed, gameShell->getSpeed());

	return flag_quantExecuted;
}

void UniverseX::receiveCommand(const netCommandGame& command)
{
	xassert(command.isGameCommand() && "Incorrect command");
	command.execute();
}

const unsigned int periodSendLogQuant=8; //степень двойки!
const unsigned int maskPeriodSendLogQuant=periodSendLogQuant-1;//

void UniverseX::logQuant()
{
	if(reelCheckMode_ == REEL_SAVE){
		reelCheckFile_.write((const char*)net_log_buffer, net_log_buffer.tell());
	}
	else if(reelCheckMode_ == REEL_VERIFY){
		if(reelCheckBuffer_.size() < net_log_buffer.tell())
			reelCheckBuffer_.alloc(net_log_buffer.tell());
		int len = reelCheckFile_.read(reelCheckBuffer_.buffer(), net_log_buffer.tell());
		if(memcmp(reelCheckBuffer_.buffer(), net_log_buffer.buffer(), len)){
			XStream f0("replay0", XS_OUT);
			XStream f1("replay1", XS_OUT);
			f0.write((const char*)reelCheckBuffer_, len);
			f1.write((const char*)net_log_buffer, len);
			xassert("Desync in replay occured" && 0);
		}
		if(len < net_log_buffer.tell()){ // switch to write mode
			reelCheckMode_ = REEL_SAVE;
			reelCheckFile_.close();
			reelCheckFile_.open("replay", XS_OUT | XS_APPEND | XS_NOREPLACE);
			reelCheckFile_.write((const char*)net_log_buffer + len, net_log_buffer.tell() - len);
		}
	}

	lagQuant=getInternalLagQuant(); //Для сервера
	if(pNetCenter && currentQuant!=0){
		lagQuant=getInternalLagQuant(); //Для сервера

		signatureGame=crc32((unsigned char*)net_log_buffer.buffer(), net_log_buffer.tell(), signatureGame);
		if((currentQuant & maskPeriodSendLogQuant)==0){ //Каждый 8 квант отсылается сигнатура
			pNetCenter->SendEvent(&netCommand4H_BackGameInformation2(lagQuant, currentQuant, signatureGame, gameShell->accessibleQuantPeriod(), false, pNetCenter->getState()));
			signatureGame=startCRC32;
		}
		pushBackLogList(currentQuant, net_log_buffer);
		//sendLog(currentQuant);
	}

	net_log_buffer.init();
}

void UniverseX::sendLog(unsigned int quant)
{
	xassert(quant);//Проверка, что не нулевой квант(кванты начинаются с 1-цы!)
	if(quant && ((quant & maskPeriodSendLogQuant)==0) ){
		unsigned int begLogQuant=((quant-1) & (~maskPeriodSendLogQuant)) +1;
		unsigned int sgn=startCRC32;
		int i;
		for(i=begLogQuant; i<=quant; i++){
			sLogElement* pLogElemente=getLogElement(i);
			xassert(pLogElemente);
			sgn=crc32((unsigned char*)pLogElemente->pLog->buffer(), pLogElemente->pLog->tell(), sgn);

		}
		pNetCenter->SendEvent(&netCommand4H_BackGameInformation2(getInternalLagQuant(), quant, sgn, gameShell->accessibleQuantPeriod(), true, pNetCenter->getState()));
	}
}

void UniverseX::sendCommand(const netCommandGame& command) 
{ 
	debugCommandBuffer_.putNetCommand(&command);
	if(debugCommandBuffer_.filled_size > 1024){
#ifndef _FINAL_VERSION_
		static XStream ff("commandBufferOverflow", XS_OUT);
		InOutNetComBuffer in_buffer(debugCommandBuffer_.tell(), false);
		in_buffer.putBufferPacket((unsigned char*)debugCommandBuffer_.buffer(), debugCommandBuffer_.tell());
		while(in_buffer.currentNetCommandID()!=NETCOM_ID_NONE) {
			NCEventID event = (NCEventID)in_buffer.currentNetCommandID();
			switch(event){
			case NETCOM_4G_ID_UNIT_COMMAND: 
				netCommand4G_UnitCommand(in_buffer).writeLog(ff);
				break;
			case NETCOM_4G_ID_UNIT_LIST_COMMAND: 
				netCommand4G_UnitListCommand(in_buffer).writeLog(ff);
				break;
			case NETCOM_4G_ID_PLAYER_COMMAND: 
				netCommand4G_PlayerCommand(in_buffer).writeLog(ff);
				break;
			case NETCOM_4G_ID_FORCED_DEFEAT:
				netCommand4G_ForcedDefeat(in_buffer).writeLog(ff);
				break;
			case NETCOM_4G_ID_EVENT:
				netCommand4G_Event(in_buffer).writeLog(ff);
				break;
			}
			in_buffer.nextNetCommand();
		}
		ff < "\n\n\n";
		xassert("Command buffer overflow" && 0);
#endif
		return;
	}

	if(!isMultiPlayer()){
		if(!flag_rePlayReel){
			MTAuto lock(m_FullListGameCommandLock);

			netCommandGame* pnc = command.clone();
			pnc->setCurCommandQuantAndCounter(currentQuant + 1, 0);
			fullListGameCommands.push_back(pnc);
			/////receiveCommand(command);
		}
	}
	else {
		pNetCenter->SendEvent(&command);
	}
}

//----------------------------- Dread Place ----------------------------
bool UniverseX::SingleQuant()
{
	currentQuant++;
	if(!flag_stopSavePlayReel) 
		lastQuant_inFullListGameCommands=currentQuant;

	if(flag_rePlayReel){
		vector<netCommandGame*>::iterator p;
		for(p=curRePlayPosition; p!=replayListGameCommands.end(); p++){
			if((*p)->curCommandQuant_==currentQuant){
				receiveCommand(**p);
			}
			else {
				curRePlayPosition=p;
				break;
			}
		}

		if(currentQuant > endQuant_inReplayListGameCommands)
            universe()->checkEvent(Event(Event::END_REPLAY));
	}
	else {
		static vector<netCommandGame*> vc(8);
		vc.clear();
		{
			MTAuto lock(m_FullListGameCommandLock);
			for(curGameComPosition; curGameComPosition<fullListGameCommands.size(); curGameComPosition++){
				if(fullListGameCommands[curGameComPosition]->curCommandQuant_==currentQuant)
					vc.push_back(fullListGameCommands[curGameComPosition]);
				else 
					break;
			}
		}
		vector<netCommandGame*>::iterator p;
		for(p=vc.begin(); p!=vc.end(); ++p)
			receiveCommand(**p);
	}

	Quant();

	allowedRealizingQuant=currentQuant+1;

	lastRealizedQuant=currentQuant;

	return true;
}

void UniverseX::drawDebug2D() const
{
	Universe::drawDebug2D();
	if(DebugPrm::instance().showNetStat){
#ifndef _FINAL_VERSION_
		if(isMultiPlayer()){
			XBuffer msg;
			msg.SetDigits(10);
			msg < "curQnt: " <= currentQuant;
			msg < " cfmQnt" <= confirmQuant;
			msg < " iLagQnt: " <= lagQuant;
			msg < " dropQnt:" <= dropQuant;

			//gb_RenderDevice->SetFont(pMonowideFont->getFont());
			//gb_RenderDevice->OutText(20, 40, msg, sColor4f(1, 1, 1, 1));

			XBuffer msg2;
			msg2.SetDigits(10);
			static int rb=0,sb=0;
			static int lastInfoQuant=0;
			static int lastInfoQuantTime=0;
			static int quantPerSec;
			int secondQuant=currentQuant/10;
			if(lastInfoQuant < secondQuant) {
				lastInfoQuant=secondQuant;
				rb=pNetCenter->in_ClientBuf.getByteReceive();
				sb=pNetCenter->out_ClientBuf.getByteSending();
				int time = xclock();
				quantPerSec=10*1000/(time-lastInfoQuantTime);
				lastInfoQuantTime=time;
			}

			int begXInfo=gb_RenderDevice->GetSizeX()/3;

			msg < " qnt/s:" <= quantPerSec;
			msg < " GS=" <= gameShell->getSpeed();
			gb_RenderDevice->SetFont(pMonowideFont->getFont());
			//gb_RenderDevice->OutText(20, 40, msg, sColor4f(1, 1, 1, 1));
			gb_RenderDevice->OutText(begXInfo, 20+40, msg, sColor4f(1, 1, 1, 1));//360

			msg2 < pNetCenter->getStrWorkMode() < " " < pNetCenter->getStrState();
			msg2 < " byteR=" <= rb < " byteS=" <=sb;
			msg2 < " GCC=" <= generalCommandCounter;
			//gb_RenderDevice->OutText(20, 60, msg2, sColor4f(1, 1, 1, 1));
			gb_RenderDevice->OutText(begXInfo, 35+40, msg2, sColor4f(1, 1, 1, 1));//360

			gb_RenderDevice->SetFont(0);
		}
#endif
	}

	char s[512];
	char* p = s;
	*p = char(0);

	static FPS fps;
	fps.quant();
	if(terShowFPS){
		float fpsmin,fpsmax;
		fps.GetFPSminmax(fpsmin,fpsmax);
		p+=sprintf(s,"%s polygons=%i\n", currentVersion,gb_RenderDevice->GetDrawNumberPolygon());
		p+=sprintf(p,"FPS=% 3.1f min=% 3.1f max=% 3.1f\n",fps.GetFPS(),fpsmin,fpsmax);

		float lpsmin,lpsmax,percent;
		gameShell->logicFPSminmax(lpsmin,lpsmax,percent);
		percent*=100;
		p+=sprintf(p,"logic=% 2.1f min=% 2.1f % 2.1f%%\n",gameShell->logicFps(),lpsmin,percent);

	}

	if(debugShowEnabled){
		if(debug_show_mouse_position){
			Vect3f v = UI_LogicDispatcher::instance().hoverPosition();
			p += sprintf(p, "mouse=(%i,%i,%i)\n", round(v.x), round(v.y), round(v.z));
		}
		
		if(environment){
			if(showDebugWaterHeight && environment->water())
				p+=sprintf(p,"water relative level =% 3.1f\n", environment->water()->getRelativeWaterLevel());

			if(showDebugSource.enable)
				p+=sprintf(p, "sources count = %i\n", showDebugSource.sourceCount);

			if(showDebugInterface.showUpMarksCount)
				p+=sprintf(p, "show text up controllers = %i\n", showDebugInterface.getShowUpMarksCount());

			if(DebugPrm::instance().showNetStat && UI_LogicDispatcher::instance().getNetCenter().isNetGame())
				p+=sprintf(p, "UI_NetStatus: %s\n", getEnumName(UI_LogicDispatcher::instance().getNetCenter().status()));
			
			if(showDebugInterface.showSelectManager){
				SelectManager::Slots slots;
				selectManager->getSelectList(slots);
				p+=sprintf(p, "current select: size=%i, %s uniform, slots count=%i, active slot=%i\n", selectManager->selectionSize(), !selectManager->uniform() ? "NOT" : "", slots.size(), selectManager->selectedSlot());
			}

			if(showDebugNumSounds)
				p+=sprintf(p,"Sounds played: %d\nSounds used: %d\n",sndSystem.numberOfPlayingSounds(),sndSystem.numberOfUsedSounds());
		}
	}

	if(*s){
		xassert(p-s<sizeof(s));
		//gb_RenderDevice->SetFont(pMonowideFont->getFont());
		gb_RenderDevice->OutText(0, 20, s, sColor4f(1, 1, 1, 1));
		//gb_RenderDevice->SetFont(0);	
	}
}

//По идее вызов корректный т.к. reJoin не пошлется пока игра не остановлена(stopGame_HostMigrate)
void UniverseX::sendListGameCommand2Host(unsigned int begQuant, unsigned int endQuant)
{
	vector<netCommandGame*>::iterator p;
	InOutNetComBuffer out_buffer(1024,1);
	if(lastQuant_inFullListGameCommands < endQuant) endQuant=lastQuant_inFullListGameCommands; /// 
	for(p=fullListGameCommands.begin(); p!=fullListGameCommands.end(); p++){
		xassert((*p)->isGameCommand());
		if( ((*p)->curCommandQuant_ >=begQuant) && ((*p)->curCommandQuant_ <=endQuant))
			out_buffer.putNetCommand((*p));
	}
	netCommand4H_ResponceLastQuantsCommands nco(begQuant, endQuant, clientGeneralCommandCounterInListCommand, out_buffer.filled_size, (unsigned char*) out_buffer.buffer() );
	if(isMultiPlayer()){
		pNetCenter->SendEvent(&nco);
	}

}


void UniverseX::stopGame_HostMigrate()
{
	///clearLastQuantListGameCommand();
	//Очистка всех не выполненных команд из списка
//	vector<netCommandGame*>::reverse_iterator p;
//	for(p=fullListGameCommands.rbegin(); p!=fullListGameCommands.rend(); ){
//		if((*p)->curCommandQuant_ > lastQuant_inFullListGameCommands) {
//			delete *p;
//			fullListGameCommands.erase(p.base());
//			p=fullListGameCommands.rbegin();
//		}
//		else break;//p++;
//	}

	xassert(allowedRealizingQuant==lastRealizedQuant);
	xassert(allowedRealizingQuant==lastQuant_inFullListGameCommands);
	vector<netCommandGame*>::iterator p;
	while(!fullListGameCommands.empty()){
		p=fullListGameCommands.end();
		p--;
		if((*p)->curCommandQuant_ > lastQuant_inFullListGameCommands){
			delete *p;
			fullListGameCommands.erase(p);
			clientGeneralCommandCounterInListCommand--;
		}
		else break;
	}

	speedDelta_ = 0;
	nextQuantInterval=0;
	lastQuantAllowedTimeCommand=0;
	generalCommandCounter4TimeCommand=0;
	allowedRealizingQuant=lastQuant_inFullListGameCommands;

	xassert(clientGeneralCommandCounterInListCommand==fullListGameCommands.size());
	//clientGeneralCommandCounterInListCommand;

	//clientGeneralCommandCounterInListCommand   ---- fullListGameCommands.push_back(pnc);
	//allowedRealizingQuant
	flag_HostMigrate=true;
}

void UniverseX::putInputGameCommand2fullListGameCommandAndCheckAllowedRun(netCommandGame* pnc)
{
	///if(pnc->curCommandQuant_ >= lastQuant_inFullListGameCommands) //В случае смены хоста - пропускать выполненные комманды
	if(pnc->curCommandQuant_ > allowedRealizingQuant) {//В случае смены хоста - пропускать выполненные комманды
		fullListGameCommands.push_back(pnc);
		xassert(pnc->curCommandCounter_==clientGeneralCommandCounterInListCommand);
		clientGeneralCommandCounterInListCommand++;
//		receiveCommand(*pnc);

		//Разрешение проигрывания комманд
		if(pnc->flag_lastCommandInQuant_){ //Комманда была последняя в кванте, поэтому разрешаем проигрывать до кванта включительно
			allowedRealizingQuant=pnc->curCommandQuant_;
		}
		else { //Комманда была не последняя в кванте, поэтому разрешаем проигрывать до предыдущего кванта
			if(pnc->curCommandQuant_>0){
				allowedRealizingQuant=pnc->curCommandQuant_-1;
			}
		} 
		//Если количество комманд совпадает со счетчиком команд в последней NEXT_COMMAND(TimeCommand)
		// и при этом более позднее чем разрешенное сейчас  то разрешается проигрывание до кванта указанного в последней TimeComand-е
		if( (clientGeneralCommandCounterInListCommand==generalCommandCounter4TimeCommand) &&
			(lastQuantAllowedTimeCommand > allowedRealizingQuant) ){
			allowedRealizingQuant=lastQuantAllowedTimeCommand;
		}
	}
	else {
		//xassert(0&&"host migrate ?");
		LogMsg("host migrate ?");
		//можно вставить поиск в списке и подтверждение
		vector<netCommandGame*>::iterator p;
		for(p=fullListGameCommands.begin(); p!=fullListGameCommands.end(); p++){
			if((*p)->curCommandCounter_==pnc->curCommandCounter_) break;
		}
		if(p!=fullListGameCommands.end()){
			if((*p)->isGameCommand() && (*p)->compare(*pnc)){
				//good
			}
			else {
				xassert(0&&"Command not compare!(by HostMigrate!)");
			}
		}
		else{
			xassert(0&&"Command not found!(by HostMigrate!)");
		}
	}
}


//#define NETCOM_DBG_LOG
#ifdef NETCOM_DBG_LOG
XStream netCommandLog("netcommandlog.log", XS_OUT);
#endif

bool UniverseX::ReceiveEvent(NCEventID event, InOutNetComBuffer& in_buffer)
{
	switch(event) {
	case NETCOM_4C_ID_SEND_LOG_2_HOST:
		{
			xassert(flag_HostMigrate);
			netCommand4C_sendLog2Host nc(in_buffer);
			LogMsg("Client netCommand4C_sendLog2Host request log from %u quant\n", nc.begQuant);
			LogMsg("Client lastQuant_inFullListGameCommands - %u\n", lastQuant_inFullListGameCommands);
			unsigned int i;
			for(i=nc.begQuant; i<=lastQuant_inFullListGameCommands; i++){
				sendLog(i);
			}
			flag_HostMigrate=false;

		}
		break;
	case NETCOM_ID_NEXT_QUANT:
		{
			xassert(!flag_HostMigrate);
			netCommandNextQuant nc(in_buffer);
			if(nc.numberQuant_ > lastQuantAllowedTimeCommand){
				lastQuantAllowedTimeCommand=nc.numberQuant_;
				generalCommandCounter4TimeCommand=nc.globalCommandCounter_;
				nextQuantInterval=nc.quantInterval_;
			}
			if(clientGeneralCommandCounterInListCommand==generalCommandCounter4TimeCommand){
				allowedRealizingQuant=lastQuantAllowedTimeCommand;
			}
			//Не работает ! 
			/*if(nc.numberQuant_ <= lastQuant_inFullListGameCommands) {
				XBuffer* pCurLogQuant=getLogInLogList(nc.numberQuant_); //В случае смены хоста - пропускать выполненные комманды но отсылать backGameInformation
				if(pCurLogQuant){
					unsigned int signature=crc32((unsigned char*)pCurLogQuant->address(), pCurLogQuant->tell(), startCRC32);
					pNetCenter->SendEvent(&netCommand4H_BackGameInformation2(0, nc.numberQuant_, signature));
				}
				else xassert(0&&"No log(after migrate host)!");
				break;
			}*/
			if(nc.quantConfirmation_!=netCommandNextQuant::NOT_QUANT_CONFIRMATION){ //Сейчас quantConfirmation_ посылается во всех коммандах
				confirmQuant=nc.quantConfirmation_;
				//clear list 
				eraseLogListUntil(nc.quantConfirmation_);
			}
#ifdef NETCOM_DBG_LOG
			netCommandLog < "Quant=" <=nc.numberQuant_ <"\n";
#endif
			///xassert(nc.numberQuant_==(lastQuant_inFullListGameCommands+1));//Проверка на пропуск кванта(в случае выпадения!? пакета) PS. сейчас они выпадают!

			netPause=nc.flag_pause_;
			//LogMsg("Quant-%u\n", allowedRealizingQuant);
		}
		break;


	case NETCOM_4G_ID_UNIT_COMMAND: 
		{
			xassert(!flag_HostMigrate);
			netCommand4G_UnitCommand*  pnc= new netCommand4G_UnitCommand(in_buffer);
			xassert(pnc->curCommandQuant_ < 0xcdcd0000);
			putInputGameCommand2fullListGameCommandAndCheckAllowedRun(pnc);
		}
		break;
	case NETCOM_4G_ID_UNIT_LIST_COMMAND: 
		{
			xassert(!flag_HostMigrate);
			netCommand4G_UnitListCommand*  pnc= new netCommand4G_UnitListCommand(in_buffer);
			xassert(pnc->curCommandQuant_ < 0xcdcd0000);
			putInputGameCommand2fullListGameCommandAndCheckAllowedRun(pnc);
		}
		break;
	case NETCOM_4G_ID_PLAYER_COMMAND:
		{
			xassert(!flag_HostMigrate);
			netCommand4G_PlayerCommand*  pnc= new netCommand4G_PlayerCommand(in_buffer);
			xassert(pnc->curCommandQuant_ < 0xcdcd0000);
			putInputGameCommand2fullListGameCommandAndCheckAllowedRun(pnc);
		}
		break;
	case NETCOM_4G_ID_FORCED_DEFEAT:
		{
			xassert(!flag_HostMigrate);
			netCommand4G_ForcedDefeat*  pnc= new netCommand4G_ForcedDefeat(in_buffer);
			xassert(pnc->curCommandQuant_ < 0xcdcd0000);
			putInputGameCommand2fullListGameCommandAndCheckAllowedRun(pnc);
		}
		break;
	case NETCOM_4G_ID_EVENT:
		{
			xassert(!flag_HostMigrate);
			netCommand4G_Event* pnc=new netCommand4G_Event(in_buffer);
			xassert(pnc->curCommandQuant_ < 0xcdcd0000);
			putInputGameCommand2fullListGameCommandAndCheckAllowedRun(pnc);
		}
		break;
	case NETCOM_4C_ID_SAVE_LOG:
		{
			netCommand4C_SaveLog nc(in_buffer);

			const int BUF_CN_SIZE=MAX_COMPUTERNAME_LENGTH + 1;
			DWORD cns = BUF_CN_SIZE;
			char cname[BUF_CN_SIZE];
			GetComputerName(cname, &cns);

			const int BUF_UN_SIZE=UNLEN + 1;
			DWORD uns = BUF_UN_SIZE;
			char username[BUF_UN_SIZE];
			GetUserName(username, &uns);

			XBuffer tb;
			tb < "outNet_" < cname < "_" < username <= pNetCenter->m_localUNID.dpnid() < ".log";

			XStream f(tb, XS_OUT);
			//const char* currentVersion;
			f < currentVersion < "\r\n";
			writeLogList2File(f);
			f.close();
			LogMsg("receive netCommand4C_SaveLog q=%u\n" , nc.quant);
			//::MessageBox(0, "Unique!!!; outnet.log saved", "Error network synchronization", MB_OK|MB_ICONERROR);
			pNetCenter->ExecuteInterfaceCommand_thA(NetGEC_GameDesynchronized);
		}
		break;

	case EVENT_ID_SERVER_TIME_CONTROL: {
		terEventControlServerTime event(in_buffer);
		SetServerSpeedScale(event.scale);
		break;
		}

	default:
		//return false;
		xassert("Invalid netCommand to client");
		in_buffer.ignoreNetCommand();
		break;
	}
	return true;

}

void fCommandSelectUnit(XBuffer& stream)
{
	UnitInterface* unit;
	stream.read(unit);
	bool shift_flag, deselect_flag;
	stream.read(shift_flag);
	stream.read(deselect_flag);
	universeX()->select(unit, shift_flag, deselect_flag);
}

void UniverseX::select(UnitInterface* unit, bool shiftPressed, bool no_deselect)
{
	if(MT_IS_GRAPH())
		selectManager->selectUnit(unit, shiftPressed, no_deselect);
	else
		streamLogicCommand.set(fCommandSelectUnit) << (void*)unit << shiftPressed << no_deselect;
}

void UniverseX::deselect(UnitInterface* unit)
{
	selectManager->deselectUnit(unit);
}

void UniverseX::changeUnitNotify(UnitInterface* oldUnit, UnitInterface* newUnit)
{
	if(oldUnit->player() == activePlayer())
		selectManager->changeUnit(oldUnit, newUnit);
}

void UniverseX::clearListGameCommands(vector<netCommandGame*> & gcList)
{
	vector<netCommandGame*>::iterator p;
	for(p=gcList.begin(); p!=gcList.end(); p++){
		delete *p;
	}
	gcList.clear();
}

void UniverseX::setActivePlayer(int playerID, int cooperativeIndex)
{
	Universe::setActivePlayer(playerID);
	selectManager->setPlayer(activePlayer());
	UI_LogicDispatcher::instance().SetSelectPeram(universe()->activePlayer()->race()->selection_param);
}
