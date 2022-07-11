#pragma once 

#pragma warning(disable : 4251) // class '' needs to have dll-interface to be used by clients of class ''
#pragma warning(disable : 4275) // non dll-interface class '' used as base for dll-interface class ''

#ifdef _DLL
	#ifdef RENDER_EXPORTS
	#define RENDER_API __declspec(dllexport)
	#else
	#define RENDER_API __declspec(dllimport)
	#endif
#else
	#define RENDER_API
#endif
