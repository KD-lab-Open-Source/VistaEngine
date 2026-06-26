#pragma once
#include "../WindowsAPI.h"
// DirectPlay 8 — stub for non-Windows builds. The DirectPlay networking backend
// (P2P_interface{1Th,3Th,2ThDPF,Aux}.cpp) is Windows-only; these declarations
// exist only so the Network headers (P2P_interface.h) parse for callers. Real
// cross-platform networking is a later replacement (Track B, like the renderer).
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
