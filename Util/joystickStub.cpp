// Portable joystick stub for the cross-platform build.
//
// The real joystick.cpp is DirectInput8; SDL3 will provide cross-platform
// joystick input later (Track B). These no-ops let the game link with the
// joystick disabled (InitDirectInput fails, so no input is polled).
#include <string>
#include <vector>
using namespace std;

#include "joystick.h"

JoystickState joystickState;

JoystickState::JoystickState()
{
	for(int i = 0; i < JOY_CONTROL_ID_MAX; ++i)
		controlState_[i] = 0;
}

bool JoystickState::isControlPressed(JoystickControlID) const { return false; }
bool JoystickState::isControlPressed(const JoystickControlSetup&) const { return false; }

// JoystickSetup derives from the editor LibraryWrapper/EditorLibraryInterface
// hierarchy; constructing one off-Windows would drag in that whole stack. The
// joystick is disabled here, so hand back inert storage rather than a live
// instance (the library is never actually edited/serialized in this build).
template<>
JoystickSetup& LibraryWrapper<JoystickSetup>::instance()
{
	alignas(JoystickSetup) static char storage[sizeof(JoystickSetup)];
	return *reinterpret_cast<JoystickSetup*>(storage);
}

bool InitDirectInput(HWND) { return false; }
void FreeDirectInput() {}
bool UpdateDirectInputState() { return false; }
