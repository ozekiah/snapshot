#include <stdio.h>
#include "config.h"
#include "snapshot.h"
#include "fs.h"
#include "main.h"

int create_snapshot()
{
        const char *source_directory = "/home/brem/Programming/checkpoint/src";
        const char *destination_file = "/etc/checkpoint/snapshots/test15.tar.xz";

        int result = create_tar_xz(source_directory, destination_file);
        if (result != 0) {
                fprintf(stderr, "Error creating tar.xz archive\n");
                return 1;
        }

        return 0;
}


