#ifndef __KDW_WIN32_TYPES_H_INCLUDED__
#define __KDW_WIN32_TYPES_H_INCLUDED__

#ifdef _WIN32
// The SDK's own types. This header used to hand-roll the lot of them to avoid
// including windows.h — and declared UINT_PTR and LONG_PTR as 32-bit ints, which a
// 64-bit build rejects outright: basetsd.h makes them pointer-sized. The game only
// parses these headers (kdw itself is no longer built anywhere, see Util/CMakeLists),
// so let the SDK be the one source of truth for them.
#include <windows.h>
#include <commctrl.h>	// HIMAGELIST
#endif // _WIN32


namespace Win32{

}

#endif
