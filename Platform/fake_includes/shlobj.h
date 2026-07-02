#pragma once
// Windows shell API — stub for non-Windows builds.
// Only the bits used for locating the user's profile/save directory are
// modelled: CSIDL_PERSONAL ("My Documents") resolves to the user's home
// directory, under which the engine creates "My Games/<savePath>/".
#include "../WindowsAPI.h"

#define CSIDL_PERSONAL      0x0005
#define CSIDL_FLAG_CREATE   0x8000
#define SHGFP_TYPE_CURRENT  0
#define SHGFP_TYPE_DEFAULT  1

// Defined out-of-line in Platform/WindowsAPI.cpp (WindowsAPI.h is force-included
// into every TU, so keeping a body here would rebuild the world on each edit).
HRESULT SHGetFolderPathA(HWND hwnd, int csidl, HANDLE token, DWORD flags, char* path);
