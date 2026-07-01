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
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cwchar>
#include <cstdio>
#include <cstdlib>
#include <strings.h>
#include <unordered_set>
#include <mutex>

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
// "wide" means UTF-32 code points. Only CP_UTF8 gets real multibyte decoding;
// every other code page is a single-byte/Latin-1 passthrough (ASCII-correct).
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
        while (p < end) {
            if (dst) { if (produced >= dstLen) break; dst[produced] = (wchar_t)*p; }
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
        for (size_t i = 0; i < srcCount; ++i) {
            unsigned int cp = (unsigned int)src[i];
            if (dst) { if (produced >= dstLen) break; dst[produced] = cp < 0x100 ? (char)cp : '?'; }
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
                const char* dirToScan = out.empty() ? "." : out.c_str();
                if (DIR* d = ::opendir(dirToScan)) {
                    for (struct dirent* ent; (ent = ::readdir(d)) != nullptr; ) {
                        if (::strcasecmp(ent->d_name, comp.c_str()) == 0) {
                            matched = ent->d_name;
                            break;
                        }
                    }
                    ::closedir(d);
                }
            }
        }
        if (!out.empty() && out.back() != '/') out += '/';
        out += matched;
    }
    return out;
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
    if (drive) drive[0] = '\0';
    char tmp[MAX_PATH];
    if (dir)   { strncpy(tmp, path, MAX_PATH-1); tmp[MAX_PATH-1]='\0';
                 char* d = dirname(tmp);
                 strncpy(dir, d, _MAX_DIR-1); dir[_MAX_DIR-1]='\0';
                 strncat(dir, "/", _MAX_DIR - strlen(dir) - 1); }
    if (fname) { strncpy(tmp, path, MAX_PATH-1); tmp[MAX_PATH-1]='\0';
                 char* b = basename(tmp); char* dot = strrchr(b, '.');
                 size_t n = dot ? (size_t)(dot-b) : strlen(b);
                 if (n > _MAX_FNAME-1) n = _MAX_FNAME-1;
                 strncpy(fname, b, n); fname[n] = '\0'; }
    if (ext)   { strncpy(tmp, path, MAX_PATH-1); tmp[MAX_PATH-1]='\0';
                 char* b = basename(tmp); char* dot = strrchr(b, '.');
                 strncpy(ext, dot ? dot : "", _MAX_EXT-1); ext[_MAX_EXT-1]='\0'; }
}

HANDLE FindFirstFileA(const char* rawPattern, WIN32_FIND_DATAA* fd) {
    // Windows returns INVALID_HANDLE_VALUE for an empty pattern; callers rely on
    // this (e.g. DirIterator's default-constructed `end` sentinel passes "").
    // Don't fall through to the no-slash "current directory" branch below, which
    // would open "." and hand back a valid handle, breaking such sentinels.
    if (!rawPattern || !*rawPattern) return INVALID_HANDLE_VALUE;
    std::string pattern_s = NormalizePath(rawPattern);
    const char* pattern = pattern_s.c_str();
    if (!*pattern) return INVALID_HANDLE_VALUE;
    char dir_path[MAX_PATH]; strncpy(dir_path, pattern, MAX_PATH-1);
    char* slash = strrchr(dir_path, '/'); if (!slash) slash = strrchr(dir_path, '\\');
    if (slash) *slash = '\0'; else { dir_path[0]='.'; dir_path[1]='\0'; }
    DIR* d = opendir(dir_path);
    if (!d) return INVALID_HANDLE_VALUE;
    auto* ctx = new _FindContext; ctx->dir = d;
    strncpy(ctx->path, dir_path, MAX_PATH-1);
    struct dirent* ent;
    while ((ent = readdir(d)) != nullptr) {
        if (ent->d_name[0] == '.') continue;
        strncpy(fd->cFileName, ent->d_name, MAX_PATH-1);
        fd->dwFileAttributes = (ent->d_type == DT_DIR) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        fd->nFileSizeHigh = fd->nFileSizeLow = 0;
        return (HANDLE)ctx;
    }
    closedir(d); delete ctx; return INVALID_HANDLE_VALUE;
}

BOOL FindNextFileA(HANDLE h, WIN32_FIND_DATAA* fd) {
    if (h == INVALID_HANDLE_VALUE) return FALSE;
    auto* ctx = (_FindContext*)h;
    struct dirent* ent;
    while ((ent = readdir(ctx->dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;
        strncpy(fd->cFileName, ent->d_name, MAX_PATH-1);
        fd->dwFileAttributes = (ent->d_type == DT_DIR) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        fd->nFileSizeHigh = fd->nFileSizeLow = 0;
        return TRUE;
    }
    return FALSE;
}

BOOL FindCloseHandle(HANDLE h) {
    if (h == INVALID_HANDLE_VALUE) return FALSE;
    auto* ctx = (_FindContext*)h;
    closedir(ctx->dir); delete ctx; return TRUE;
}

#endif // !_WIN32
