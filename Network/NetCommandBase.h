#ifndef __NETCOMMANDBASE_H__
#define __NETCOMMANDBASE_H__

enum NCEventID
{
	NETCOM_None=0,


	NETCOM4C_StartLoadGame,

	NETCOM4G_NextQuant,
	NETCOM4G_UnitCommand,
	NETCOM4G_UnitListCommand,
	NETCOM4G_PlayerCommand,
	NETCOM4G_Event,
	NETCOM4G_ForcedDefeat,

	NETCOM4G_ChatMessage,

	NETCOM4H_BackGameInformation,
	NETCOM4C_DisplayDistrincAreas,
	NETCOM4C_SaveLog,
	NETCOM4C_SendLog2Host,

	NETCOM4H_BackGameInformation2,


	NETCOM4H_RequestPause,
	NETCOM4C_Pause,

	////---
	NETCOM4H_JoinRequest,
	NETCOM4C_JoinResponse,

	NETCOM4H_RejoinRequest,
	NETCOM4C_RequestLastQuantCommands,
	NETCOM4H_ResponceLastQuantsCommands,

	NETCOM4H_GameIsLoaded,
	NETCOM4C_ContinueGameAfterHostMigrate,

	NETCOM4C_CurMissionDescriptionInfo,
	NETCOM4H_ChangePlayerRace,
	NETCOM4H_ChangePlayerColor,
	NETCOM4H_ChangePlayerSign,
	NETCOM4H_ChangeRealPlayerType,
	NETCOM4H_ChangePlayerDifficulty,
	NETCOM4H_ChangePlayerClan,
	NETCOM4H_ChangeMissionDescription,
	NETCOM4H_Join2Command,
	NETCOM4H_KickInCommand,
	NETCOM4C_DiscardUser,

	NETCOM4H_PlayerIsReadyOrStartLoadGame,

	NETCOM4H_CreateGame,

	NETCOM4C_AlifePacket,
	NETCOM4H_AlifePacket,

	NETCOM4C_ClientIsNotResponce,

	NETCOM4C_ServerTimeControl,
};

//-------------------------------

struct NetCommandBase
{
	NCEventID EventID;

	NetCommandBase(NCEventID event_id) { EventID = event_id; }
	virtual ~NetCommandBase() {}

	virtual void Write(XBuffer& out) const {}

	virtual bool isGameCommand() const { return false; }
};

#endif //__NETCOMMANDBASE_H__