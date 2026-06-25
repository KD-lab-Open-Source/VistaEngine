#pragma once
#include "../WindowsAPI.h"
// DirectInput 8 — stub for non-Windows builds. Replaced by SDL3 input.
typedef struct IDirectInput8A       IDirectInput8A;
typedef struct IDirectInputDevice8A IDirectInputDevice8A;
typedef IDirectInput8A*             LPDIRECTINPUT8;
typedef IDirectInputDevice8A*       LPDIRECTINPUTDEVICE8;
