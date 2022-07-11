#ifndef __CHEAT_MANAGER_H__
#define __CHEAT_MANAGER_H__

#include "Timers.h"

class CheatManager
{
public:
	void keyPressed(const struct sKey& Key);
	void addString(const char* str) { map_[str] = 1; }

private:
	typedef StaticMap<string, int> Map;
	Map map_;
	string input_;
	DurationNonStopTimer timer_;
};

extern CheatManager cheatManager;

#endif //__CHEAT_MANAGER_H__