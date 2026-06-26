#pragma once
#include "../WindowsAPI.h"
// Windows Multimedia (mmio) — stub for non-Windows builds. The WAVE-file
// reader/writer that uses these (Sound/WaveFile.cpp) is Windows-only; these
// declarations exist only so headers that hold these types by value
// (e.g. Sound/SoundInternal.h's CWaveFile) parse cross-platform.

typedef DWORD FOURCC;
typedef void* HMMIO;

struct MMCKINFO {
    FOURCC ckid;
    DWORD  cksize;
    FOURCC fccType;
    DWORD  dwDataOffset;
    DWORD  dwFlags;
};

struct MMIOINFO {
    DWORD  dwFlags;
    FOURCC fccIOProc;
    void*  pIOProc;
    DWORD  wErrorRet;
    void*  htask;
    long   cchBuffer;
    char*  pchBuffer;
    char*  pchNext;
    char*  pchEndRead;
    char*  pchEndWrite;
    long   lBufOffset;
    long   lDiskOffset;
    DWORD  adwInfo[4];
    DWORD  dwReserved1;
    DWORD  dwReserved2;
    HMMIO  hmmio;
};
