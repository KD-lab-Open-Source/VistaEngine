#ifndef __UNITS_STD_AFX_H_INCLUDED__
#define __UNITS_STD_AFX_H_INCLUDED__
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT  0x0400
#include <windows.h>

#include <my_STL.h>

// Standart includes
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <typeinfo>
#include <mmsystem.h>
#include <dplay8.h>

// STL
#include <vector> 
#include <string>
#include <algorithm>

using namespace std;

// XTool
#include "xutil.h"
#include "xmath.h"
#include "xzip.h"

#include "IRenderDevice.h"
#include "IVisGeneric.h"
#include "VisGenericDefine.h"

#include "Profiler.h"
#include "SystemUtil.h"
#include "DebugUtil.h"

#include "SwapVector.h"

#endif
