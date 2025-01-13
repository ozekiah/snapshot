#ifndef FS_H
#define FS_H

#include <archive.h>

int create_tar_xz(const char *src, const char *dst);
int path_exists(const char *path);

#endif
