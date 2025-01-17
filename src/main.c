#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

#include "main.h"
#include "snapshot.h"
#include "config.h"

#define PROGRAM_NAME "snapshot"
#define AUTHOR "ozekiah"
#define LICENSE "GNU GPL v2.0"

struct config config;
struct options opts = {
        .path = NULL,
        .store = 0,
        .restore = 0,
        .revision = 0,
        .discard = 0,
        .list = 0,
        .compare = 0,
        .version = 0,
        .help = 0
};

static int get_path(int argc)
{
        if (optind >= argc) {
                char cwd[PATH_MAX];
                if (getcwd(cwd, PATH_MAX) == NULL) {
                        perror("getcwd");
                        return 1;
                }
                
                opts.path = strdup(cwd);
                if (opts.path == NULL) {
                        perror("strdup");
                        return 1;
                }
                return 0;
        }

        opts.path = strdup(optarg);
        if (opts.path == NULL) {
                perror("strdup");
                return 1;
        }
        
        return 0;
}

static int parse_options(int argc, char *argv[])
{
        int opt;

        static struct option long_options[] = {
                {"store", required_argument, 0, 's'},
                {"restore", required_argument, 0, 'r'},
                {"revision", required_argument, 0, 'R'},
                {"discard", required_argument, 0, 'd'},
                {"list", required_argument, 0, 'l'},
                {"compare", required_argument, 0, 'c'},
                {"version", no_argument, 0, 'v'},
                {"help", no_argument, 0, 'h'},
                {0, 0, 0, 0}
        };

        while ((opt = getopt_long(argc, argv, "s:r:R:d:l:c:h", long_options, NULL)) != -1) {
                switch (opt) {
                case 's':
                        if (get_path(argc) != 0) {
                            return 1;
                        }
                        opts.store = 1;
                        break;
                case 'r':
                        if (get_path(argc) != 0) {
                            return 1;
                        }
                        opts.restore = 1;
                        break;
                case 'R':
                        if (!optarg) {
                            fprintf(stderr, "Error: revision requires an argument\n");
                            return 1;
                        }
                        opts.revision = atoi(optarg);
                        if (opts.revision < 0) {
                            fprintf(stderr, "Error: invalid revision number\n");
                            return 1;
                        }
                        break;
                case 'd':
                        if (get_path(argc) != 0) {
                            return 1;
                        }
                        opts.discard = 1;
                        break;
                case 'l':
                        if (get_path(argc) != 0) {
                            return 1;
                        }
                        opts.list = 1;
                        break;
                case 'c':
                        if (get_path(argc) != 0) {
                            return 1;
                        }
                        opts.compare = 1;
                        break;
                case 'h':
                        opts.help = 1;
                        break;
                default:
                        print_usage(argv[0]);
                        return 1;
                }
        }

        return 0;
}

static void print_help()
{

}

static void print_usage(const char *program_name)
{
        printf("Usage: %s [-v] [-m mode] [-h help] [path, path, ...]\n", program_name);
}

void print_args() 
{
        printf("Args:\n");
        printf("    Path: %s\n", opts.path);
        printf("    Store: %d\n", opts.store);
        printf("    Restore: %d\n", opts.restore);
        printf("    Revision: %d\n", opts.revision);
        printf("    Discard: %d\n", opts.discard);
        printf("    List: %d\n", opts.list);
        printf("    Compare: %d\n", opts.compare);
}

int main(int argc, char *argv[])
{
        // TODO: check that the config file exists and the file structure is correct
        (void)deserialize_config(&config, "/etc/snapshot/config.yaml");

        if (parse_options(argc, argv) != 0) {
                print_usage(argv[0]);
                return 1;
        }

        print_args();

        if (opts.help) {
                print_help();
                return 0;
        }

        if (opts.store) {
                create_snapshot(opts.path);
        } 

        if (opts.restore) {
                restore_snapshot(opts.path, opts.revision);
        }

        free(opts.path);
        return 0;
}
