#include "StdAfx.h"
#include "EventListeners.h"
#include "MainFrame.h"

EventMaster& eventMaster()
{
	return mainFrame();
}
