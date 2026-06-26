#pragma once
#include "../WindowsAPI.h"
#include <cctype>
#include <strings.h>
// Windows Shell Lightweight API (shlwapi.h) — minimal stub for non-Windows builds.

// Case-insensitive string comparisons (return <0 / 0 / >0).
inline int StrCmpI(LPCSTR a, LPCSTR b)            { return strcasecmp(a, b); }
inline int StrCmpNI(LPCSTR a, LPCSTR b, int n)    { return strncasecmp(a, b, (size_t)n); }

// Case-insensitive substring search; returns a pointer into pszFirst, or null.
inline LPCSTR StrStrI(LPCSTR pszFirst, LPCSTR pszSrch) {
    if(!pszFirst || !pszSrch) return nullptr;
    if(!*pszSrch) return pszFirst;
    for(const char* p = pszFirst; *p; ++p) {
        const char* a = p; const char* b = pszSrch;
        while(*a && *b && std::tolower((unsigned char)*a) == std::tolower((unsigned char)*b)) { ++a; ++b; }
        if(!*b) return p;
    }
    return nullptr;
}
