#include <stdio.h>
#include "config.h"
#include "snapshot.h"
#include "fs.h"
#include "main.h"

int create_snapshot()
{
        char *source_directory = opts.path[0];

        char destination_file[4096];
        snprintf(destination_file, sizeof(destination_file), "%s/test1.tar.xz", config.path);

        int result = create_tar_xz(source_directory, destination_file);
        if (result != 0) {
                fprintf(stderr, "Error creating tar.xz archive\n");
                return 1;
        }

        return 0;
}


