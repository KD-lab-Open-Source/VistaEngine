#pragma once
// MSVC <strsafe.h> stub — safe string functions
#include <cstring>
#include <cstdarg>
#include <cstdio>
#ifndef STRSAFE_E_INSUFFICIENT_BUFFER
#define STRSAFE_E_INSUFFICIENT_BUFFER ((HRESULT)0x8007007AL)
#endif
typedef char* STRSAFE_LPSTR;
typedef const char* STRSAFE_LPCSTR;
inline HRESULT StringCchCopyA(char* d, size_t n, const char* s) { strncpy(d,s,n); d[n-1]=0; return S_OK; }
inline HRESULT StringCchCopyW(wchar_t* d, size_t n, const wchar_t* s) { wcsncpy(d,s,n); d[n-1]=0; return S_OK; }
inline HRESULT StringCchPrintfA(char* d, size_t n, const char* fmt, ...) {
    va_list a; va_start(a,fmt); vsnprintf(d,(size_t)n,fmt,a); va_end(a); return S_OK;
}
inline HRESULT StringCbVPrintf(char* d, size_t cb, const char* fmt, va_list args) {
    if(cb) { vsnprintf(d,(size_t)cb,fmt,args); d[cb-1]=0; } return S_OK;
}
inline HRESULT StringCchLengthA(const char* s, size_t n, size_t* len) { *len=strnlen(s,n); return S_OK; }
inline HRESULT StringCchCatA(char* d, size_t n, const char* s) { strncat(d,s,n-strlen(d)-1); return S_OK; }
#define StringCchCopy  StringCchCopyA
#define StringCchPrintf StringCchPrintfA
#define StringCchLength StringCchLengthA
#define StringCchCat   StringCchCatA
