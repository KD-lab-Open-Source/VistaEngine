#pragma once
// Windows shell API — stub for non-Windows builds.
// Only ShellExecute (used to open URLs in the default browser) is modelled.
// A cross-platform replacement (e.g. SDL_OpenURL) is a later concern.
#include "../WindowsAPI.h"

#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif

inline HINSTANCE ShellExecuteA(HWND, const char*, const char*, const char*, const char*, int) {
    return (HINSTANCE)0;
}
#define ShellExecute ShellExecuteA
