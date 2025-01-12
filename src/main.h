#ifndef MAIN_H
#define MAIN_H

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

extern struct options opts;

static void print_help();
static void print_usage();

enum mode {
       MODE_CREATE,
       MODE_RESTORE,
       MODE_DISCARD,
       MODE_LIST,
       MODE_COMPARE
};

struct options {
        char **path;
        enum mode mode;
        int help;
};

#endif
