#ifndef REVISION_H
#define REVISION_H

#include <openssl/sha.h>
#include "tree.h"
#include "delta.h"

struct revision {
        int version;
        unsigned char hash[SHA_DIGEST_LENGTH];
        struct tree *base_tree;
        struct tree_delta *delta;
        int base_version;              
};

struct revision *create_base_revision(const char *dir_path);
struct revision *create_delta_revision(const char *rev_dir, struct revision *base, const char *current_dir);
struct revision **get_revisions(const char *rev_dir, size_t *count);
int save_revision_to_file(const char *filepath, struct revision *rev);
struct revision *load_revision_from_file(const char *filepath);
void free_revision(struct revision *rev);
int restore_specific_revision(const char *rev_dir, int target_version, const char *output_dir);

#endif
