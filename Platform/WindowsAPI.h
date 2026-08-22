#pragma once

// Cross-platform shim for Windows API types, macros, and inline stubs.
// On Windows the real headers are used. On all other platforms this file
// provides the minimum surface needed to compile the codebase.

#ifdef _WIN32

#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>

// The SDL event pump (PlatformWindow::pumpEvents) records every key transition it
// sees, because off-Windows that table *is* GetAsyncKeyState (see its declaration
// below). Here the real GetAsyncKeyState reads the OS keyboard state directly and
// needs no help, so the pump's bookkeeping calls fold away.
inline void PlatformSetKeyState(int /*vk*/, bool /*down*/) {}
inline void PlatformClearKeyStates() {}
// Likewise for the cursor: the real GetCursorPos asks the OS, so the pump's latch
// is not needed here.
inline void PlatformSetMousePosition(int /*x*/, int /*y*/) {}

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
typedef uint64_t  DWORD64;
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
typedef unsigned char UCHAR;
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
typedef void* HACCEL;
typedef void* HDWP;
typedef void* HBITMAP;
typedef void* HRGN;
typedef void* HPEN;
typedef void* HFONT;
typedef void* HGDIOBJ;
typedef void* HKEY;
typedef void* HIMAGELIST;
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
#define itoa  _itoa
#define ltoa  _ltoa
#define ultoa _ultoa
#define _gcvt(v, d, buf) (sprintf(buf, "%.*g", d, (double)(v)), buf)

// ─── Directory / file utilities ───────────────────────────────────────────────
// No <dirent.h> here, deliberately. This header is force-included into every
// translation unit, so anything it includes is included everywhere — and dirent
// brings the whole POSIX DT_* namespace with it, which collides with the engine's
// own names (Render/D3D/DrawType.h has a DT_UNKNOWN). Directory iteration is done
// with <filesystem> in WindowsAPI.cpp, where it belongs.
#include <sys/stat.h>
#include <unistd.h>
#include <string>

// ─── Path normalization ───────────────────────────────────────────────────────
// Windows paths are case-insensitive and use '\' separators; the engine hard-codes
// both. On case-sensitive POSIX filesystems (Linux, and case-sensitive macOS
// volumes) such a path won't resolve unless every component matches on disk
// exactly. NormalizePath rewrites a Windows-style path into a real POSIX path:
//   * '\' separators become '/';
//   * each existing component is replaced with its real on-disk spelling, found
//     by a case-insensitive scan of the parent directory.
// Components that don't exist on disk (e.g. the leaf of a file/dir about to be
// created, or a wildcard pattern) are kept verbatim. All path-consuming wrappers
// below funnel their argument through this first.
//
// Defined out-of-line in Platform/WindowsAPI.cpp: this header is force-included
// into every translation unit, so keeping the body here would make every edit to
// the path logic trigger a full rebuild.
std::string NormalizePath(const char* path);

inline int _mkdir(const char* path)              { return mkdir(NormalizePath(path).c_str(), 0755); }
inline int _rmdir(const char* path)              { return rmdir(NormalizePath(path).c_str()); }
inline int _chdir(const char* path)              { return chdir(NormalizePath(path).c_str()); }
#define _getcwd         getcwd
inline int _unlink(const char* path)             { return unlink(NormalizePath(path).c_str()); }
inline int _access(const char* path, int mode)   { return access(NormalizePath(path).c_str(), mode); }

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

// Native `long` overloads: engine code uses `volatile long` counters, and on
// LP64 (Linux/macOS) `long` is 64-bit and distinct from LONG (int32_t), so these
// don't collide with the overloads above. (On Win32 the real API is used.)
inline long InterlockedIncrement(long volatile* p) { return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST); }
inline long InterlockedDecrement(long volatile* p) { return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST); }
inline long InterlockedExchange(long volatile* p, long v) {
    long old; __atomic_exchange(p, &v, &old, __ATOMIC_SEQ_CST); return old;
}
inline long InterlockedCompareExchange(long volatile* p, long ex, long cmp) {
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
// CreateFileA hands out FILE* handles; CloseHandle must fclose those (it's a real
// out-of-line function, not a no-op stub, or every CreateFile leaks an fd). It is a
// no-op for non-file handles (events/threads). See WindowsAPI.cpp.
BOOL           CloseHandle(HANDLE);
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
#define WM_ACTIVATEAPP 0x001C
#define WM_CHAR        0x0102
#define WM_SYSKEYDOWN  0x0104
#define WM_SYSKEYUP    0x0105
#define WM_MOUSEMOVE   0x0200
#define WM_LBUTTONUP   0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONUP   0x0205
#define WM_RBUTTONDBLCLK 0x0206
#define WM_MBUTTONDOWN 0x0207
#define WM_MBUTTONUP   0x0208
#define WM_MBUTTONDBLCLK 0x0209
#define WM_MOUSELAST   0x020E
#define WM_MOUSELEAVE  0x02A3
#define WM_UNICHAR     0x0109

// Mouse-key state flags (winuser.h); used by UI input handling.
#define MK_LBUTTON  0x0001
#define MK_RBUTTON  0x0002
#define MK_SHIFT    0x0004
#define MK_CONTROL  0x0008
#define MK_MBUTTON  0x0010

typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);

// GDI bitmap header (wingdi.h) — used by value in image/video code.
#ifndef _BITMAPINFOHEADER_DEFINED_
#define _BITMAPINFOHEADER_DEFINED_
struct BITMAPINFOHEADER {
    DWORD biSize;
    long  biWidth;
    long  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    long  biXPelsPerMeter;
    long  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
};
#endif

inline BOOL    PeekMessage(MSG*, HWND, UINT, UINT, UINT) { return FALSE; }
inline BOOL    GetMessage(MSG*, HWND, UINT, UINT)         { return FALSE; }
inline BOOL    TranslateMessage(const MSG*)               { return FALSE; }
inline LRESULT DispatchMessage(const MSG*)                { return 0; }
inline void    PostQuitMessage(int)                       {}
inline HWND    GetForegroundWindow()                      { return nullptr; }
inline BOOL    SetForegroundWindow(HWND)                  { return TRUE; }

// ─── System-identity stubs ────────────────────────────────────────────────────
#define MAX_COMPUTERNAME_LENGTH 15
inline BOOL GetComputerName(LPSTR buf, DWORD* size) { if(buf && size && *size) buf[0] = 0; return FALSE; }
inline BOOL GetUserName(LPSTR buf, DWORD* size)     { if(buf && size && *size) buf[0] = 0; return FALSE; }

// ─── Console / thread stubs (debug logging, thread priority) ──────────────────
struct COORD { short X, Y; };
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE  ((DWORD)-12)
inline HANDLE GetStdHandle(DWORD)                                  { return nullptr; }
inline BOOL   AllocConsole()                                       { return FALSE; }
inline BOOL   SetConsoleScreenBufferSize(HANDLE, COORD)            { return FALSE; }
inline BOOL   WriteConsole(HANDLE, const void*, DWORD, DWORD*, void*) { return FALSE; }
inline int    lstrlen(const char* s)                               { return s ? (int)strlen(s) : 0; }

#define THREAD_PRIORITY_HIGHEST       2
#define THREAD_PRIORITY_NORMAL        0
#define THREAD_PRIORITY_ABOVE_NORMAL  1
inline HANDLE GetCurrentThread()              { return nullptr; }
inline BOOL   SetThreadPriority(HANDLE, int)  { return TRUE; }

// ─── Critical section stubs ───────────────────────────────────────────────────
// Win32 CRITICAL_SECTION is recursive (a thread may re-enter a section it already
// owns). Mirror that with a RECURSIVE pthread mutex, else re-entrant locks (e.g.
// ControlManager::registerHotKey -> unRegisterHotKey) self-deadlock.
struct CRITICAL_SECTION { pthread_mutex_t mutex; };
inline void InitializeCriticalSection(CRITICAL_SECTION* cs)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&cs->mutex, &attr);
	pthread_mutexattr_destroy(&attr);
}
inline void DeleteCriticalSection(CRITICAL_SECTION* cs)     { pthread_mutex_destroy(&cs->mutex); }
inline void EnterCriticalSection(CRITICAL_SECTION* cs)      { pthread_mutex_lock(&cs->mutex); }
inline void LeaveCriticalSection(CRITICAL_SECTION* cs)      { pthread_mutex_unlock(&cs->mutex); }

// ─── Thread stubs ─────────────────────────────────────────────────────────────
typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID);
inline HANDLE CreateThread(void*, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, DWORD*) { return nullptr; }
inline DWORD  WaitForSingleObject(HANDLE, DWORD)  { return 0; }
inline DWORD  WaitForMultipleObjects(DWORD, const HANDLE*, BOOL, DWORD) { return 0; }
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
#define KEY_QUERY_VALUE     0x0001
#define REG_SZ              1
#define REG_DWORD           4
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS       0L
#endif
typedef BYTE* LPBYTE;
inline LONG RegOpenKeyExA(HKEY, const char*, DWORD, DWORD, PHKEY) { return 1; }
inline LONG RegQueryValueExA(HKEY, const char*, DWORD*, DWORD*, BYTE*, DWORD*) { return 1; }
inline LONG RegCloseKey(HKEY) { return 0; }
#define RegOpenKeyEx    RegOpenKeyExA
#define RegQueryValueEx RegQueryValueExA
inline LONG RegSetValueExA(HKEY, const char*, DWORD, DWORD, const BYTE*, DWORD) { return 1; }
inline LONG RegCreateKeyExA(HKEY, const char*, DWORD, char*, DWORD, DWORD, void*, PHKEY, DWORD*) { return 1; }
#define RegSetValueEx   RegSetValueExA
#define RegCreateKeyEx  RegCreateKeyExA
#define REG_OPTION_NON_VOLATILE 0

// lstr* string helpers (winbase.h) map to the C library equivalents.
inline char* lstrcpy(char* d, const char* s) { return strcpy(d, s); }
inline char* lstrcpyn(char* d, const char* s, int n) { if(n>0){ strncpy(d, s, (size_t)(n-1)); d[n-1]=0; } return d; }
inline int   lstrcmp(const char* a, const char* b) { return strcmp(a, b); }

// COM init flag + Windows version query (kernel32) — stubs.
#define COINIT_MULTITHREADED 0
inline DWORD GetVersion() { return 0; }
inline HRESULT CoInitializeEx(void*, DWORD) { return 0; }
inline void    CoUninitialize() {}
inline DWORD_PTR SetThreadAffinityMask(HANDLE, DWORD_PTR mask) { return mask; }

// Window-style flags (winuser.h).
#define WS_OVERLAPPED   0x00000000
#define WS_POPUP        0x80000000
#define WS_VISIBLE      0x10000000
#define WS_CAPTION      0x00C00000
#define WS_SYSMENU      0x00080000
#define WS_THICKFRAME   0x00040000
#define WS_MINIMIZEBOX  0x00020000
#define WS_MAXIMIZEBOX  0x00010000
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX)
#define SWP_FRAMECHANGED 0x0020
#define SWP_SHOWWINDOW   0x0040
inline BOOL GetClientRect(HWND, RECT* r) { if(r){ r->left=r->top=0; r->right=0; r->bottom=0; } return TRUE; }

// Window management (winuser.h) — no-op stubs (real windowing = SDL3, Track B).
#define GWL_STYLE     (-16)
#define GWL_EXSTYLE   (-20)
#define HWND_NOTOPMOST ((HWND)(LONG_PTR)-2)
#define HWND_TOPMOST   ((HWND)(LONG_PTR)-1)
#define IMAGE_ICON    1
#define LR_DEFAULTCOLOR 0
#define EVENT_ALL_ACCESS 0x1F0003
inline BOOL SetWindowPos(HWND, HWND, int, int, int, int, UINT) { return TRUE; }
inline LONG SetWindowLongA(HWND, int, LONG v) { return v; }
inline LONG GetWindowLongA(HWND, int) { return 0; }
#define SetWindowLong SetWindowLongA
#define GetWindowLong GetWindowLongA
inline HWND SetFocus(HWND h) { return h; }
inline HWND FindWindowA(const char*, const char*) { return 0; }
#define FindWindow FindWindowA
inline HANDLE OpenEventA(DWORD, BOOL, const char*) { return 0; }
#define OpenEvent OpenEventA
inline BOOL AdjustWindowRect(RECT*, DWORD, BOOL) { return TRUE; }
inline HRESULT CoInitialize(void*) { return 0; }

struct SYSTEM_INFO {
    DWORD dwOemId; DWORD dwPageSize; void* lpMinimumApplicationAddress;
    void* lpMaximumApplicationAddress; DWORD_PTR dwActiveProcessorMask;
    DWORD dwNumberOfProcessors; DWORD dwProcessorType; DWORD dwAllocationGranularity;
    WORD wProcessorLevel; WORD wProcessorRevision;
};
inline void GetSystemInfo(SYSTEM_INFO* si) { if(si){ *si = SYSTEM_INFO{}; si->dwNumberOfProcessors = 1; } }

// Window-class registration / creation (winuser.h). No-op stubs; real windowing
// is the SDL3 replacement (Track B).
#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif
#define CS_VREDRAW 0x0001
#define CS_HREDRAW 0x0002
#define CS_DBLCLKS 0x0008
#define CS_CLASSDC 0x0040
#define BLACK_BRUSH 4
#define WHITE_BRUSH 0
#define HTCLIENT  1
#define WM_CREATE        0x0001
#define WM_DESTROY       0x0002
#define WM_PAINT         0x000F
#define WM_CLOSE         0x0010
#define WM_GETMINMAXINFO 0x0024
#define WM_SETCURSOR     0x0020
#define WM_MOVE          0x0003
#define PM_NOREMOVE      0x0000
#define TME_LEAVE        0x00000002
struct TRACKMOUSEEVENT { DWORD cbSize, dwFlags; HWND hwndTrack; DWORD dwHoverTime; };
inline BOOL TrackMouseEvent(TRACKMOUSEEVENT*) { return TRUE; }
inline BOOL _TrackMouseEvent(TRACKMOUSEEVENT* e) { return TrackMouseEvent(e); }
inline BOOL WaitMessage() { return TRUE; }

struct WNDCLASSEX {
    UINT cbSize, style; WNDPROC lpfnWndProc; int cbClsExtra, cbWndExtra;
    HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; HBRUSH hbrBackground;
    const char* lpszMenuName; const char* lpszClassName; HICON hIconSm;
};
struct MINMAXINFO { POINT ptReserved, ptMaxSize, ptMaxPosition, ptMinTrackSize, ptMaxTrackSize; };
typedef void* HGDIOBJ;
inline HGDIOBJ GetStockObject(int) { return 0; }
typedef WORD   ATOM;
inline ATOM    RegisterClassExA(const WNDCLASSEX*) { return 0; }
inline BOOL    UnregisterClassA(const char*, HINSTANCE) { return TRUE; }
inline HWND    CreateWindowExA(DWORD, const char*, const char*, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, void*) { return 0; }
inline HWND    CreateWindowA(const char* cls, const char* name, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE inst, void* p) { return CreateWindowExA(0, cls, name, style, x, y, w, h, parent, menu, inst, p); }
inline LRESULT DefWindowProcA(HWND, UINT, WPARAM, LPARAM) { return 0; }
#define RegisterClassEx RegisterClassExA
#define UnregisterClass UnregisterClassA
#define CreateWindowEx  CreateWindowExA
#define CreateWindow    CreateWindowA
#define DefWindowProc   DefWindowProcA

// ─── argv / command line ─────────────────────────────────────────────────────
#if defined(__APPLE__)
#  include <crt_externs.h>
#  define __argv (*_NSGetArgv())
#  define __argc (*_NSGetArgc())
#elif defined(__linux__)
   extern char** __argv;
   extern int    __argc;
#endif

// ─── MSVC integer type aliases ───────────────────────────────────────────────
// No __int64_t / __uint64_t here: those are glibc's own names, where they are
// `long` on LP64, and redefining them as `long long` is a hard error on Linux.
// Nothing in the tree used them.
#ifndef __int64
#  define __int64 long long
#endif

// ─── LARGE_INTEGER ────────────────────────────────────────────────────────────
union LARGE_INTEGER {
    struct { DWORD LowPart; LONG HighPart; } u;
    LONGLONG QuadPart;
};
inline BOOL QueryPerformanceCounter(LARGE_INTEGER* c) {
    using namespace std::chrono;
    c->QuadPart = (LONGLONG)duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
    return TRUE;
}
inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* f) {
    f->QuadPart = 1000000000LL; return TRUE;
}

// ─── Additional file / path API ───────────────────────────────────────────────
#include <libgen.h>

#define FILE_FLAG_RANDOM_ACCESS  0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000
#define FILE_FLAG_NO_BUFFERING   0x20000000
#define FILE_FLAG_WRITE_THROUGH  0x80000000

inline BOOL FlushFileBuffers(HANDLE h) { return fflush((FILE*)h) == 0; }

// Legacy global-heap allocators — the Win32 GlobalAlloc family maps to the C
// heap (flat memory model; the GMEM_* flags only matter under 16-bit segmented
// memory).
#define GMEM_FIXED    0x0000
#define GMEM_ZEROINIT 0x0040
#define GMEM_MOVEABLE 0x0002
inline void* GlobalAlloc(unsigned flags, size_t sz) {
    void* p = malloc(sz);
    if (p && (flags & GMEM_ZEROINIT)) memset(p, 0, sz);
    return p;
}
inline void* GlobalReAlloc(void* p, size_t sz, unsigned) { return realloc(p, sz); }
inline void* GlobalFree(void* p) { free(p); return 0; }

// Legacy OpenFile (16-bit-era API): the engine only uses it with OF_DELETE.
#define OF_DELETE 0x00000200
typedef struct _OFSTRUCT { unsigned char cBytes; } OFSTRUCT;
HFILE  OpenFile(const char* path, OFSTRUCT*, unsigned style);   // defined in WindowsAPI.cpp
HANDLE CreateFileA(const char* path, DWORD access, DWORD, void*, DWORD creation, DWORD, HANDLE);
#define CreateFile CreateFileA

// Templated on the byte-count and out-param types: Win32 uses DWORD (==unsigned
// long under LLP64), but callers like XStream use `unsigned long` for the
// out-param, which is 64-bit under LP64 and so won't bind to DWORD* here.
template<class CountT, class OutT>
inline BOOL ReadFile(HANDLE h, void* buf, CountT n, OutT* read, void*) {
    size_t r = fread(buf, 1, (size_t)n, (FILE*)h);
    if (read) *read = (OutT)r;
    return r > 0 || n == 0;
}
template<class CountT, class OutT>
inline BOOL WriteFile(HANDLE h, const void* buf, CountT n, OutT* written, void*) {
    size_t w = fwrite(buf, 1, (size_t)n, (FILE*)h);
    if (written) *written = (OutT)w;
    return w == (size_t)n;
}
inline BOOL CloseFileHandle(HANDLE h) { return fclose((FILE*)h) == 0; }

#define GetFileSize(h, high) ((DWORD)({ long p=ftell((FILE*)(h)); fseek((FILE*)(h),0,SEEK_END); long s=ftell((FILE*)(h)); fseek((FILE*)(h),p,SEEK_SET); if(high)*(DWORD*)(high)=0; s; }))

char* _fullpath(char* absPath, const char* relPath, size_t);   // defined in WindowsAPI.cpp
void  _splitpath(const char* path, char* drive, char* dir, char* fname, char* ext);

#define _makepath(path,d,dir,fn,ext) snprintf(path,MAX_PATH,"%s%s%s%s",(dir)?(dir):"",(fn)?(fn):"",(ext)?(ext):"","")

inline DWORD GetCurrentDirectoryA(DWORD n, char* buf) {
    return getcwd(buf, n) ? (DWORD)strlen(buf) : 0;
}
#define GetCurrentDirectory GetCurrentDirectoryA

#define strlwr _strlwr

// ─── WIN32 find file (directory iteration) ────────────────────────────────────
struct FILETIME { DWORD dwLowDateTime; DWORD dwHighDateTime; };

struct WIN32_FIND_DATAA {
    DWORD    dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD    nFileSizeHigh;
    DWORD    nFileSizeLow;
    DWORD    dwReserved0;
    DWORD    dwReserved1;
    char     cFileName[MAX_PATH];
    char     cAlternateFileName[14];
};
typedef WIN32_FIND_DATAA WIN32_FIND_DATA;

// Directory iteration — defined in WindowsAPI.cpp (the empty-pattern handling and
// case-correcting scan are non-trivial and shouldn't live in this app-wide header).
// The search state is a _FindContext, but callers only ever hold it as an opaque
// HANDLE, so its definition stays in the .cpp along with the <filesystem> it needs.
HANDLE FindFirstFileA(const char* rawPattern, WIN32_FIND_DATAA* fd);
#define FindFirstFile FindFirstFileA

BOOL FindNextFileA(HANDLE h, WIN32_FIND_DATAA* fd);
#define FindNextFile FindNextFileA

BOOL FindCloseHandle(HANDLE h);
#define FindClose FindCloseHandle

// ─── SYSTEMTIME ───────────────────────────────────────────────────────────────
struct SYSTEMTIME {
    WORD wYear, wMonth, wDayOfWeek, wDay;
    WORD wHour, wMinute, wSecond, wMilliseconds;
};
inline void GetLocalTime(SYSTEMTIME* st) {
    time_t t = time(nullptr); struct tm* tm = localtime(&t);
    st->wYear = tm->tm_year + 1900; st->wMonth = tm->tm_mon + 1;
    st->wDay = tm->tm_mday; st->wDayOfWeek = tm->tm_wday;
    st->wHour = tm->tm_hour; st->wMinute = tm->tm_min;
    st->wSecond = tm->tm_sec; st->wMilliseconds = 0;
}
inline void GetSystemTime(SYSTEMTIME* st) {
    time_t t = time(nullptr); struct tm* tm = gmtime(&t);
    st->wYear = tm->tm_year + 1900; st->wMonth = tm->tm_mon + 1;
    st->wDay = tm->tm_mday; st->wDayOfWeek = tm->tm_wday;
    st->wHour = tm->tm_hour; st->wMinute = tm->tm_min;
    st->wSecond = tm->tm_sec; st->wMilliseconds = 0;
}
#include <time.h>

// ─── File seek/time constants ─────────────────────────────────────────────────
#define FILE_BEGIN    0
#define FILE_CURRENT  1
#define FILE_END      2

inline DWORD SetFilePointer(HANDLE h, LONG dist, LONG* distHigh, DWORD method) {
    int whence = (method == FILE_BEGIN) ? SEEK_SET : (method == FILE_CURRENT) ? SEEK_CUR : SEEK_END;
    if (fseek((FILE*)h, dist, whence) != 0) return (DWORD)-1;
    return (DWORD)ftell((FILE*)h);
}

inline BOOL GetFileTime(HANDLE, FILETIME*, FILETIME*, FILETIME*) { return FALSE; }
inline BOOL FileTimeToDosDateTime(const FILETIME*, WORD* date, WORD* time_) {
    if (date) *date = 0; if (time_) *time_ = 0; return FALSE;
}

inline BOOL DeleteFileA(const char* path) { return remove(NormalizePath(path).c_str()) == 0; }
#define DeleteFile DeleteFileA

// ─── File time stubs ──────────────────────────────────────────────────────────
inline void GetSystemTimeAsFileTime(FILETIME* ft) {
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    // Convert POSIX time to Windows FILETIME (100ns intervals since 1601-01-01)
    uint64_t v = (uint64_t)ts.tv_sec * 10000000ULL + ts.tv_nsec / 100 + 116444736000000000ULL;
    ft->dwLowDateTime  = (DWORD)(v & 0xFFFFFFFF);
    ft->dwHighDateTime = (DWORD)(v >> 32);
}
inline BOOL DosDateTimeToFileTime(WORD /*date*/, WORD /*time_*/, FILETIME* ft) {
    if (ft) { ft->dwLowDateTime = 0; ft->dwHighDateTime = 0; } return FALSE;
}
inline LONG CompareFileTime(const FILETIME* a, const FILETIME* b) {
    if (a->dwHighDateTime != b->dwHighDateTime)
        return a->dwHighDateTime < b->dwHighDateTime ? -1 : 1;
    if (a->dwLowDateTime != b->dwLowDateTime)
        return a->dwLowDateTime < b->dwLowDateTime ? -1 : 1;
    return 0;
}
inline BOOL FileTimeToSystemTime(const FILETIME* ft, SYSTEMTIME* st) {
    uint64_t v = ((uint64_t)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
    if (v < 116444736000000000ULL) { memset(st, 0, sizeof(*st)); return FALSE; }
    time_t t = (time_t)((v - 116444736000000000ULL) / 10000000ULL);
    struct tm* tm = gmtime(&t);
    if (!tm) { memset(st, 0, sizeof(*st)); return FALSE; }
    st->wYear = tm->tm_year + 1900; st->wMonth = tm->tm_mon + 1;
    st->wDay = tm->tm_mday; st->wDayOfWeek = tm->tm_wday;
    st->wHour = tm->tm_hour; st->wMinute = tm->tm_min;
    st->wSecond = tm->tm_sec; st->wMilliseconds = 0;
    return TRUE;
}
inline BOOL SystemTimeToTzSpecificLocalTime(void* /*tz*/, const SYSTEMTIME* src, SYSTEMTIME* dst) {
    if (dst) *dst = *src; return TRUE;
}

// ─── Directory / file operations ──────────────────────────────────────────────
#include <sys/stat.h>
#include <unistd.h>
inline BOOL CreateDirectoryA(const char* path, void*) {
    return mkdir(NormalizePath(path).c_str(), 0755) == 0 ? TRUE : FALSE;
}
#ifndef CreateDirectory
#  define CreateDirectory CreateDirectoryA
#endif
inline BOOL RemoveDirectoryA(const char* path) { return rmdir(NormalizePath(path).c_str()) == 0 ? TRUE : FALSE; }
#ifndef RemoveDirectory
#  define RemoveDirectory RemoveDirectoryA
#endif
inline BOOL SetCurrentDirectoryA(const char* path) { return chdir(NormalizePath(path).c_str()) == 0 ? TRUE : FALSE; }
#ifndef SetCurrentDirectory
#  define SetCurrentDirectory SetCurrentDirectoryA
#endif

// ─── String utilities ─────────────────────────────────────────────────────────
#include <cctype>
inline char* strupr(char* s) {
    for (char* p = s; *p; ++p) *p = (char)toupper((unsigned char)*p);
    return s;
}

// ─── COM / GUID generation ────────────────────────────────────────────────────
inline HRESULT CoCreateGuid(GUID* g) {
    if (!g) return E_POINTER;
    FILE* f = fopen("/dev/urandom", "rb");
    if (f) { fread(g, 1, sizeof(GUID), f); fclose(f); }
    else    memset(g, 0, sizeof(GUID));
    return S_OK;
}
#define objbase_h  // prevent <objbase.h> include

// ─── Pragma warning (ignore MSVC-specific pragmas) ───────────────────────────
#define PRAGMA_WARNING_PUSH
#define PRAGMA_WARNING_POP

// ─── Assert-like macro used in some modules ──────────────────────────────────
#ifndef MDebugBreak
#  define MDebugBreak() __builtin_trap()
#endif

// ─── xassert (project-wide assertion) ────────────────────────────────────────
#include <cassert>
#ifndef xassert
#  define xassert(e) assert(e)
#endif

// ─── MSVC function-signature predefined macro ─────────────────────────────────
#ifndef __FUNCSIG__
#  define __FUNCSIG__ __PRETTY_FUNCTION__
#endif

// ─── MessageBox constants and stub ───────────────────────────────────────────
#define MB_OK                0x00000000
#define MB_OKCANCEL          0x00000001
#define MB_YESNO             0x00000004
#define MB_RETRYCANCEL       0x00000005
#define MB_ICONHAND          0x00000010
#define MB_ICONERROR         0x00000010
#define MB_ICONQUESTION      0x00000020
#define MB_ICONEXCLAMATION   0x00000030
#define MB_ICONWARNING       0x00000030
#define MB_ICONINFORMATION   0x00000040
#define MB_ABORTRETRYIGNORE  0x00000002
#define MB_ICONSTOP          0x00000010
#define MB_TOPMOST           0x00040000
#define SW_MINIMIZE          6
#define IDOK     1
#define IDCANCEL 2
#define IDABORT  3
#define IDRETRY  4
#define IDIGNORE 5
#define IDYES    6
#define IDNO     7
inline int MessageBoxA(HWND, const char*, const char*, UINT) { return IDOK; }
inline int MessageBoxW(HWND, const wchar_t*, const wchar_t*, UINT) { return IDOK; }
#define MessageBox MessageBoxA

// ─── Additional window messages ───────────────────────────────────────────────
#define WM_USER  0x0400
#define WM_APP   0x8000

// ─── Code page constants ──────────────────────────────────────────────────────
#define CP_ACP   0
#define CP_OEMCP 1
#define CP_UTF8  65001

// ─── Unicode conversion ───────────────────────────────────────────────────────
// Faithful (enough) reimplementation of the Win32 MultiByteToWideChar /
// WideCharToMultiByte contract that the engine's a2w/w2a helpers and the
// serialization archives depend on. wchar_t is 32-bit here, so "wide" means
// UTF-32 code points. Contract honoured:
//   * dst == null  → query: return the number of wide/narrow units that would
//                    be produced (NOT written), so a follow-up convert call
//                    returns the same value.
//   * srcLen >= 0  → consume exactly that many source units, count excludes the
//                    terminator (none is appended unless room and requested).
//   * srcLen <  0  → source is null-terminated; the terminator is converted and
//                    INCLUDED in the returned count (matches Win32).
// Only CP_UTF8 gets real multibyte decoding; every other code page (CP_ACP,
// 1251/1252, …) is treated as a single-byte/Latin-1 passthrough — correct for
// ASCII, which is all the non-UTF-8 callers here actually need.
// Bodies live in WindowsAPI.cpp so edits don't rebuild the whole project.
#define MB_PRECOMPOSED 0x00000001
int MultiByteToWideChar(UINT codePage, DWORD, const char* src, int srcLen, wchar_t* dst, int dstLen);
int WideCharToMultiByte(UINT codePage, DWORD, const wchar_t* src, int srcLen, char* dst, int dstLen, const char*, BOOL*);

// xassert() routes through DiagAssert(), which lives in the Win32 crash handler
// (XERRHAND) and has no counterpart here. With NASSERT defined, xutil.h takes the
// branch where the xassert macros expand to nothing.
#ifndef NASSERT
#define NASSERT
#endif

#include <cmath>

// ─── Color extraction macros (Windows COLORREF helpers) ───────────────────────
#define GetRValue(rgb)  ((BYTE)(rgb))
#define GetGValue(rgb)  ((BYTE)((WORD)(rgb) >> 8))
#define GetBValue(rgb)  ((BYTE)((rgb) >> 16))

// ─── Virtual key codes ────────────────────────────────────────────────────────
#define VK_LBUTTON   0x01
#define VK_RBUTTON   0x02
#define VK_CANCEL    0x03
#define VK_MBUTTON   0x04
#define VK_BACK      0x08
#define VK_TAB       0x09
#define VK_RETURN    0x0D
#define VK_SHIFT     0x10
#define VK_CONTROL   0x11
#define VK_MENU      0x12
#define VK_PAUSE     0x13
#define VK_CAPITAL   0x14
#define VK_ESCAPE    0x1B
#define VK_SNAPSHOT  0x2C
#define VK_SCROLL    0x91
#define VK_NUMLOCK   0x90
#define VK_LWIN      0x5B
#define VK_RWIN      0x5C
#define VK_APPS      0x5D
#define VK_SPACE     0x20
#define VK_PRIOR     0x21
#define VK_NEXT      0x22
#define VK_END       0x23
#define VK_HOME      0x24
#define VK_LEFT      0x25
#define VK_UP        0x26
#define VK_RIGHT     0x27
#define VK_DOWN      0x28
#define VK_INSERT    0x2D
#define VK_DELETE    0x2E
#define VK_F1        0x70
#define VK_F2        0x71
#define VK_F3        0x72
#define VK_F4        0x73
#define VK_F5        0x74
#define VK_F6        0x75
#define VK_F7        0x76
#define VK_F8        0x77
#define VK_F9        0x78
#define VK_F10       0x79
#define VK_F11       0x7A
#define VK_F12       0x7B
#define VK_NUMPAD0   0x60
#define VK_NUMPAD2   0x62
#define VK_NUMPAD4   0x64
#define VK_NUMPAD6   0x66
#define VK_NUMPAD8   0x68
#define VK_NUMPAD9   0x69
#define VK_MULTIPLY  0x6A
#define VK_ADD       0x6B
#define VK_SUBTRACT  0x6D
#define VK_DECIMAL   0x6E
#define VK_DIVIDE    0x6F

// Polled key state. This is the engine's ONLY source of held-key input: camera
// pan/rotate/zoom poll it every frame (ControlManager::key(...).pressed() ->
// isPressed(), Util/SystemUtil.h), and addModifiersState() folds the Ctrl/Shift/
// Alt bits it reports into every click and keypress. The state is fed by the SDL
// event pump (PlatformWindow::pumpEvents) rather than queried from SDL here, so
// it stays valid when polled off the main thread. Bodies live in WindowsAPI.cpp.
SHORT GetAsyncKeyState(int vk);

// Called by the event pump: record a key/mouse-button transition (vk is a VK_*
// code), and drop every key on focus loss so nothing sticks down while we are in
// the background and SDL is not delivering key-ups.
void PlatformSetKeyState(int vk, bool down);
void PlatformClearKeyStates();

// Polled cursor position, the mouse's counterpart to GetAsyncKeyState above and fed
// the same way -- by the SDL event pump, rather than queried from SDL on the spot.
// The callers that poll rather than wait for WM_MOUSEMOVE are the modal
// loops that run their own frame (ReelManager::showLogoModal, whose metaballs and
// fish both track the cursor); a stub returning (0,0) leaves them pinned to the top
// left corner. SDL reports motion in WINDOW coordinates, and ScreenToClient below
// is the identity, so this is client-relative -- which is what every caller wants.
void PlatformSetMousePosition(int x, int y);

// PeekMessage flag, LoadImage flags (winuser.h).
#define PM_REMOVE       0x0001
#define IMAGE_CURSOR    2
#define LR_LOADFROMFILE 0x0010

// Common-control notification header (commctrl.h) — used by value in editor
// UI callbacks (onNotify); only its presence is needed for the headers to parse.
struct NMHDR { HWND hwndFrom; unsigned long idFrom; UINT code; };

// Window/cursor helpers (winuser.h). No-op on non-Windows; real windowing is the
// eventual SDL3 replacement (Track B).
inline BOOL ScreenToClient(HWND, POINT*) { return TRUE; }
inline BOOL ClientToScreen(HWND, POINT*) { return TRUE; }
BOOL GetCursorPos(POINT* p);   // see PlatformSetMousePosition above
inline BOOL DestroyCursor(HCURSOR) { return TRUE; }
inline int  ShowCursor(BOOL) { return 0; }
inline BOOL ShowWindow(HWND, int) { return TRUE; }

// GetSystemMetrics indices (winuser.h) + a stub returning sane desktop defaults.
#define SM_CXSCREEN     0
#define SM_CYSCREEN     1
#define SM_CYCAPTION    4
#define SM_CXSIZEFRAME  32
#define SM_CYSIZEFRAME  33
#define SM_CXICON       11
#define SM_CYICON       12
#define SM_CXSMICON     49
#define SM_CYSMICON     50
inline int GetSystemMetrics(int index) {
    switch(index) { case SM_CXSCREEN: return 1920; case SM_CYSCREEN: return 1080;
                    case SM_CYCAPTION: return 24; case SM_CXSIZEFRAME: case SM_CYSIZEFRAME: return 4; }
    return 0;
}
inline BOOL SetCursorPos(int, int) { return TRUE; }
inline HCURSOR SetCursor(HCURSOR) { return 0; }
inline HANDLE  LoadImageA(HINSTANCE, const char*, UINT, int, int, UINT) { return 0; }
#define LoadImage LoadImageA

// MSVC CRT extension: checks if c is a valid C identifier character
inline int __iscsym(int c) { return (c >= 0 && c <= 127) && (isalnum(c) || c == '_'); }

// MSVC wide-string conversions (CRT extensions).
inline int    _wtoi(const wchar_t* s) { return (int)wcstol(s, nullptr, 10); }
inline double _wtof(const wchar_t* s) { return wcstod(s, nullptr); }

// MSVC's swprintf has no buffer-size parameter (swprintf(buf, fmt, ...)), unlike
// the C standard's swprintf(buf, n, fmt, ...). This array overload recovers the
// size from the destination buffer and forwards to the standard form. It only
// matches array arguments, so correctly-sized calls still pick the std version.
template<size_t N, class... A>
inline int swprintf(wchar_t (&buf)[N], const wchar_t* fmt, A... args) {
    return std::swprintf(buf, N, fmt, args...);
}

#endif // !_WIN32
