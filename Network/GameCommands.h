#ifndef __GAME_COMMANDS_H__
#define __GAME_COMMANDS_H__

#include "NetCommandBase.h"
#include "UnitCommand.h"
#include "Units\UnitAttribute.h"

class Event;
typedef PolymorphicHandle<const Event> EventHandle;

class UnitInterface;
typedef vector<UnitInterface*> UnitInterfaceList; 
typedef UnitLink<UnitInterface> UnitInterfaceLink;
typedef vector<UnitInterfaceLink> UnitInterfaceLinks;

class netCommandGame : public NetCommandBase{
public:
	netCommandGame(NCEventID event_id);
	void baseRead(XBuffer& in);
	void baseWrite(XBuffer& out) const;

	void setCurCommandQuantAndCounter(unsigned int ccq, unsigned int ccc){ curCommandQuant_=ccq; curCommandCounter_=ccc; }
	void setFlagLastCommandInQuant(){ flag_lastCommandInQuant_=true; }

	bool isGameCommand() const { return true; }

	virtual void execute() const = 0;
	virtual void writeLog(XStream& ff){}
	virtual bool compare(const netCommandGame& op)const;
	virtual netCommandGame* clone()const =0;

	unsigned int curCommandQuant_;
	unsigned int curCommandCounter_;
	bool flag_lastCommandInQuant_;
};

template <class cloneClass, NCEventID EVENT_ID>
class netCommandGameClonator : public netCommandGame{
public:
	netCommandGameClonator() : netCommandGame(EVENT_ID) {}
	netCommandGame* clone()const{
		cloneClass* pc=new cloneClass(static_cast<const cloneClass&>(*this));
		return pc;
	}
};

class netCommand4G_ForcedDefeat : 
	public netCommandGameClonator<netCommand4G_ForcedDefeat, NETCOM4G_ForcedDefeat>
{
public:
	netCommand4G_ForcedDefeat(const int _userID) : userID(_userID) { }
	netCommand4G_ForcedDefeat(XBuffer& in){ baseRead(in); in.read(&userID, sizeof(userID)); }
	void Write(XBuffer& out) const{ baseWrite(out); out.write(&userID, sizeof(userID)); }
	void execute() const;
	void writeLog(XStream& ff);
	bool compare(const netCommandGame& sop)const;

	int userID;
};

class netCommand4G_UnitCommand : 
	public netCommandGameClonator<netCommand4G_UnitCommand, NETCOM4G_UnitCommand>
{
public:
	netCommand4G_UnitCommand(int ownerID, const UnitCommand& unitCommand);
	netCommand4G_UnitCommand(XBuffer& in);
	void Write(XBuffer& out) const;
	void execute() const;
	void writeLog(XStream& ff);
	bool compare(const netCommandGame& sop)const;
	
	int ownerID() const { return ownerID_; }
	const UnitCommand& unitCommand() const { return unitCommand_; }

private:
	int ownerID_;
	UnitCommand unitCommand_;
};

class netCommand4G_UnitListCommand : 
	public netCommandGameClonator<netCommand4G_UnitListCommand, NETCOM4G_UnitListCommand>
{
public:
	netCommand4G_UnitListCommand(const UnitInterfaceList& unitList, const UnitCommand& unitCommand, bool usePositionGenerator = false);
	netCommand4G_UnitListCommand(XBuffer& in);
	void Write(XBuffer& out) const;
	void execute() const;
	void writeLog(XStream& ff);
	bool compare(const netCommandGame& sop)const;
	
	const UnitCommand& unitCommand() const { return unitCommand_; }

private:
	typedef vector<int> UnitList;
	UnitList unitList_;
	UnitCommand unitCommand_;
	bool usePositionGenerator_;
};

class netCommand4G_PlayerCommand : 
	public netCommandGameClonator<netCommand4G_PlayerCommand, NETCOM4G_PlayerCommand>
{
public:
	netCommand4G_PlayerCommand(unsigned int playerID, const UnitCommand& unitCommand);
	netCommand4G_PlayerCommand(XBuffer& in);
	void Write(XBuffer& out) const;
	void execute() const;
	void writeLog(XStream& ff);
	bool compare(const netCommandGame& sop)const;

	const unsigned int ownerPlayerID() const { return ownerPlayerID_; }
	const UnitCommand& unitCommand() const { return unitCommand_; }

private:
	unsigned int ownerPlayerID_;
	UnitCommand unitCommand_;
};


class netCommand4G_Event : 
	public netCommandGameClonator<netCommand4G_Event, NETCOM4G_Event>
{
public:
	netCommand4G_Event(const Event& event);
	netCommand4G_Event(XBuffer& in);
	void Write(XBuffer& out) const;
	void execute() const;
	void writeLog(XStream& ff);
	bool compare(const netCommandGame& sop)const;
	const Event& event() const { return *event_; }

private:
	EventHandle event_;
};

#endif //__GAME_COMMANDS_H__