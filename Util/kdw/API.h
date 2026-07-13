#ifndef __KDW_API_H_INCLUDED__
#define __KDW_API_H_INCLUDED__

#pragma warning(disable : 4251) // class '' needs to have dll-interface to be used by clients of class ''
#pragma warning(disable : 4275) // non dll-interface class '' used as base for dll-interface class ''

// Empty, for the same reason as RENDER_API (Render/inc/rd.h): _DLL is MSVC's flag for
// linking against the DLL runtime, not for building one. kdw is not built at all now
// (kdwStub.cpp provides its entry points), and declaring those __declspec(dllimport)
// while the stub defines them is what produced the 'inconsistent dll linkage' warnings.
#define KDW_API


#endif
