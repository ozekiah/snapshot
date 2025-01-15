#ifndef TREE_H
#define TREE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

struct file_delta {
        off_t offset;
        size_t deleted_size;
        size_t added_size;
        unsigned char *added_data;
        unsigned char *deleted_data;
};

struct blob {
        char type[5]; 
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
        char type[7]; 
        char name[256];
        unsigned char hash[20];
        struct tree *subtree;
        struct tree_entry *next;
        struct blob *blob;
};

struct tree {
        char type[5]; 
        size_t entry_count;
        struct tree_entry *entries;
};

struct blob *create_blob(const char* file_path);
struct tree_entry *create_tree_entry(const char *name, struct blob *blob);
struct tree *create_tree(struct tree_entry *entry);
struct tree *form_tree(const char *dir_path);
struct tree *form_tree(const char *dir_path);
int serialize_tree(FILE *out, struct tree *tree);
int deserialize_tree(FILE *in, struct tree **tree);

#endif
