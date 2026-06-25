#pragma once
#include "../WindowsAPI.h"
// DirectPlay 8 — stub for non-Windows builds. Replaced by cross-platform networking.
typedef struct IDirectPlay8Peer    IDirectPlay8Peer;
typedef struct IDirectPlay8Address IDirectPlay8Address;
typedef IDirectPlay8Peer*          PDIRECTPLAY8PEER;
typedef IDirectPlay8Address*       PDIRECTPLAY8ADDRESS;
