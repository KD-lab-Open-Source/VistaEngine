#include "stdafx.h"
#include "CheatManager.h"
#include "GlobalAttributes.h"
#include "Universe.h"
#include "GameCommands.h"

CheatManager cheatManager;

void CheatManager::keyPressed(const sKey& key)
{
	if(!timer_.busy())
		input_.clear();

	input_.insert(input_.end(), tolower(key.key));
	timer_.start(60000/GlobalAttributes::instance().cheatTypeSpeed);
	if(input_.size() > 4 && map_.exists(input_)){
		universe()->sendCommand(netCommand4G_Event(*EventHandle(new EventStringPlayer(input_.c_str(), universe()->activePlayer()->playerID()))));
		input_.clear();
	}
}


