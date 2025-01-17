#ifndef REVISION_H
#define REVISION_H

#include "tree.h"
#include "delta.h"

struct revision {
        int version;
        unsigned char hash[SHA_DIGEST_LENGTH];
        struct tree *base_tree;         // Only set for base version
        struct tree_delta *delta;       // Only set for delta versions
        int base_version;              
};

struct revision *create_base_revision(const char *dir_path);
struct revision *create_delta_revision(const char *rev_dir, struct revision *base, const char *current_dir);
int save_revision_to_file(const char *filepath, struct revision *rev);
struct revision *load_revision_from_file(const char *filepath);
int restore_revision(struct revision *rev, const char *output_dir);
int restore_specific_revision(const char *rev_dir, int target_version, const char *output_dir);
void free_revision(struct revision *rev);

#endif
