#ifndef FS_H
#define FS_H

#include <archive.h>

int create_tar_xz(const char *src, const char *dst);
long int get_dir_inode(const char *dir_path);
int deflate_file(const char *src_path, const char *dst_path);
int inflate_file(const char *src_path, const char *dst_path);
int tmp_inflate_file(const char *src_path, const char *dst_path);
int path_exists(const char *path);

#endif
