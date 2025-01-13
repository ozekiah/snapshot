#ifndef FS_H
#define FS_H

#include <archive.h>

static void add_file_to_archive(struct archive *a, const char *path, const char *base);
static void add_directory_to_archive(struct archive *a, const char *base, const char *dir);
int create_tar_xz(const char *src, const char *dst);

#endif
