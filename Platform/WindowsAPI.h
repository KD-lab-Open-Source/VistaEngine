#pragma once

// Cross-platform shim for Windows API types, macros, and inline stubs.
// On Windows the real headers are used. On all other platforms this file
// provides the minimum surface needed to compile the codebase.

#ifdef _WIN32

#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>

#else // !_WIN32

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <chrono>

// ─── Basic integer types ──────────────────────────────────────────────────────
typedef uint8_t   BYTE;
typedef uint16_t  WORD;
typedef uint32_t  DWORD;
typedef uint64_t  QWORD;
typedef int32_t   LONG;
typedef uint32_t  ULONG;
typedef int64_t   LONGLONG;
typedef uint64_t  ULONGLONG;
typedef int32_t   INT;
typedef uint32_t  UINT;
typedef int16_t   SHORT;
typedef uint16_t  USHORT;
typedef int32_t   BOOL;
typedef float     FLOAT;

// ─── Character / string types ─────────────────────────────────────────────────
typedef char      CHAR;
typedef wchar_t   WCHAR;
typedef char      TCHAR;
typedef char*     LPSTR;
typedef const char*    LPCSTR;
typedef wchar_t*  LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef TCHAR*    LPTSTR;
typedef const TCHAR*   LPCTSTR;

typedef void*     LPVOID;
typedef void*     PVOID;
typedef const void*    LPCVOID;

typedef LONG      HRESULT;
typedef DWORD     COLORREF;

// ─── Handle types ─────────────────────────────────────────────────────────────
typedef void* HANDLE;
typedef void* HWND;
typedef void* HDC;
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HBRUSH;
typedef void* HMENU;
typedef void* HBITMAP;
typedef void* HRGN;
typedef void* HPEN;
typedef void* HFONT;
typedef void* HGDIOBJ;
typedef void* HKEY;
typedef HANDLE HFILE;
typedef HANDLE HRSRC;
typedef HANDLE HGLOBAL;

typedef intptr_t   LONG_PTR;
typedef uintptr_t  ULONG_PTR;
typedef uintptr_t  SIZE_T;
typedef intptr_t   SSIZE_T;
typedef ULONG_PTR  DWORD_PTR;
typedef LONG_PTR   LPARAM;
typedef ULONG_PTR  WPARAM;
typedef LONG_PTR   LRESULT;

// ─── Boolean / NULL ───────────────────────────────────────────────────────────
#ifndef TRUE
#  define TRUE  1
#endif
#ifndef FALSE
#  define FALSE 0
#endif
#ifndef NULL
#  define NULL  nullptr
#endif

// ─── Limits ───────────────────────────────────────────────────────────────────
#ifndef MAX_PATH
#  define MAX_PATH 260
#endif
#define _MAX_PATH   MAX_PATH
#define _MAX_DIR    256
#define _MAX_DRIVE  3
#define _MAX_FNAME  256
#define _MAX_EXT    256

// ─── Calling conventions (no-ops on non-MSVC) ─────────────────────────────────
#ifndef WINAPI
#  define WINAPI
#endif
#define WINAPIV
#define CALLBACK
#define APIENTRY
#define CDECL
#ifndef __cdecl
#  define __cdecl
#endif
#ifndef __stdcall
#  define __stdcall
#endif
#define STDCALL
#define PASCAL

// ─── declspec (used in DLL interface macros) ──────────────────────────────────
#define __declspec(x)
#define DECLSPEC_NOVTABLE
#define __forceinline __attribute__((always_inline))
#define __assume(x)

// ─── Handle sentinel ──────────────────────────────────────────────────────────
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

// ─── HRESULT constants ────────────────────────────────────────────────────────
#define S_OK              ((HRESULT)0L)
#define S_FALSE           ((HRESULT)1L)
#define E_NOTIMPL         ((HRESULT)0x80004001L)
#define E_NOINTERFACE     ((HRESULT)0x80004002L)
#define E_POINTER         ((HRESULT)0x80004003L)
#define E_ABORT           ((HRESULT)0x80004004L)
#define E_FAIL            ((HRESULT)0x80004005L)
#define E_UNEXPECTED      ((HRESULT)0x8000FFFFL)
#define E_OUTOFMEMORY     ((HRESULT)0x8007000EL)
#define E_INVALIDARG      ((HRESULT)0x80070057L)
#define E_ACCESSDENIED    ((HRESULT)0x80070005L)
#define DXGI_ERROR_NOT_FOUND ((HRESULT)0x887A0002L)

#define FAILED(hr)    ((HRESULT)(hr) < 0)
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#define MAKE_HRESULT(sev, fac, code) \
    ((HRESULT)(((DWORD)(sev)<<31)|((DWORD)(fac)<<16)|((DWORD)(code))))

// ─── GUID ─────────────────────────────────────────────────────────────────────
struct GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
};
typedef GUID  IID;
typedef GUID  CLSID;
typedef const GUID& REFGUID;
typedef const IID&  REFIID;
typedef const CLSID& REFCLSID;
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    extern const GUID name
inline bool operator==(const GUID& a, const GUID& b) {
    return __builtin_memcmp(&a, &b, sizeof(GUID)) == 0;
}
inline bool operator!=(const GUID& a, const GUID& b) { return !(a == b); }

// ─── COM interface macros ─────────────────────────────────────────────────────
#define PURE    = 0
#define THIS_
#define THIS    void
#define DECLARE_INTERFACE(iface)            struct iface
#define DECLARE_INTERFACE_(iface, base)     struct iface : public base
#define STDMETHOD(m)                        virtual HRESULT m
#define STDMETHOD_(t, m)                    virtual t m
#define STDMETHODIMP                        HRESULT
#define STDMETHODIMP_(t)                    t
#define STDMETHODCALLTYPE
#define IFACEMETHOD(m)                      STDMETHOD(m)
#define IFACEMETHOD_(t, m)                  STDMETHOD_(t, m)

struct IUnknown {
    virtual HRESULT QueryInterface(REFIID, void**) PURE;
    virtual ULONG   AddRef() PURE;
    virtual ULONG   Release() PURE;
    virtual ~IUnknown() = default;
};

// ─── POINT / RECT / SIZE ──────────────────────────────────────────────────────
struct POINT  { LONG x, y; };
struct RECT   { LONG left, top, right, bottom; };
struct SIZE   { LONG cx, cy; };

// ─── Bit / word manipulation macros ──────────────────────────────────────────
#define LOWORD(l)         ((WORD)(((DWORD)(l)) & 0xffff))
#define HIWORD(l)         ((WORD)((((DWORD)(l)) >> 16) & 0xffff))
#define LOBYTE(w)         ((BYTE)(((DWORD)(w)) & 0xff))
#define HIBYTE(w)         ((BYTE)((((DWORD)(w)) >> 8) & 0xff))
#define MAKELONG(lo, hi)  ((LONG)(((WORD)(lo)) | (((DWORD)((WORD)(hi))) << 16)))
#define MAKEWORD(lo, hi)  ((WORD)(((BYTE)(lo)) | (((WORD)((BYTE)(hi))) << 8)))
#define MAKEWPARAM(l, h)  ((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELPARAM(l, h)  ((LPARAM)(DWORD)MAKELONG(l, h))
#define GET_X_LPARAM(lp)  ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)  ((int)(short)HIWORD(lp))

// ─── Memory utilities ─────────────────────────────────────────────────────────
#define ZeroMemory(p, n)     memset(p, 0, n)
#define CopyMemory(d, s, n)  memcpy(d, s, n)
#define MoveMemory(d, s, n)  memmove(d, s, n)
#define FillMemory(p, n, v)  memset(p, v, n)
#define RtlZeroMemory(p, n)  memset(p, 0, n)

// ─── String utilities (MSVC _prefix → POSIX) ─────────────────────────────────
#include <strings.h>
#define _stricmp      strcasecmp
#define _strnicmp     strncasecmp
#define stricmp       strcasecmp
#define strnicmp      strncasecmp
#define _snprintf     snprintf
#define _vsnprintf    vsnprintf
#define _snwprintf    swprintf
#define _strlwr(s)    ({ char* _s=(s); for(char* _p=_s;*_p;_p++) *_p=tolower((unsigned char)*_p); _s; })
#define _strupr(s)    ({ char* _s=(s); for(char* _p=_s;*_p;_p++) *_p=toupper((unsigned char)*_p); _s; })
#define _itoa(v, buf, r) (sprintf(buf, (r)==16 ? "%x" : "%d", v), buf)
#define _ltoa(v, buf, r) (sprintf(buf, (r)==16 ? "%lx" : "%ld", (long)(v)), buf)
#define _ultoa(v, buf, r) (sprintf(buf, (r)==16 ? "%lx" : "%lu", (unsigned long)(v)), buf)
#define _gcvt(v, d, buf) (sprintf(buf, "%.*g", d, (double)(v)), buf)

// ─── Directory / file utilities ───────────────────────────────────────────────
#include <sys/stat.h>
#include <unistd.h>
#define _mkdir(path)    mkdir(path, 0755)
#define _rmdir          rmdir
#define _chdir          chdir
#define _getcwd         getcwd
#define _unlink         unlink
#define _access         access

// ─── File attribute / access constants ───────────────────────────────────────
#define FILE_ATTRIBUTE_NORMAL      0x00000080
#define FILE_ATTRIBUTE_DIRECTORY   0x00000010
#define FILE_ATTRIBUTE_READONLY    0x00000001
#define GENERIC_READ               0x80000000
#define GENERIC_WRITE              0x40000000
#define FILE_SHARE_READ            0x00000001
#define FILE_SHARE_WRITE           0x00000002
#define OPEN_EXISTING              3
#define OPEN_ALWAYS                4
#define CREATE_ALWAYS              2
#define CREATE_NEW                 1
#define TRUNCATE_EXISTING          5

// ─── Interlocked ops (placeholders — replace with <atomic> later) ─────────────
inline LONG InterlockedIncrement(LONG volatile* p) { return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST); }
inline LONG InterlockedDecrement(LONG volatile* p) { return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST); }
inline LONG InterlockedExchange(LONG volatile* p, LONG v) {
    LONG old; __atomic_exchange(p, &v, &old, __ATOMIC_SEQ_CST); return old;
}
inline LONG InterlockedCompareExchange(LONG volatile* p, LONG ex, LONG cmp) {
    __atomic_compare_exchange_n(p, &cmp, ex, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return cmp;
}

// ─── Debug output ─────────────────────────────────────────────────────────────
inline void OutputDebugStringA(const char* s)    { fputs(s, stderr); }
inline void OutputDebugStringW(const wchar_t* s) { fputws(s, stderr); }
#define OutputDebugString OutputDebugStringA
#define DebugBreak()      __builtin_trap()

// ─── Module / process stubs ───────────────────────────────────────────────────
inline HMODULE GetModuleHandle(const char*)     { return nullptr; }
inline DWORD   GetLastError()                   { return 0; }
inline void    SetLastError(DWORD)              {}
inline DWORD   GetCurrentThreadId()             { return (DWORD)(uintptr_t)pthread_self(); }
inline DWORD   GetCurrentProcessId()            { return (DWORD)getpid(); }
inline BOOL    CloseHandle(HANDLE)              { return TRUE; }
inline LPVOID  GlobalLock(HGLOBAL h)            { return h; }
inline BOOL    GlobalUnlock(HGLOBAL)            { return TRUE; }
inline BOOL    FreeLibrary(HMODULE)             { return TRUE; }

// needs pthread
#include <pthread.h>
#include <unistd.h>

// ─── Timing ───────────────────────────────────────────────────────────────────
inline DWORD timeGetTime() {
    using namespace std::chrono;
    return (DWORD)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
inline DWORD GetTickCount() { return timeGetTime(); }
inline void  Sleep(DWORD ms) { usleep(ms * 1000); }

// ─── Window message stubs ─────────────────────────────────────────────────────
struct MSG {
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
};
#define WM_QUIT        0x0012
#define WM_KEYDOWN     0x0100
#define WM_KEYUP       0x0101
#define WM_LBUTTONDOWN 0x0201
#define WM_RBUTTONDOWN 0x0204
#define WM_SIZE        0x0005

inline BOOL    PeekMessage(MSG*, HWND, UINT, UINT, UINT) { return FALSE; }
inline BOOL    GetMessage(MSG*, HWND, UINT, UINT)         { return FALSE; }
inline BOOL    TranslateMessage(const MSG*)               { return FALSE; }
inline LRESULT DispatchMessage(const MSG*)                { return 0; }
inline void    PostQuitMessage(int)                       {}
inline HWND    GetForegroundWindow()                      { return nullptr; }
inline BOOL    SetForegroundWindow(HWND)                  { return TRUE; }

// ─── Critical section stubs ───────────────────────────────────────────────────
struct CRITICAL_SECTION { pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; };
inline void InitializeCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_init(&cs->mutex, nullptr); }
inline void DeleteCriticalSection(CRITICAL_SECTION* cs)     { pthread_mutex_destroy(&cs->mutex); }
inline void EnterCriticalSection(CRITICAL_SECTION* cs)      { pthread_mutex_lock(&cs->mutex); }
inline void LeaveCriticalSection(CRITICAL_SECTION* cs)      { pthread_mutex_unlock(&cs->mutex); }

// ─── Thread stubs ─────────────────────────────────────────────────────────────
typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID);
inline HANDLE CreateThread(void*, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, DWORD*) { return nullptr; }
inline DWORD  WaitForSingleObject(HANDLE, DWORD)  { return 0; }
inline BOOL   SetEvent(HANDLE)                    { return TRUE; }
inline BOOL   ResetEvent(HANDLE)                  { return TRUE; }
inline HANDLE CreateEvent(void*, BOOL, BOOL, const char*) { return nullptr; }
#define WAIT_OBJECT_0  0
#define WAIT_TIMEOUT   0x00000102
#define INFINITE       0xFFFFFFFF

// ─── Registry stubs ───────────────────────────────────────────────────────────
typedef HKEY* PHKEY;
#define HKEY_LOCAL_MACHINE  ((HKEY)(ULONG_PTR)0x80000002)
#define HKEY_CURRENT_USER   ((HKEY)(ULONG_PTR)0x80000001)
#define KEY_READ            0x20019
#define KEY_WRITE           0x20006
#define REG_SZ              1
#define REG_DWORD           4
inline LONG RegOpenKeyExA(HKEY, const char*, DWORD, DWORD, PHKEY) { return 1; }
inline LONG RegQueryValueExA(HKEY, const char*, DWORD*, DWORD*, BYTE*, DWORD*) { return 1; }
inline LONG RegCloseKey(HKEY) { return 0; }
inline LONG RegSetValueExA(HKEY, const char*, DWORD, DWORD, const BYTE*, DWORD) { return 1; }
inline LONG RegCreateKeyExA(HKEY, const char*, DWORD, char*, DWORD, DWORD, void*, PHKEY, DWORD*) { return 1; }

// ─── Pragma warning (ignore MSVC-specific pragmas) ───────────────────────────
#define PRAGMA_WARNING_PUSH
#define PRAGMA_WARNING_POP

// ─── Assert-like macro used in some modules ──────────────────────────────────
#ifndef MDebugBreak
#  define MDebugBreak() __builtin_trap()
#endif

#endif // !_WIN32
