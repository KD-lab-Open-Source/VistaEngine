// define parameter _LIB_NAME

#if defined(_AFXDLL)
#define _MT_DLL_SUFFIX "Dll"
#elif defined(_DLL) || defined(_MTD)
#define _MT_DLL_SUFFIX "Dll"
#elif defined(_MT)
#define _MT_DLL_SUFFIX "Mt"
#else
#define _MT_DLL_SUFFIX 
#endif 

#ifdef _DEBUG
#define _DEBUG_SUFFIX "Dbg"
#else 
#define _DEBUG_SUFFIX 
#endif 

// No #pragma comment(lib, ...) any more. It asked the linker for names like
// "XUtilDll.lib" — Dll because MSVC defines _DLL when linking the DLL runtime (/MD),
// which is not what this header thought it meant — and no such library exists: CMake
// builds XUtil as a static library and declares who links it. The suffix machinery
// above is left as the record of what the name used to be.
#define _FULL_NAME_ _LIB_NAME _MT_DLL_SUFFIX _DEBUG_SUFFIX ".lib"

#undef _FULL_NAME_
#undef _DEBUG_SUFFIX
#undef _MT_DLL_SUFFIX
#undef _LIB_NAME

