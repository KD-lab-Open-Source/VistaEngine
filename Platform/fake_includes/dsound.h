#pragma once
#include "../WindowsAPI.h"
// DirectSound — stub for non-Windows builds. Replaced by SDL3 audio.
typedef struct IDirectSound8        IDirectSound8;
typedef struct IDirectSoundBuffer8  IDirectSoundBuffer8;
typedef struct IDirectSound3DBuffer IDirectSound3DBuffer;
typedef IDirectSound8*              LPDIRECTSOUND8;
typedef IDirectSoundBuffer8*        LPDIRECTSOUNDBUFFER8;

struct DSBUFFERDESC { DWORD dwSize, dwFlags, dwBufferBytes, dwReserved; void* lpwfxFormat; };
struct WAVEFORMATEX { WORD wFormatTag, nChannels; DWORD nSamplesPerSec, nAvgBytesPerSec;
                      WORD nBlockAlign, wBitsPerSample, cbSize; };
