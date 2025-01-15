#include <stdio.h>
#include "config.h"
#include "snapshot.h"
#include "fs.h"
#include "main.h"
#include "utils.h"
#include "tree.h"

int create_snapshot()
{
        char *source_dir = opts.path[0];
        long int source_inode = get_dir_inode(source_dir);

        char destination_dir[4096];
        snprintf(destination_dir, sizeof(destination_dir), "%s/%ld", config.path, source_inode);
        mkdir(destination_dir, 0777);

        char destination_file[4096];
        char timestamp_str[26];
        timestamp(timestamp_str);
        snprintf(destination_file, sizeof(destination_file), "%s/%s", destination_dir, timestamp_str);

        int result = create_tar_xz(source_dir, destination_file);
        if (result != 0) {
                fprintf(stderr, "Error creating tar.xz archive\n");
                return 1;
        }

        return 0;
}
