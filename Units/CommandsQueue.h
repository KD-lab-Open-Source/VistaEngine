#ifndef __COMMANDS_QUEUE_H__
#define __COMMANDS_QUEUE_H__

#include "UnitCommand.h"
#include "UserInterface\UI_Types.h"
#include "Units\LabelObject.h"
#include "Units\IronLegion.h"
#include "Game\Player.h"

class UnitCommandExtended : public UnitCommand
{
public:
	UnitCommandExtended(CommandID commandID = COMMAND_ID_NONE, int actorID = 0);

	void serialize(Archive& ar);
	void refresh();
	
	const UI_MessageSetup& messageSetup() const { return messageSetup_; }
	
	Vect2f coordinates(int index) const;

	const LabelObject& anchor() const { return anchor_; }

	int actorID() const { return actorID_; }

	void setCamera() const;

private:
	int actorID_;
	LabelObject anchor_;
	UI_MessageSetup messageSetup_;

	CameraSplineName cameraSplineName;
	bool smoothTransition; 
	int cycles; 
};

struct CommandsQueue : StringTableBase, vector<UnitCommandExtended>
{
	CommandsQueue(const char* name = "") : StringTableBase(name) {}
	void serialize(Archive& ar);
	
	typedef AttributeUnitActingReferences Actors;
	Actors actors;
};

class CommandColorManager : public LibraryWrapper<CommandColorManager>
{
public:
	CommandColorManager(); 
	void serialize(Archive& ar);
	const Color3c& getColor(CommandID key); 

private:
	typedef vector<Color3c> CommandColors;
	CommandColors colors_;
};

typedef StringTable<CommandsQueue> CommandsQueueLibrary;
typedef StringTableReference<CommandsQueue, true>  CommandsQueueReference;

class CommandsQueueProcessor : public ShareHandleBase
{
public:
	CommandsQueueProcessor(const CommandsQueue& commands, const LegionariesLinks& actors);
	bool quant();
	
	static void interrupt();

private:
	const CommandsQueue& commands_;
	LegionariesLinks actors_;
	
	int index_;
	const UnitCommandExtended* command_; 
	UnitLink<UnitLegionary> actor_;
	bool moving_;
	bool cameraSet_;
	bool executingCommand_;
	LogicTimer animationTimer_;

	static LogicTimer interruptTimer_;
};



#endif //__COMMANDS_QUEUE_H__
