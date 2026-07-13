#pragma once 

#pragma warning(disable : 4251) // class '' needs to have dll-interface to be used by clients of class ''
#pragma warning(disable : 4275) // non dll-interface class '' used as base for dll-interface class ''

// Empty: every module here is a static library, on every platform.
//
// This used to key off _DLL, which MSVC defines when you link against the *DLL runtime*
// (/MD, CMake's default) — it has never meant "this project is a DLL". So a static
// Render was declaring its symbols __declspec(dllimport) and then defining them, which
// MSVC rejects outright (C2491 on pLibrary3dx, C2487 on TileMap::serialize).
#define RENDER_API
