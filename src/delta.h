#ifndef DELTA_H
#define DELTA_H

#include "tree.h"

struct file_delta {
        off_t offset;
        size_t deleted_size;
        size_t added_size;
        unsigned char *added_data;
        unsigned char *deleted_data;
};

struct tree_delta {
        struct tree_entry *added_entries;
        struct tree_entry *removed_entries;
        struct tree_entry *modified_entries;
};

void append_tree_entry_to_list(struct tree_entry **list, struct tree_entry *new_entry);
struct tree_entry *clone_tree_entry(const struct tree_entry *original);
struct tree_delta *calculate_tree_delta(struct tree *old_tree, struct tree *new_tree);
void apply_tree_delta(struct tree *tree, const struct tree_delta *delta);
void free_tree_delta(struct tree_delta *delta);
int serialize_tree_delta(FILE *out, struct tree_delta *delta);
int deserialize_tree_delta(FILE *in, struct tree_delta **delta);

#endif
