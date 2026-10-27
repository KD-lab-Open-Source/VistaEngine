#pragma once

class Archive;

class IniFile 
{
public:
	bool MainMenu;
	bool DisableVideo;
	float GameSpeed;
	bool HT;
	bool EnableReplay;
	bool AutoSavePlayReel;
	bool ControlFpEnable;

	bool ZIP;

	bool SynchroByClock;
	int StandartFrameRate;

	bool enableConsole;


	struct Network {
		bool ScreenLog;
		bool FileLog;
		int MaxTimePauseGame;
		int TimeOutClientOrServerReceive;
		int TimeOutDisconnect;
		bool NetworkSimulator;
		int DPSigningLevel;
		bool HostMigrate;
		bool NoUseDPNSVR;
		int DecreaseSpeedOnSlowestPlayer;
		int Port;
		int dpConnectTimeout;
		int dpConnectRetries;
		int lanConnectTimeout;
		int lanConnectRetries;
		int inetConnectTimeout;
		int inetConnectRetries;
		Network();
		void serialize(Archive& ar);
	};

	Network network;

	IniFile();
	/// Reads iniFile.cfg over the defaults. Not done by the constructor: iniFile is a
	/// global, built before main() has made the data directory the working directory.
	void load();
	/// Writes it back. WinMain does, on the way out -- not the destructor: at exit the
	/// archive code's own statics, first built inside load(), are already destroyed.
	void save();
	void serialize(Archive& ar);
};

extern IniFile iniFile;