#ifndef MAIN_H
#define MAIN_H

#include "config.h"

extern struct options opts;
extern struct config config;

static void print_help();
static void print_usage();

struct options {
        char *path;
        int store;
        int restore;
        int revision;
        int discard;
        int list;
        int compare;
        int version;
        int help;
};

#endif
