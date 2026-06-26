#pragma once
#include "../WindowsAPI.h"
// DirectSound — stub for non-Windows builds. Replaced by SDL3 audio.
typedef struct IDirectSound8           IDirectSound8;
typedef struct IDirectSoundBuffer       IDirectSoundBuffer;
typedef struct IDirectSoundBuffer8      IDirectSoundBuffer8;
typedef struct IDirectSound3DBuffer     IDirectSound3DBuffer;
typedef struct IDirectSound3DBuffer8    IDirectSound3DBuffer8;
typedef struct IDirectSound3DListener8  IDirectSound3DListener8;
typedef IDirectSound8*                  LPDIRECTSOUND8;
typedef IDirectSoundBuffer*             LPDIRECTSOUNDBUFFER;
typedef IDirectSoundBuffer8*            LPDIRECTSOUNDBUFFER8;
typedef IDirectSound3DBuffer8*          LPDIRECTSOUND3DBUFFER8;
typedef IDirectSound3DListener8*        LPDIRECTSOUND3DLISTENER8;

// DS3DBUFFER is held by value in the Sound headers, so it must be a complete
// type. Fields are unused on non-Windows (the DirectSound backend isn't built).
struct DS3DBUFFER { DWORD dwSize; };

struct DSBUFFERDESC { DWORD dwSize, dwFlags, dwBufferBytes, dwReserved; void* lpwfxFormat; };
struct WAVEFORMATEX { WORD wFormatTag, nChannels; DWORD nSamplesPerSec, nAvgBytesPerSec;
                      WORD nBlockAlign, wBitsPerSample, cbSize; };
