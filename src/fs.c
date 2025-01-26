#include <archive.h>
#include <archive_entry.h> 
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fs.h"

static void add_file_to_archive(struct archive *a, const char *path, const char *base)
{
        struct archive_entry *entry;
        struct stat st;
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", base, path);

        if (stat(fullpath, &st) != 0) {
                perror("stat");
                return;
        }

        entry = archive_entry_new();
        archive_entry_set_pathname(entry, path);
        archive_entry_set_size(entry, st.st_size);
        archive_entry_set_perm(entry, st.st_mode & 0777);

        if (S_ISREG(st.st_mode)) {
                archive_entry_set_filetype(entry, AE_IFREG);
        } else if (S_ISDIR(st.st_mode)) {
                archive_entry_set_filetype(entry, AE_IFDIR);
        }

        archive_write_header(a, entry);

        if (S_ISREG(st.st_mode)) {
                FILE *file = fopen(fullpath, "rb");
                if (file) {
                        char buffer[8192];
                        size_t bytes_read;
                        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                                archive_write_data(a, buffer, bytes_read);
                        }

                        fclose(file);
                }
        }

        archive_entry_free(entry);
}

static void add_directory_to_archive(struct archive *a, const char *base, const char *dir) 
{
        DIR *dp;
        struct dirent *entry;
        char path[1024];

        dp = opendir(dir);
        if (!dp) {
                perror("opendir");
                return;
        }

        printf("Opening directory: %s\n", dir);

        while ((entry = readdir(dp)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                        continue;
                }

                snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
                printf("Processing entry: %s\n", path);

                if (entry->d_type == DT_DIR) {
                        add_file_to_archive(a, path + strlen(base) + 1, base);
                        add_directory_to_archive(a, base, path);
                } else {
                        add_file_to_archive(a, path + strlen(base) + 1, base);
                }
        }

        closedir(dp);
}

int create_tar_xz(const char *src, const char *dst) 
{
        struct archive *a = archive_write_new();
        archive_write_set_format_pax_restricted(a);
        archive_write_add_filter_xz(a);  

        if (archive_write_open_filename(a, dst) != ARCHIVE_OK) {
                fprintf(stderr, "Failed to open output file: %s\n", archive_error_string(a));
                archive_write_free(a);
                return 1;
        }

        add_directory_to_archive(a, src, src);

        archive_write_close(a);
        archive_write_free(a);

        printf("Created .tar.xz: %s\n", dst);
        return 0;
}



long int get_dir_inode(const char *dir_path)
{
        struct stat dir_stat;
        if (stat(dir_path, &dir_stat) == -1) {
                perror("stat");
                return -1;
        }

        return dir_stat.st_ino;
}

int deflate_file(const char *src_path, const char *dst_path)
{
        FILE *src = fopen(src_path, "rb");
        FILE *dst = fopen(dst_path, "wb");
        if (!src || !dst) return -1;

        z_stream strm = { 0 };
        deflateInit(&strm, Z_DEFAULT_COMPRESSION);

        unsigned char src_buf[16384];
        unsigned char dst_buf[16384];
        int ret;

        do {
                strm.avail_in = fread(src_buf, 1, sizeof(src_buf), src);
                if (ferror(src)) return -1;
                strm.next_in = src_buf;

                do {
                        strm.avail_out = sizeof(dst_buf);
                        strm.next_out = dst_buf;
                        ret = deflate(&strm, feof(src) ? Z_FINISH : Z_NO_FLUSH);
                        fwrite(dst_buf, 1, sizeof(dst_buf) - strm.avail_out, dst);                
                } while (strm.avail_out == 0);
        } while (ret != Z_STREAM_END);

        deflateEnd(&strm);
        fclose(src);
        fclose(dst);
        return 0;
}

int inflate_file(const char *src_path, const char *dst_path) 
{
        FILE *src = fopen(src_path, "rb");
        FILE *dst = fopen(dst_path, "wb");
        if (!src || !dst) return -1;

        z_stream strm = {0};
        inflateInit(&strm);

        unsigned char src_buf[16384];
        unsigned char dst_buf[16384];
        int ret;

        do {
                strm.avail_in = fread(src_buf, 1, sizeof(src_buf), src);
                if (ferror(src)) return -1;
                strm.next_in = src_buf;

                do {
                        strm.avail_out = sizeof(dst_buf);
                        strm.next_out = dst_buf;
                        ret = inflate(&strm, Z_NO_FLUSH);
                        fwrite(dst_buf, 1, sizeof(dst_buf) - strm.avail_out, dst);
                } while (strm.avail_out == 0);
        } while (ret != Z_STREAM_END);

        inflateEnd(&strm);
        fclose(src);
        fclose(dst);
        return 0;
}

int tmp_inflate_file(const char *src_path, const char *dst_path)
{
        char tmp_dst_path[PATH_MAX];
        snprintf(tmp_dst_path, PATH_MAX, "/tmp/%s.tmp", src_path);

        if (inflate_file(src_path, tmp_dst_path) != 0) {
                fprintf(stderr, "failed to deflaite file: %s", src_path);
                return -1;
        }
        
        return 0;
}

void cleanup_tmp_files()
{
       // TODO; 
}

int path_exists(const char *path) 
{
        struct stat buffer;
        return (stat(path, &buffer) == 0);
}
                
int remove_dir(const char *path)
{
        DIR *d = opendir(path);
        if (!d) return -1;
    
        struct dirent *p;
        char file_path[1024];
        int len = strlen(path);
    
        while ((p = readdir(d))) {
                if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                 continue;

                snprintf(file_path, sizeof(file_path), "%s/%s", path, p->d_name);
                struct stat statbuf;
                stat(file_path, &statbuf);

                if (S_ISDIR(statbuf.st_mode))
                        remove_dir(file_path);
                else
                        unlink(file_path);
        }
    
        closedir(d);
        return rmdir(path);
}
