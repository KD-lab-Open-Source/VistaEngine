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
#include <cstdio>
#include <cstdlib>
#include <strings.h>

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
    return f ? (HANDLE)f : INVALID_HANDLE_VALUE;
}

char* _fullpath(char* absPath, const char* relPath, size_t) {
    return realpath(NormalizePath(relPath).c_str(), absPath);
}

void _splitpath(const char* path, char* drive, char* dir, char* fname, char* ext) {
    if (drive) drive[0] = '\0';
    char tmp[MAX_PATH]; strncpy(tmp, path, MAX_PATH-1); tmp[MAX_PATH-1]='\0';
    if (dir)   { char* d = dirname(tmp);  strncpy(dir, d, MAX_PATH); strncat(dir, "/", MAX_PATH); }
    strncpy(tmp, path, MAX_PATH-1);
    if (fname) { char* b = basename(tmp); char* dot = strrchr(b, '.'); size_t n = dot ? (size_t)(dot-b) : strlen(b); strncpy(fname, b, n); fname[n] = '\0'; }
    strncpy(tmp, path, MAX_PATH-1);
    if (ext)   { char* b = basename(tmp); char* dot = strrchr(b, '.'); strncpy(ext, dot ? dot : "", MAX_PATH); }
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
