#pragma once
// Windows shell API — stub for non-Windows builds.
// Only the bits used for locating the user's profile/save directory are
// modelled. Real cross-platform path resolution (e.g. SDL_GetPrefPath) is a
// later concern; here we just need the headers to parse and a benign default.
#include "../WindowsAPI.h"

#define CSIDL_PERSONAL      0x0005
#define CSIDL_FLAG_CREATE   0x8000
#define SHGFP_TYPE_CURRENT  0
#define SHGFP_TYPE_DEFAULT  1

inline HRESULT SHGetFolderPathA(HWND, int, HANDLE, DWORD, char* path) {
    if(path) path[0] = '\0';
    return S_OK;
}
