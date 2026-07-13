#pragma once
#ifdef _WIN32
#  include <windows.h>			// DWORD, HANDLE
#else
#  include "Platform/WindowsAPI.h"
#endif
// DirectPlay 8, declared for every platform — including Windows, which is why this
// lives here and not with the non-Windows shims. Microsoft removed DirectPlay from
// the Windows SDK, so <dplay8.h> is missing there too, and P2P_interface.h names
// these types whether or not the DirectPlay backend is built (it no longer is).
// These declarations exist only so the Network headers parse. Real cross-platform
// networking is a later replacement (Track B, like the renderer).
typedef struct IDirectPlay8Peer    IDirectPlay8Peer;
typedef struct IDirectPlay8Address IDirectPlay8Address;
typedef IDirectPlay8Peer*          PDIRECTPLAY8PEER;
typedef IDirectPlay8Address*       PDIRECTPLAY8ADDRESS;

typedef DWORD  DPNID;
typedef HANDLE DPNHANDLE;

// Used only via pointer/reference in the headers, so incomplete types suffice.
typedef struct _DPN_CONNECTION_INFO        DPN_CONNECTION_INFO;
typedef struct _DPN_APPLICATION_DESC       DPN_APPLICATION_DESC;
typedef struct _DPNMSG_ENUM_HOSTS_RESPONSE DPNMSG_ENUM_HOSTS_RESPONSE;
