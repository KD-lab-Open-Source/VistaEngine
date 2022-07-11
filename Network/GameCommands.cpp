#include "stdafx.h"
#include "GameCommands.h"
#include "Triggers.h"
#include "UnitInterface.h"
#include "Universe.h"
#include "Squad.h"

//-----------------------------------------------------

netCommandGame::netCommandGame(NCEventID event_id) : NetCommandBase(event_id)
{
	curCommandQuant_=-1;
	curCommandCounter_=-1;
	flag_lastCommandInQuant_=0;
}

bool netCommandGame::compare(const netCommandGame& op)const
{
	if(EventID!=op.EventID) 
		return false;
	else 
		return true;
}

void netCommandGame::baseRead(XBuffer& in) 
{
	in.read(&curCommandQuant_, sizeof(curCommandQuant_));
	in.read(&curCommandCounter_, sizeof(curCommandCounter_));
	in.read(&flag_lastCommandInQuant_, sizeof(flag_lastCommandInQuant_));
}

void netCommandGame::baseWrite(XBuffer& out) const
{
	out.write(&curCommandQuant_, sizeof(curCommandQuant_));
	out.write(&curCommandCounter_, sizeof(curCommandCounter_));
	out.write(&flag_lastCommandInQuant_, sizeof(flag_lastCommandInQuant_));
}

//-----------------------------------------------------

netCommand4G_Event::netCommand4G_Event(const Event& event) 
{
	xassert(event.numRef() && "Для отправки по сети события должны создаваться по new и передаваться через временный EventHandle");
	event_ = &event;
}

netCommand4G_Event::netCommand4G_Event(XBuffer& in) 
{
	baseRead(in);
	event_ = Event::create(in);
}

void netCommand4G_Event::Write(XBuffer& out) const
{
	baseWrite(out);
	event_->writeNet(out);
}

void netCommand4G_Event::execute() const
{	
	universe()->checkEvent(event());
	log_var(event().type());
}

bool netCommand4G_Event::compare(const netCommandGame& sop) const
{
	if(__super::compare(sop))
		return true;
	else
		return false;
}

//-----------------------------------------------------
void  netCommand4G_ForcedDefeat::execute() const
{
	universe()->forcedDefeat(userID);
	log_var(userID);
}

bool netCommand4G_ForcedDefeat::compare(const netCommandGame& sop) const
{
	if(__super::compare(sop))
		return userID==static_cast<const netCommand4G_ForcedDefeat&>(sop).userID;
	else return false;
}

//-----------------------------------------------------

netCommand4G_UnitCommand::netCommand4G_UnitCommand(int ownerID, const UnitCommand& unitCommand) :  ownerID_(ownerID), unitCommand_(unitCommand)
{
}

netCommand4G_UnitCommand::netCommand4G_UnitCommand(XBuffer& in) 
{
	baseRead(in); in > ownerID_; unitCommand_.read(in);
}

void netCommand4G_UnitCommand::Write(XBuffer& out) const
{
	baseWrite(out); out < ownerID_; unitCommand_.write(out);
}

void netCommand4G_UnitCommand::execute() const 
{
	UnitInterface* unit = safe_cast<UnitInterface*>(UnitID::get(ownerID()));
	if(unit)
		unit->receiveCommand(unitCommand());

	log_var(unitCommand().commandID());
	log_var(unitCommand().position());
}

bool netCommand4G_UnitCommand::compare(const netCommandGame& sop) const
{
	if(__super::compare(sop)){
		const netCommand4G_UnitCommand& uc=static_cast<const netCommand4G_UnitCommand&>(sop);
		return (ownerID_==uc.ownerID_ && unitCommand_==uc.unitCommand_);
	}
	else
		return false;
}

//-----------------------------------------------------

netCommand4G_PlayerCommand::netCommand4G_PlayerCommand(unsigned int playerID, const UnitCommand& unitCommand) : unitCommand_(unitCommand), ownerPlayerID_(playerID)
{
}

netCommand4G_PlayerCommand::netCommand4G_PlayerCommand(XBuffer& in) 
{
	baseRead(in); 
	in.read(&ownerPlayerID_, sizeof(ownerPlayerID_));
	unitCommand_.read(in);
}

void netCommand4G_PlayerCommand::Write(XBuffer& out) const
{
	baseWrite(out); 
	out.write(&ownerPlayerID_, sizeof(ownerPlayerID_));
	unitCommand_.write(out);
}

void netCommand4G_PlayerCommand::execute() const
{
	Player* player = universe()->findPlayer(ownerPlayerID());
	xassert(player);
	player->executeCommand(unitCommand());
}

bool netCommand4G_PlayerCommand::compare(const netCommandGame& sop) const
{
	if(__super::compare(sop)){
		const netCommand4G_PlayerCommand& pc=static_cast<const netCommand4G_PlayerCommand&>(sop);
		return (ownerPlayerID_==pc.ownerPlayerID_ && unitCommand_==pc.unitCommand_);
	}
	else
		return false;
}

//-----------------------------------------------------

netCommand4G_UnitListCommand::netCommand4G_UnitListCommand(const UnitInterfaceList& unitList, const UnitCommand& unitCommand) 
:  unitCommand_(unitCommand)
{
	unitList_.reserve(unitList.size());
	UnitInterfaceList::const_iterator i;
	FOR_EACH(unitList, i)
		unitList_.push_back((*i)->unitID().index());
}

netCommand4G_UnitListCommand::netCommand4G_UnitListCommand(XBuffer& in) 
{
	baseRead(in); 
	unsigned char size;
	in > size; 
	unitList_.reserve(size);
	while(size--){
		int unitID;
		in > unitID;
		unitList_.push_back(unitID);
	}
	unitCommand_.read(in);
}

void netCommand4G_UnitListCommand::Write(XBuffer& out) const
{
	baseWrite(out); 
	out < (unsigned char)unitList_.size(); 
	UnitList::const_iterator i;
	FOR_EACH(unitList_, i)
		out < *i;
	unitCommand_.write(out);
}

void netCommand4G_UnitListCommand::execute() const
{
	UnitInterfaceLinks unitList;
	unitList.reserve(unitList_.size());
	UnitList::const_iterator i;
	FOR_EACH(unitList_, i){
		UnitInterface* unit = safe_cast<UnitInterface*>(UnitID::get(*i));
		if(unit)
			unitList.push_back(unit);
	}

	if(unitCommand().commandID() != COMMAND_ID_ADD_SQUAD){
		UnitInterfaceLinks::iterator ui;
		FOR_EACH(unitList, ui)
			(*ui)->receiveCommand(unitCommand());
	}
	else{
		UnitInterfaceLinks::iterator i;
		FOR_EACH(unitList, i)
			if(*i && (*i)->attr().isSquad()){
				for(UnitInterfaceLinks::iterator j = i + 1; j != unitList.end(); ++j)
					if(*j && (*j)->attr().isSquad())
						safe_cast<UnitSquad*>(&**i)->addSquad(safe_cast<UnitSquad*>(&**j));
			}
	}

	log_var(unitCommand().commandID());
	log_var(unitCommand().position());
}

bool netCommand4G_UnitListCommand::compare(const netCommandGame& sop) const
{
	if(!__super::compare(sop))
		return false;
		
	const netCommand4G_UnitListCommand& uc = static_cast<const netCommand4G_UnitListCommand&>(sop);
	return unitList_.size() == uc.unitList_.size() && unitCommand_==uc.unitCommand_;
}


void netCommand4G_ForcedDefeat::writeLog(XStream& ff)
{
	ff < "ForceDefeat\n";
}

void netCommand4G_UnitCommand::writeLog(XStream& ff)
{
	ff < "UnitCommand: " <= unitCommand_.commandID() < "\n";
}

void netCommand4G_UnitListCommand::writeLog(XStream& ff)
{
	ff < "UnitListCommand: " <= unitCommand_.commandID() < "\t" <= unitList_.size() < "\n";
}

void netCommand4G_PlayerCommand::writeLog(XStream& ff)
{
	ff < "PlayerCommand: " <= unitCommand_.commandID() < "\n";
}

void netCommand4G_Event::writeLog(XStream& ff)
{
	ff < "Event: " <= event_->type() < "\n";
}
