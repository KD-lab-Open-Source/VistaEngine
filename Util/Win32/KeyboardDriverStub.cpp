// KeyboardDriver stub for the cross-platform build.
//
// The real KeyboardDriver translates Win32 WM_KEY* messages (virtual keys, dead
// keys) into engine key events. SDL3 will feed keyboard input cross-platform
// (Track B); off-Windows this is inert.
#include <string>
using namespace std;

#include "KeyboardDriver.h"

KeyboardDriver::KeyboardDriver(IEventHandler* handler)
	: handler_(handler)
{
}

KeyboardDriver::~KeyboardDriver() {}

bool KeyboardDriver::handleKeyMessage(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return false;
}
