#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

#include "main.h"
#include "snapshot.h"
#include "config.h"

#define PROGRAM_NAME "SVD"
#define DESCRIPTION "Save Directory"
#define AUTHOR "Ozekiah"
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
                        opts.path = strdup(optarg);
                        opts.store = 1;
                        break;
                case 'r':
                        opts.path = strdup(optarg);
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
                        opts.path = strdup(optarg);
                        opts.discard = 1;
                        break;
                case 'l':
                        opts.path = strdup(optarg);
                        opts.list = 1;
                        break;
                case 'c':
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
        printf("%s:\n", PROGRAM_NAME);
        printf("Description: %s\n", DESCRIPTION);
        printf("Author: %s\n", AUTHOR);
        printf("License: %s\n\n", LICENSE);
        printf("Usage: %s [options] [path]\n\n", PROGRAM_NAME);
        printf("Options:\n");
        printf("  -s, --store        Store a new snapshot\n");
        printf("  -r, --restore      Restore from a snapshot\n");
        printf("  -R, --revision=N   Specify revision number for restore/compare\n");
        printf("  -d, --discard      Discard specified snapshot\n");
        printf("  -l, --list         List available snapshots\n");
        printf("  -c, --compare      Compare current state with snapshot\n");
        printf("  -h, --help         Display this help message\n");
}

static void print_usage(const char *program_name)
{
        printf("Usage: %s [-s store] [-r restore] [-d discard]\n    [-l list] [-c compare] [-R revision] [-h help]\n", program_name);
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
        (void)deserialize_config(&config, "/etc/svd/config");

        if (parse_options(argc, argv) != 0) {
                print_usage(argv[0]);
                return 1;
        }

        // print_args();

        if (opts.help) {
                print_help();
                goto cleanup;
        }

        if (opts.store) {
                create_snapshot(opts.path);
                goto cleanup;
        } 

        if (opts.restore) {
                restore_snapshot(opts.path, opts.revision);
                goto cleanup;
        }

        if (opts.discard) {
                discard_snapshot(opts.path);
                goto cleanup;
        }

        if (opts.list) {
                list_snapshot(opts.path);
                goto cleanup;
        }

cleanup:

        free(opts.path);
        return 0;
}
