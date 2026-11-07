// WiiUNX2 GPL-3.0-or-later. Flush a stdio stream to storage before it is closed or renamed.
#pragma once
#include <cerrno>
#include <cstdio>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif
namespace nx {
// Filesystems without sync support report EINVAL, ENOSYS or ENOTSUP; those count as success.
inline bool syncFile(FILE *file) {
    if(std::fflush(file)!=0) return false;
#if defined(_WIN32)
    const int result=_commit(_fileno(file));
#else
    const int result=fsync(fileno(file));
#endif
    if(result==0) return true;
    return errno==EINVAL || errno==ENOSYS || errno==ENOTSUP;
}
}
