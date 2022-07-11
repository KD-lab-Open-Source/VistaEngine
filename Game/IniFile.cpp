#include "stdafx.h"
#include "IniFile.h"
#include "Serialization\Serialization.h"
#include "Serialization\XPrmArchive.h"

IniFile iniFile;

IniFile::IniFile()
{
	MainMenu = true;
	DisableVideo = true;
	GameSpeed = 1.f;
	HT = true;
	EnableReplay = true;
	AutoSavePlayReel = false;
	ControlFpEnable = true;

	ZIP = true;

	SynchroByClock = true;
	StandartFrameRate = 25;

	enableConsole = true;

	XPrmIArchive ia;
	if(ia.open("iniFile.cfg"))
		serialize(ia);
}

IniFile::~IniFile()
{
	save();
}

void IniFile::save()
{
	serialize(XPrmOArchive("iniFile.cfg"));
}

void IniFile::serialize(Archive& ar)
{
	ar.serialize(MainMenu, "MainMenu", "MainMenu");
	ar.serialize(DisableVideo, "DisableVideo", "DisableVideo");
	ar.serialize(GameSpeed, "GameSpeed", "GameSpeed");
	ar.serialize(HT, "HT", "HT");
	ar.serialize(EnableReplay, "EnableReplay", "EnableReplay");
	ar.serialize(AutoSavePlayReel, "AutoSavePlayReel", "AutoSavePlayReel");
	ar.serialize(ControlFpEnable, "ControlFpEnable", "ControlFpEnable");

	ar.serialize(ZIP, "ZIP", "ZIP");

	ar.serialize(SynchroByClock, "SynchroByClock", "SynchroByClock");
	ar.serialize(StandartFrameRate, "StandartFrameRate", "StandartFrameRate");

	ar.serialize(enableConsole, "enableConsole", "enableConsole");

	ar.serialize(network, "network", "network");
}


IniFile::Network::Network()
{
	ScreenLog = false;
	FileLog = false;
	MaxTimePauseGame=40000;
	TimeOutClientOrServerReceive=5000;
	TimeOutDisconnect=60000;
	NetworkSimulator=0;
	DPSigningLevel=0;
	HostMigrate=1;
	NoUseDPNSVR=0;
	DecreaseSpeedOnSlowestPlayer=1;
	Port=0;
	dpConnectTimeout=200;
	dpConnectRetries=14;
	lanConnectTimeout=200;
	lanConnectRetries=7;
	inetConnectTimeout=200;
	inetConnectRetries=7;
}

void IniFile::Network::serialize(Archive& ar)
{
	ar.serialize(ScreenLog, "ScreenLog", "ScreenLog");
	ar.serialize(FileLog, "FileLog", "FileLog");
	ar.serialize(MaxTimePauseGame, "MaxTimePauseGame", "MaxTimePauseGame");
	ar.serialize(TimeOutClientOrServerReceive, "TimeOutClientOrServerReceive", "TimeOutClientOrServerReceive");
	ar.serialize(TimeOutDisconnect, "TimeOutDisconnect", "TimeOutDisconnect");
	ar.serialize(NetworkSimulator, "NetworkSimulator", "NetworkSimulator");
	ar.serialize(DPSigningLevel, "DPSigningLevel", "DPSigningLevel");
	ar.serialize(HostMigrate, "HostMigrate", "HostMigrate");
	ar.serialize(NoUseDPNSVR, "NoUseDPNSVR", "NoUseDPNSVR");
	ar.serialize(DecreaseSpeedOnSlowestPlayer, "DecreaseSpeedOnSlowestPlayer", "DecreaseSpeedOnSlowestPlayer");
	ar.serialize(Port, "Port", "Port");
	ar.serialize(dpConnectTimeout, "dpConnectTimeout", "dpConnectTimeout");
	ar.serialize(dpConnectRetries, "dpConnectRetries", "dpConnectRetries");
	ar.serialize(lanConnectTimeout, "lanConnectTimeout", "lanConnectTimeout");
	ar.serialize(lanConnectRetries, "lanConnectRetries", "lanConnectRetries");
	ar.serialize(inetConnectTimeout, "inetConnectTimeout", "inetConnectTimeout");
	ar.serialize(inetConnectRetries, "inetConnectRetries", "inetConnectRetries");
}
