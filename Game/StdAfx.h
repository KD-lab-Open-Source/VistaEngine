#ifndef __GAME_STD_AFX_H_INCLUDED__
#define __GAME_STD_AFX_H_INCLUDED__

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// A fallback now, not a choice: the root CMakeLists force-includes <windows.h> into every
// C++ TU on Windows (/FIwindows.h), so sdkddkver.h has already picked a value by the time
// this line is reached, and windows.h itself was parsed with *that* one. Left unguarded it
// redefined the SDK's value -- 62 C4005 warnings a build, and two API levels in one TU.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT  0x0501
#endif
#include <windows.h>

#include <my_STL.h>

// Standart includes
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <direct.h>

#include <mmsystem.h>
#include <dplay8.h>

// STL
#include <vector> 
#include <string>
#include <algorithm>

using namespace std;

// XTool
#include "xutil.h"
#include "XMath/xmath.h"
#include "XZip.h"

#include "Profiler.h"
#include "SystemUtil.h"
#include "DebugUtil.h"
#include "DebugPrm.h"

#include "XTL/SwapVector.h"

#endif
