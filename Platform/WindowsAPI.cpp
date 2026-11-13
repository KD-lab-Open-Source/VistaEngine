// Out-of-line implementations for the non-Windows Win32 shim.
//
// WindowsAPI.h is force-included (-include) into every C++ translation unit, so a
// body kept there makes every edit trigger a full project rebuild. The non-trivial
// path / directory-iteration helpers therefore live here: editing them recompiles
// only this file and relinks. The header keeps the declarations (and the small,
// stable inline shims).
//
// On Windows the real headers are used and this file is empty.

#include "Platform/WindowsAPI.h"

#ifndef _WIN32

#include <string>
#include <filesystem>
#include <system_error>
#include <fnmatch.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cwchar>
#include <cstdio>
#include <cstdlib>
#include <strings.h>
#include <unordered_set>
#include <mutex>
#include <atomic>

// ─── Polled key state ─────────────────────────────────────────────────────────
// Written by the SDL event pump on the main thread, read by isPressed() from the
// graphics thread, so the slots are atomic. Indexed by VK_* code.
namespace {
std::atomic<bool> g_keyState[256];
}

void PlatformSetKeyState(int vk, bool down)
{
	if(vk > 0 && vk < 256)
		g_keyState[vk].store(down, std::memory_order_relaxed);
}

void PlatformClearKeyStates()
{
	for(std::atomic<bool>& key : g_keyState)
		key.store(false, std::memory_order_relaxed);
}

SHORT GetAsyncKeyState(int vk)
{
	if(vk <= 0 || vk >= 256)
		return 0;
	// Win32 reports "currently down" in the high bit; isPressed() masks 0x8000.
	return g_keyState[vk].load(std::memory_order_relaxed) ? SHORT(0x8000) : SHORT(0);
}

// ─── Polled cursor position ───────────────────────────────────────────────────
// Written by the SDL event pump, read by GetCursorPos.
//
// Everything in this file runs on ONE thread: _beginthread is a no-op off Windows
// (Platform/fake_includes/process.h) and Runtime::GameStart forces the synchronous
// load path there, so the logic/graphics split never exists. These could be plain
// ints. They are atomic to match g_keyState above -- the same kind of latch, fed
// from the same place -- and relaxed atomics compile to the same load and store, so
// the consistency costs nothing. Note it is per-slot, not per-pair: a reader can
// still take x from one event and y from the next, which for a cursor is harmless.
//
// Window coordinates, since ScreenToClient is the identity here.
namespace {
std::atomic<int> g_mouseX{0};
std::atomic<int> g_mouseY{0};
}

void PlatformSetMousePosition(int x, int y)
{
	g_mouseX.store(x, std::memory_order_relaxed);
	g_mouseY.store(y, std::memory_order_relaxed);
}

BOOL GetCursorPos(POINT* p)
{
	if(!p)
		return FALSE;
	p->x = g_mouseX.load(std::memory_order_relaxed);
	p->y = g_mouseY.load(std::memory_order_relaxed);
	return TRUE;
}

// CreateFileA returns a FILE* (cast to HANDLE). CloseHandle must fclose those, but
// it is also called on event/thread handles (CreateEvent/_beginthread) that are NOT
// FILE*. Track the file handles we hand out so CloseHandle closes only those and is
// a harmless no-op for everything else. Without this every CreateFile leaks an fd
// (isFileExists, terrain VMAP, ...), which exhausts the 256-fd limit fast.
static std::unordered_set<void*>& fileHandleSet() { static std::unordered_set<void*> s; return s; }
static std::mutex&                fileHandleMutex() { static std::mutex m; return m; }

static void registerFileHandle(void* h)
{
    std::lock_guard<std::mutex> lk(fileHandleMutex());
    fileHandleSet().insert(h);
}

BOOL CloseHandle(HANDLE h)
{
    if(!h || h == INVALID_HANDLE_VALUE) return FALSE;
    std::lock_guard<std::mutex> lk(fileHandleMutex());
    auto it = fileHandleSet().find(h);
    if(it != fileHandleSet().end()){
        fclose((FILE*)h);
        fileHandleSet().erase(it);
    }
    return TRUE;
}

// ─── Unicode conversion ───────────────────────────────────────────────────────
// See WindowsAPI.h for the honoured Win32 contract. wchar_t is 32-bit here, so
// "wide" means UTF-32 code points. CP_UTF8 is decoded as UTF-8; 1251 and 1252 by
// their tables below; every other code page, CP_ACP among them, is a
// single-byte/Latin-1 passthrough (ASCII-correct).

namespace {

// Bytes 0x80-0xFF of Windows-1251 (Cyrillic) and Windows-1252 (Western), the two code
// pages the fonts are built from (Render/src/FT_Font.cpp, BitmapFont.cpp): a font's
// character table is every byte of the page, widened here. Below 0x80 both are ASCII.
// Generated from Python's cp1251/cp1252 codecs; the few bytes a page leaves undefined
// map to the C1 control of the same value, as Windows maps them.
const char16_t cp1251High[128] = {
	0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
	0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
	0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
	0x0098, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
	0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
	0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
	0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
	0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
	0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
	0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
	0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
	0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
	0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
	0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
	0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
	0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
};

const char16_t cp1252High[128] = {
	0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
	0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
	0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
	0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178,
	0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
	0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
	0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
	0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
	0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
	0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
	0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
	0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
	0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
	0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
	0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
	0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF,
};

// The table for 0x80-0xFF of `codePage`, or null for a Latin-1 passthrough.
const char16_t* highHalf(UINT codePage)
{
	switch (codePage) {
	case 1251: return cp1251High;
	case 1252: return cp1252High;
	default:   return nullptr;
	}
}

// A code point's byte in a single-byte code page: '?' when the page cannot spell it,
// which is what Win32 substitutes too.
char toSingleByte(const char16_t* high, unsigned int cp)
{
	if (cp < 0x80)
		return (char)cp;
	if (!high)
		return cp < 0x100 ? (char)cp : '?';
	for (int i = 0; i < 128; ++i)
		if (high[i] == cp)
			return (char)(0x80 + i);
	return '?';
}

} // namespace

int MultiByteToWideChar(UINT codePage, DWORD, const char* src, int srcLen, wchar_t* dst, int dstLen) {
    size_t srcBytes = srcLen < 0 ? strlen(src) : (size_t)srcLen;
    const unsigned char* p = (const unsigned char*)src;
    const unsigned char* end = p + srcBytes;
    int produced = 0;
    if (codePage == CP_UTF8) {
        while (p < end) {
            unsigned char c = *p++;
            unsigned int cp; int extra;
            if (c < 0x80)      { cp = c;        extra = 0; }
            else if (c < 0xE0) { cp = c & 0x1F; extra = 1; }
            else if (c < 0xF0) { cp = c & 0x0F; extra = 2; }
            else               { cp = c & 0x07; extra = 3; }
            while (extra-- > 0 && p < end && (*p & 0xC0) == 0x80)
                cp = (cp << 6) | (*p++ & 0x3F);
            if (dst) { if (produced >= dstLen) break; dst[produced] = (wchar_t)cp; }
            ++produced;
        }
    } else {
        const char16_t* high = highHalf(codePage);
        while (p < end) {
            unsigned char c = *p;
            if (dst) { if (produced >= dstLen) break; dst[produced] = (wchar_t)(c >= 0x80 && high ? high[c - 0x80] : c); }
            ++p; ++produced;
        }
    }
    if (srcLen < 0) { // include the terminator, like Win32
        if (dst && produced < dstLen) dst[produced] = 0;
        ++produced;
    }
    return produced;
}

int WideCharToMultiByte(UINT codePage, DWORD, const wchar_t* src, int srcLen, char* dst, int dstLen, const char*, BOOL*) {
    size_t srcCount = srcLen < 0 ? wcslen(src) : (size_t)srcLen;
    int produced = 0;
    if (codePage == CP_UTF8) {
        for (size_t i = 0; i < srcCount; ++i) {
            unsigned int cp = (unsigned int)src[i];
            char tmp[4]; int n;
            if (cp < 0x80)        { tmp[0] = (char)cp; n = 1; }
            else if (cp < 0x800)  { tmp[0] = (char)(0xC0 | (cp >> 6));  tmp[1] = (char)(0x80 | (cp & 0x3F)); n = 2; }
            else if (cp < 0x10000){ tmp[0] = (char)(0xE0 | (cp >> 12)); tmp[1] = (char)(0x80 | ((cp >> 6) & 0x3F)); tmp[2] = (char)(0x80 | (cp & 0x3F)); n = 3; }
            else                  { tmp[0] = (char)(0xF0 | (cp >> 18)); tmp[1] = (char)(0x80 | ((cp >> 12) & 0x3F)); tmp[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); tmp[3] = (char)(0x80 | (cp & 0x3F)); n = 4; }
            if (dst) { if (produced + n > dstLen) break; for (int k = 0; k < n; ++k) dst[produced + k] = tmp[k]; }
            produced += n;
        }
    } else {
        const char16_t* high = highHalf(codePage);
        for (size_t i = 0; i < srcCount; ++i) {
            if (dst) { if (produced >= dstLen) break; dst[produced] = toSingleByte(high, (unsigned int)src[i]); }
            ++produced;
        }
    }
    if (srcLen < 0) { // include the terminator, like Win32
        if (dst && produced < dstLen) dst[produced] = 0;
        ++produced;
    }
    return produced;
}

std::string NormalizePath(const char* path) {
    if (!path || !*path) return std::string(path ? path : "");

    std::string in(path);
    for (char& c : in) if (c == '\\') c = '/';

    std::string out = (in[0] == '/') ? "/" : "";  // preserve absolute root

    size_t pos = 0;
    while (pos < in.size()) {
        while (pos < in.size() && in[pos] == '/') ++pos;   // skip separators
        if (pos >= in.size()) break;
        size_t end = in.find('/', pos);
        if (end == std::string::npos) end = in.size();
        std::string comp = in.substr(pos, end - pos);
        pos = end;

        std::string matched = comp;  // default: keep verbatim
        if (comp != "." && comp != "..") {
            std::string candidate = out;
            if (!candidate.empty() && candidate.back() != '/') candidate += '/';
            candidate += comp;
            struct stat st;
            if (::stat(candidate.c_str(), &st) != 0) {
                // No exact match — scan the parent for a case-insensitive one.
                const std::string dirToScan = out.empty() ? "." : out;
                std::error_code ec;
                for (const auto& entry : std::filesystem::directory_iterator(dirToScan, ec)) {
                    const std::string name = entry.path().filename().string();
                    if (::strcasecmp(name.c_str(), comp.c_str()) == 0) {
                        matched = name;
                        break;
                    }
                }
            }
        }
        if (!out.empty() && out.back() != '/') out += '/';
        out += matched;
    }
    return out;
}

// Locate the user's "My Documents" base directory (see fake_includes/shlobj.h).
// The engine appends "\My Games\<savePath>\" and creates profile/save folders
// there. Returning an empty base would make NormalizePath root that path at "/",
// which is unwritable and silently breaks all profile scan/create — leaving the
// first-launch menu permanently stuck on the profile screen.
HRESULT SHGetFolderPathA(HWND, int, HANDLE, DWORD, char* path) {
    if (!path) return E_FAIL;
    const char* home = getenv("HOME");
    if (!home || !*home) home = ".";
    std::strncpy(path, home, MAX_PATH - 1);
    path[MAX_PATH - 1] = '\0';
    return S_OK;
}

HFILE OpenFile(const char* path, OFSTRUCT*, unsigned style) {
    if (style & OF_DELETE) remove(NormalizePath(path).c_str());
    return 0;
}

HANDLE CreateFileA(const char* path, DWORD access, DWORD, void*, DWORD creation, DWORD, HANDLE) {
    const char* mode = (access & GENERIC_WRITE) ? "r+b" : "rb";
    if (creation == CREATE_ALWAYS) mode = "w+b";
    else if (creation == CREATE_NEW) mode = "w+bx";
    FILE* f = fopen(NormalizePath(path).c_str(), mode);
    if(!f) return INVALID_HANDLE_VALUE;
    registerFileHandle(f);   // so CloseHandle actually fcloses it
    return (HANDLE)f;
}

char* _fullpath(char* absPath, const char* relPath, size_t) {
    return realpath(NormalizePath(relPath).c_str(), absPath);
}

void _splitpath(const char* path, char* drive, char* dir, char* fname, char* ext) {
    // The Windows contract sizes the caller's buffers as _MAX_DRIVE/_MAX_DIR/
    // _MAX_FNAME/_MAX_EXT — NOT MAX_PATH. strncpy zero-pads to its full count, so
    // copying MAX_PATH bytes into a _MAX_EXT(256)-byte buffer smashed the stack.
    //
    // Both '\\' and '/' separate, as they do for the Windows CRT: the engine's data
    // paths are backslashed ("Resource\Models\foo.3dx"), so POSIX dirname/basename —
    // which only know '/' — used to report the whole path as the basename and "." as
    // the directory. That silently emptied extractFilePath(), and every model texture
    // resolved to ".\Textures\foo.tga" instead of "Resource\Models\Textures\foo.tga".
    if (drive) drive[0] = '\0';

    const char* lastSlash = nullptr;
    for (const char* p = path; *p; ++p)
        if (*p == '/' || *p == '\\')
            lastSlash = p;

    // The name starts after the last separator; the directory (if any) keeps it,
    // matching _splitpath's "dir includes the trailing slash, else empty".
    const char* base = lastSlash ? lastSlash + 1 : path;
    const char* dot = strrchr(base, '.');

    if (dir) {
        size_t n = lastSlash ? (size_t)(lastSlash + 1 - path) : 0;
        if (n > _MAX_DIR-1) n = _MAX_DIR-1;
        memcpy(dir, path, n); dir[n] = '\0';
    }
    if (fname) {
        size_t n = dot ? (size_t)(dot - base) : strlen(base);
        if (n > _MAX_FNAME-1) n = _MAX_FNAME-1;
        memcpy(fname, base, n); fname[n] = '\0';
    }
    if (ext) {
        strncpy(ext, dot ? dot : "", _MAX_EXT-1); ext[_MAX_EXT-1]='\0';
    }
}

// Advance ctx->dir to the next entry matching ctx->pattern, filling `fd`. The Win32
// wildcard is honoured (case-insensitive, matching the Windows FS): the original shim
// discarded the mask and returned every entry with size 0. That broke callers that
// filter by extension (DirIterator's "*.tga" etc.) and, worse, callers that gate on
// nFileSizeLow -- MissionDescriptions::readFromDir skips any entry with size 0, so the
// scenario mission list came back empty off-Windows (no campaign missions -> the
// mission-select screen has nothing to select -> the Start button never appears).
// "", "*" and "*.*" mean "everything" (classic Win32 semantics, incl. extensionless).
// The search state behind the HANDLE that FindFirstFile returns. It lives here rather
// than in the header so that <filesystem> — and, when this was POSIX dirent, the DT_*
// names that clash with the engine's own — stay out of every translation unit.
struct _FindContext {
    std::filesystem::directory_iterator it;
    std::string pattern;
};

static bool findNextMatch(_FindContext* ctx, WIN32_FIND_DATAA* fd) {
    const bool matchAll = ctx->pattern.empty() ||
                          ctx->pattern == "*" || ctx->pattern == "*.*";
    const std::filesystem::directory_iterator end;
    for (; ctx->it != end; ++ctx->it) {
        const std::string name = ctx->it->path().filename().string();
        if (name.empty() || name[0] == '.') continue;   // skip ".", "..", dotfiles (as before)
        if (!matchAll && fnmatch(ctx->pattern.c_str(), name.c_str(), FNM_CASEFOLD) != 0) continue;

        strncpy(fd->cFileName, name.c_str(), MAX_PATH-1); fd->cFileName[MAX_PATH-1] = '\0';

        std::error_code ec;
        const bool isDir = ctx->it->is_directory(ec);
        fd->dwFileAttributes = isDir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        // Report the real size: nFileSizeLow-gated callers (MissionDescriptions::
        // readFromDir) skip any entry of size 0, which once emptied the mission list.
        const uintmax_t size = isDir ? 0 : ctx->it->file_size(ec);
        fd->nFileSizeLow  = ec ? 0 : (DWORD)(size & 0xffffffff);
        fd->nFileSizeHigh = ec ? 0 : (DWORD)(size >> 32);

        ++ctx->it;      // leave the iterator on the next candidate for FindNextFile
        return true;
    }
    return false;
}

HANDLE FindFirstFileA(const char* rawPattern, WIN32_FIND_DATAA* fd) {
    // Windows returns INVALID_HANDLE_VALUE for an empty pattern; callers rely on
    // this (e.g. DirIterator's default-constructed `end` sentinel passes "").
    if (!rawPattern || !*rawPattern) return INVALID_HANDLE_VALUE;
    std::string pattern_s = NormalizePath(rawPattern);
    if (pattern_s.empty()) return INVALID_HANDLE_VALUE;

    // Split "<dir>/<mask>" at the last separator; no separator => current dir.
    std::string dir_s, mask_s;
    size_t sl = pattern_s.find_last_of("/\\");
    if (sl == std::string::npos) { dir_s = "."; mask_s = pattern_s; }
    else { dir_s = pattern_s.substr(0, sl); if (dir_s.empty()) dir_s = "/"; mask_s = pattern_s.substr(sl + 1); }

    // The error_code overload: a missing or unreadable directory must return
    // INVALID_HANDLE_VALUE, not throw — callers probe with paths that may not exist.
    std::error_code ec;
    std::filesystem::directory_iterator it(dir_s, ec);
    if (ec) return INVALID_HANDLE_VALUE;

    auto* ctx = new _FindContext{ it, mask_s };
    if (findNextMatch(ctx, fd)) return (HANDLE)ctx;
    delete ctx; return INVALID_HANDLE_VALUE;
}

BOOL FindNextFileA(HANDLE h, WIN32_FIND_DATAA* fd) {
    if (h == INVALID_HANDLE_VALUE) return FALSE;
    return findNextMatch((_FindContext*)h, fd) ? TRUE : FALSE;
}

BOOL FindCloseHandle(HANDLE h) {
    if (h == INVALID_HANDLE_VALUE) return FALSE;
    delete (_FindContext*)h;
    return TRUE;
}

#endif // !_WIN32
