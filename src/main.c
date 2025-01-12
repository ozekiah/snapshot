#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "main.h"

#define PROGRAM_NAME "Checkpoint"
#define AUTHOR "ozekiah"
#define LICENSE "GNU GPL v2.0"

struct options opts = {
        .path = NULL,
        .mode = 0,
        .help = 0
};

static int parse_options(int argc, char *argv[])
{
        int opt;

        static struct option long_options[] = {
                {"mode", required_argument, 0, 'm'},
                {"help", no_argument, 0, 'h'},
                {0, 0, 0, 0}
        };

        while ((opt = getopt_long(argc, argv, "m:h", long_options, NULL)) != -1) {
                switch (opt) {
                case 'm':
                        if (strcmp(optarg, "save") == 0) {
                                opts.mode = MODE_SAVE;
                        } else if (strcmp(optarg, "restore") == 0) {
                                opts.mode = MODE_RESTORE;
                        } else if (strcmp(optarg, "discard") == 0) {
                                opts.mode = MODE_DISCARD;
                        }
                        break;
                case 'h':
                        opts.help = 1;
                        break;
                default:
                        print_usage(argv[0]);
                        return 1;
                
                }
        }

        if (optind >= argc) {
                perror("missing input files");
                return 1;
        }

        opts.path = (char**)malloc((argc - optind + 1) * sizeof(char*));
        if (opts.path == NULL) {
                perror("error allocing memory");
                return 1;
        }

        for (int i = optind; i < argc; i++) {
                opts.path[i - optind] = argv[i];
        }

        opts.path[argc - optind] = NULL;
        return 0;
}

static void print_help()
{

}

static void print_usage(const char *program_name)
{
        printf("Usage: %s [-v] [-m mode] [-h help] [path, path, ...]\n", program_name);
}

size_t path_count() 
{
        size_t count = 0;
        while (opts.path[count] != NULL) count++;
        return count;
}

void print_args() 
{
        printf("Paths: \n");
        for (int i = 0; i < path_count(); i++) {
                printf("    %s\n", opts.path[i]);
        }

        printf("Mode: %d\n", opts.mode);
}


int main(int argc, char *argv[])
{
        if (parse_options(argc, argv) != 0) {
                print_usage(argv[0]);
                return 1;
        }

        print_args();

        if (opts.help) {
                print_help();
                return 0;
        }
        

        /* handle options */

        return 0;
}
