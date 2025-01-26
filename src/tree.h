#ifndef TREE_H
#define TREE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/sha.h>

struct blob {
        char type[5];
        size_t size;  
        size_t compressed_size;
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
        unsigned char hash[SHA_DIGEST_LENGTH];
        struct file_delta *delta;
        struct tree *subtree;
        struct tree_entry *next;
        struct blob *blob;
};

struct tree {
        char type[5]; 
        size_t entry_count;
        unsigned char hash[SHA_DIGEST_LENGTH];
        struct tree_entry *entries;
};

struct blob *create_blob(const char* file_path);
struct tree_entry *create_tree_entry(const char *name, struct blob *blob);
struct tree *create_tree(struct tree_entry *entry);
struct tree *form_tree(const char *dir_path);
void free_tree_entry(struct tree_entry *entry);
void free_tree_entries(struct tree_entry *entry);
void free_tree(struct tree *t);
int serialize_tree(FILE *out, struct tree *tree);
int deserialize_tree(FILE *in, struct tree **tree);
int restore_directory(struct tree *tree, const char *dir_path);
struct tree_entry *clone_tree_entry(const struct tree_entry *original);

#endif
