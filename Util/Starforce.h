#ifndef __STARFORCE_H__
#define __STARFORCE_H__

#ifdef USE_SECUROM

//#define STARFORCE_API extern "C" __declspec(dllexport)
//#define STARFORCE_API __declspec(dllexport)
#define STARFORCE_API 

#else

#define STARFORCE_API __declspec(dllexport)

#define SECUROM_MARKER_HIGH_SECURITY_ON(x)
#define SECUROM_MARKER_HIGH_SECURITY_OFF(x)
#define SecuROM_Tripwire() 1

#endif

STARFORCE_API void initConditions();
STARFORCE_API void initActions();
STARFORCE_API void initActionsEnvironmental();
STARFORCE_API void initActionsSound();

#endif //__STARFORCE_H__
