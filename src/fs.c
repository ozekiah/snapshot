#include <archive.h>
#include <archive_entry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

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

int path_exists(const char *path) 
{
        struct stat buffer;
        return (stat(path, &buffer) == 0);
}
