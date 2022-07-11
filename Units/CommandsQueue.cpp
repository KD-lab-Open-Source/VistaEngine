#include "StdAfx.h"
#include "CommandsQueue.h"
#include "BaseUniverseObject.h"
#include "CameraManager.h"
#include "Squad.h"
#include "UserInterface/UserInterface.h"

WRAP_LIBRARY(CommandColorManager, "CommandColorManager", "Цвета команд", "Scripts\\Content\\CommandColorManager", 0, 0);

LogicTimer CommandsQueueProcessor::interruptTimer_;

UnitCommandExtended::UnitCommandExtended(CommandID commandID, int actorID) 
{
	commandID_ = commandID;
	actorID_ = actorID;

	smoothTransition = false;
	cycles = 1;
}

void UnitCommandExtended::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(actorID_, "actorID", 0);

	ar.serialize(anchor_, "anchor", "Якорь");

	if(ar.openBlock("camera", "Камера")){
		ar.serialize(cameraSplineName, "cameraSplineName", "&Имя сплайна камеры");
		ar.serialize(cycles, "cycles", "Количество циклов");
		ar.serialize(smoothTransition, "smoothTransition", "Плавный переход");
		ar.closeBlock();
	}

	if(commandID() == COMMAND_ID_TALK)
		ar.serialize(messageSetup_, "messageSetup", "Сообщение");
}

void UnitCommandExtended::setCamera() const
{
	if(CameraSpline* spline = cameraManager->findSpline(cameraSplineName.c_str())){
		cameraManager->setSkipCutScene(false);
		cameraManager->erasePath();
		cameraManager->loadPath(*spline, smoothTransition);
		cameraManager->startReplayPath(spline->stepDuration(), cycles);
	}
}


void CommandsQueue::serialize(Archive& ar) 
{
	StringTableBase::serialize(ar); 
	ar.serialize(actors, "actors", "Актеры");
	ar.serialize(static_cast<vector<UnitCommandExtended>&>(*this), "queue", "Команды");
}

/////////////////////////////////////////////////////////////////////

CommandsQueueProcessor::CommandsQueueProcessor(const CommandsQueue& commands, const LegionariesLinks& actors)
: commands_(commands)
{
	actors_ = actors;
	index_ = 0;
	command_ = 0;

	interruptTimer_.stop();
}

bool CommandsQueueProcessor::quant()
{
	if(interruptTimer_.busy()){
		LegionariesLinks ::iterator i;
		FOR_EACH(actors_, i)
			if(*i){
				(*i)->clearOrders();
				(*i)->finishState(StateTrigger::instance());
			}
		return false;
	}

	if(!command_){
		if(index_ == commands_.size())
			return false;

		command_ = &commands_[index_++];

		moving_ = false;
		cameraSet_ = false;
		executingCommand_ = false;

		if(command_->actorID() != -1){
			actor_ = actors_[command_->actorID()];
			if(!actor_)
				return false;
			actor_->squad()->setUsedByTrigger(1, this);

			if(strlen(command_->anchor().c_str())){
				actor_->executeCommand(UnitCommand(COMMAND_ID_POINT, command_->anchor()->position(), 0));
				moving_ = true;
				return true;
			}
		}
	}

	if(moving_){
		if(!actor_->wayPointsEmpty())
			return true;
		if(!actor_->rotate(command_->anchor()->angleZ()))
			return true;
		moving_ = false;
	}

	if(!cameraSet_){
		cameraSet_ = true;
		command_->setCamera();
	}

	if(!executingCommand_){
		executingCommand_ = true;
		if(command_->commandID() == COMMAND_ID_TALK){
			if(actor_){
				actor_->startState(StateTrigger::instance());
				actor_->setChain(CHAIN_TRIGGER, command_->commandData());
			}
			UI_Dispatcher::instance().playVoice(command_->messageSetup());
			UI_Dispatcher::instance().sendMessage(command_->messageSetup());
			animationTimer_.start(round(command_->messageSetup().displayTime()*1000.f));
		}	
		else if(actor_)
			actor_->executeCommand(*command_);
	}
	else{
		if(command_->commandID() == COMMAND_ID_TALK){
			if(animationTimer_.busy())
				return true;
			if(actor_){
				actor_->finishState(StateTrigger::instance());
				actor_->squad()->setUsedByTrigger(0, this);
			}
			actor_ = 0;
			command_ = 0;
		}
		else{
			actor_ = 0; // !!!
			command_ = 0;
		}
	}
	return true;
}

void CommandsQueueProcessor::interrupt()
{
	interruptTimer_.start(1000);
}

/////////////////////////////////////////////////////////////////////

void CommandColorManager::serialize(Archive& ar)
{
	const EnumDescriptor& desc = getEnumDescriptor(CommandID(0));
	ComboStrings::const_iterator it;
	FOR_EACH(desc.comboStrings(), it){
		const char* name = it->c_str();
		int key = desc.keyByName(name);
		const char* nameAlt = desc.nameAlt(key);
		ar.serialize(colors_[key], name, nameAlt);
	}
}

const Color3c& CommandColorManager::getColor(CommandID key)
{
	return colors_[key];
}

