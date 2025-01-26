#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#include "config.h"
#include "snapshot.h"
#include "fs.h"
#include "main.h"
#include "utils.h"
#include "revision.h"

int create_snapshot(const char *dir_path)
{       
        if (!path_exists(dir_path)) {
                fprintf(stderr, "Error: Targetted directory does not exist.\n");
                return 1;
        }

        long int inode = get_dir_inode(dir_path);
        char rev_dir[PATH_MAX];
        snprintf(rev_dir, PATH_MAX, "%s/%ld", config.revisions, inode);

        if (mkdir(rev_dir, 0777) < 0 && errno != EEXIST) {
                perror("mkdir");
                return 1;
        }

        char base_path[PATH_MAX];
        snprintf(base_path, PATH_MAX, "%s/revision_0", rev_dir);

        if (access(base_path, F_OK) == -1) {
                struct revision *base = create_base_revision(dir_path);
                if (!base) {
                        perror("create base");
                        return 1;
                }
                

                if (save_revision_to_file(base_path, base) != 0) {
                        perror("save revision");
                        free_revision(base);
                        return 1;
                }

                printf("Saved base revision: %s\n", dir_path);
                free_revision(base);
        } else {
                struct revision *base = load_revision_from_file(base_path);
                if (!base) {
                        perror("load revision");
                        fprintf(stderr, "failed to load revision from file: %s\n", base_path);
                        return 1;
                }

                struct revision *delta = create_delta_revision(rev_dir, base, dir_path);
                if (!delta) {
                        fprintf(stderr, "failed to create delta file: %s\n", dir_path);
                        free_revision(base);
                        return 1;
                }

                char delta_path[PATH_MAX];
                snprintf(delta_path, sizeof(delta_path), "%s/revision_%d", rev_dir, delta->version);

                if (save_revision_to_file(delta_path, delta) != 0) {
                        fprintf(stderr, "failed to save delta revision: %s\n", delta_path);
                        free_revision(delta);
                        free_revision(base);
                        return 1;
                }

                printf("Saved delta revision: %s\n", dir_path);
                free_revision(delta);
                free_revision(base);

        }

        return 0;
}

int restore_snapshot(const char *dir_path, const int version)
{
        long int inode = get_dir_inode(dir_path);
        char rev_dir[PATH_MAX];
        snprintf(rev_dir, sizeof(rev_dir), "%s/%ld", config.revisions, inode);
        return (int)restore_specific_revision(rev_dir, version, dir_path);
}

int discard_snapshot(const char *dir_path) 
{
        long int inode = get_dir_inode(dir_path);
        char rev_dir[PATH_MAX];
        snprintf(rev_dir, sizeof(rev_dir), "%s/%ld", config.revisions, inode);
        return (int)remove_dir(rev_dir);
}
