#ifndef MAIN_H
#define MAIN_H

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

extern struct options opts;

static void print_help();
static void print_usage();

struct options {
    char **path;
    int mode;
    int help;
};

#endif
