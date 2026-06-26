#pragma once
#include "../WindowsAPI.h"
// Video for Windows (AVI) — stub for non-Windows builds. The AVI reader/writer
// (Render/src/WinVideo.cpp) is Windows-only; these declarations exist only so
// WinVideo.h parses for its callers (GameShell.h, ShowHead.cpp, EasyMap.cpp).

typedef struct _AVIFILE*   PAVIFILE;
typedef struct _AVISTREAM* PAVISTREAM;
