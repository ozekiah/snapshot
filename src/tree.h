#ifndef TREE_H
#define TREE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

enum blob_type {
        BLOB_FILE,
        BLOB_TREE
};

struct blob {
        enum blob_type type;
        size_t size;
        unsigned char *data;
        mode_t mode;
        uid_t uid;
        gid_t gid;
        struct timespec atime;
        struct timespec mtime;
        struct timespec ctime;
        char *link_target;
};

struct tree_entry {
        char mode[7];
        enum blob_type type;
        char name[256];
        unsigned char hash[20];
        struct tree_entry *next;
        struct blob *blob;
};

struct tree {
        enum blob_type type;
        size_t entry_count;
        struct tree_entry *entries;
};

#endif
