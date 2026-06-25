#pragma once
// MSVC <io.h> — POSIX file I/O wrappers
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_RDWR   O_RDWR
#define _O_CREAT  O_CREAT
#define _O_TRUNC  O_TRUNC
#define _O_BINARY 0
#define _O_TEXT   0
#define _S_IREAD  S_IRUSR
#define _S_IWRITE S_IWUSR

#define _open   open
#define _read   read
#define _write  write
#define _close  close
#define _lseek  lseek
#define _eof(fd) (lseek(fd, 0, SEEK_CUR) >= lseek(fd, 0, SEEK_END))

inline long _filelength(int fd) {
    off_t cur = lseek(fd, 0, SEEK_CUR);
    off_t end = lseek(fd, 0, SEEK_END);
    lseek(fd, cur, SEEK_SET);
    return (long)end;
}
inline long _tell(int fd) { return (long)lseek(fd, 0, SEEK_CUR); }
