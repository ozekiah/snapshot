#ifndef DELTA_H
#define DELTA_H

#include "tree.h"

struct tree;

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
struct tree_delta *calculate_tree_delta(struct tree *old_tree, struct tree *new_tree);

#endif
