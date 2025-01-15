#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tree.h"

void append_tree_entry_to_list(struct tree_entry **list, struct tree_entry *new_entry) 
{
        if (!new_entry) {
                fprintf(stderr, "Invalid new_entry in append_tree_entry_to_list\n");
                return;
        }

        if (!*list) {
                *list = new_entry;
        } else {
                struct tree_entry *current = *list;
                while (current->next) {
                        current = current->next;
                }
                current->next = new_entry;
        }
}

struct tree_entry *clone_tree_entry(const struct tree_entry *original)
{
        if (!original) {
                return NULL;
        }

        struct tree_entry *clone = malloc(sizeof(struct tree_entry));
        if (!clone) {
                perror("malloc");
                return NULL;
        }

        memcpy(clone->mode, original->mode, sizeof(original->mode));
        memcpy(clone->type, original->type, sizeof(original->type));
        strncpy(clone->name, original->name, sizeof(original->name));
        memcpy(clone->hash, original->hash, sizeof(original->hash));
        clone->next = NULL;

        // Clone blob
        if (original->blob) {
                clone->blob = malloc(sizeof(struct blob));
                if (!clone->blob) {
                    perror("malloc");
                    free(clone);
                    return NULL;
                }

                memcpy(clone->blob, original->blob, sizeof(struct blob));

                if (original->blob->data) {
                        clone->blob->data = malloc(original->blob->size);
                        if (!clone->blob->data) {
                                perror("malloc");
                                free(clone->blob);
                                free(clone);
                                return NULL;
                        }

                        memcpy(clone->blob->data, original->blob->data, original->blob->size);
                }
        } else {
                clone->blob = NULL;
        }

        // Clone subtree
        if (original->subtree) {
                clone->subtree = malloc(sizeof(struct tree));
                if (!clone->subtree) {
                        perror("malloc");
                        if (clone->blob) {
                                free(clone->blob->data);
                                free(clone->blob);
                        }
                        free(clone);
                        return NULL;
                }

                memcpy(clone->subtree, original->subtree, sizeof(struct tree));
                clone->subtree->entries = NULL;

                struct tree_entry *current_original = original->subtree->entries;
                struct tree_entry **current_clone = &clone->subtree->entries;

                while (current_original) {
                        *current_clone = clone_tree_entry(current_original);
                        if (!*current_clone) {
                                perror("clone_tree_entry");
                                free(clone->subtree);
                                if (clone->blob) {
                                        free(clone->blob->data);
                                        free(clone->blob);
                                }

                                free(clone);
                                return NULL;
                        }
                        current_original = current_original->next;
                        current_clone = &(*current_clone)->next;
                }
        } else {
                clone->subtree = NULL;
        }

        // Clone delta
        if (original->delta) {
                clone->delta = malloc(sizeof(struct file_delta));
                if (!clone->delta) {
                        perror("malloc");
                        if (clone->subtree) {
                                free(clone->subtree);
                        }
                        
                        if (clone->blob) {
                                free(clone->blob->data);
                                free(clone->blob);
                        }
                        free(clone);
                        return NULL;
                }

                memcpy(clone->delta, original->delta, sizeof(struct file_delta));

                if (original->delta->added_data) {
                        clone->delta->added_data = malloc(original->delta->added_size);
                        if (!clone->delta->added_data) {
                                perror("malloc");
                                free(clone->delta);
                                if (clone->subtree) {
                                        free(clone->subtree);
                                }
                                if (clone->blob) {
                                        free(clone->blob->data);
                                        free(clone->blob);
                                }

                                free(clone);
                                return NULL;
                        }

                        memcpy(clone->delta->added_data, original->delta->added_data, original->delta->added_size);
                } else {
                        clone->delta->added_data = NULL;
                }

                if (original->delta->deleted_data) {
                        clone->delta->deleted_data = malloc(original->delta->deleted_size);
                        if (!clone->delta->deleted_data) {
                                perror("malloc");
                                free(clone->delta->added_data);
                                free(clone->delta);
                                if (clone->subtree) {
                                        free(clone->subtree);
                                }
                                if (clone->blob) {
                                        free(clone->blob->data);
                                        free(clone->blob);
                                }

                                free(clone);
                                return NULL;
                        }
                            
                        memcpy(clone->delta->deleted_data, original->delta->deleted_data, original->delta->deleted_size);
                } else {
                        clone->delta->deleted_data = NULL;
                }
        } else {
                clone->delta = NULL;
        }

        return clone;
}

static int compare_tree_entries(const struct tree_entry *a, const struct tree_entry *b)
{
        if (!a && !b) return 0;
        if (!a) return 1;
        if (!b) return -1;
        return strcmp(a->name, b->name);
}

static void process_added_entry(struct tree_entry **added_list, const struct tree_entry *entry) 
{
        struct tree_entry *added = clone_tree_entry(entry);
        append_tree_entry_to_list(added_list, added);
}

static void process_removed_entry(struct tree_entry **removed_list, const struct tree_entry *entry) 
{
        struct tree_entry *removed = clone_tree_entry(entry);
        append_tree_entry_to_list(removed_list, removed);
}

static void process_modified_entry(struct tree_entry **modified_list,
                                   const struct tree_entry *old_entry,
                                   const struct tree_entry *new_entry) 
{
        struct tree_entry *modified = clone_tree_entry(new_entry);
        modified->delta = malloc(sizeof(struct file_delta));
        if (modified->delta) {
                modified->delta->offset = 0;
                modified->delta->deleted_size = old_entry->blob ? old_entry->blob->size : 0;
                modified->delta->added_size = new_entry->blob ? new_entry->blob->size : 0;
                modified->delta->added_data = new_entry->blob ? malloc(new_entry->blob->size) : NULL;
                modified->delta->deleted_data = old_entry->blob ? malloc(old_entry->blob->size) : NULL;

                if (modified->delta->added_data && new_entry->blob) {
                    memcpy(modified->delta->added_data, new_entry->blob->data, new_entry->blob->size);
                }

                if (modified->delta->deleted_data && old_entry->blob) {
                    memcpy(modified->delta->deleted_data, old_entry->blob->data, old_entry->blob->size);
                }
        }

        append_tree_entry_to_list(modified_list, modified);
}

static void process_subtree_delta(struct tree_delta *delta, 
                                  const struct tree_entry *old_entry,
                                  const struct tree_entry *new_entry)
{
        struct tree_delta *sub_delta = calculate_tree_delta(old_entry->subtree, new_entry->subtree);
        if (sub_delta) {
                struct tree_entry *sub_added = sub_delta->added_entries;
                while (sub_added) {
                        append_tree_entry_to_list(&delta->added_entries, sub_added);
                        sub_added = sub_added->next;
                }

                struct tree_entry *sub_removed = sub_delta->removed_entries;
                while (sub_removed) {
                        append_tree_entry_to_list(&delta->removed_entries, sub_removed);
                        sub_removed = sub_removed->next;
                }

                struct tree_entry *sub_modified = sub_delta->modified_entries;
                while (sub_modified) {
                        append_tree_entry_to_list(&delta->modified_entries, sub_modified);
                        sub_modified = sub_modified->next;
                }

                free(sub_delta);
        }
}

struct tree_delta *calculate_tree_delta(struct tree *old_tree, struct tree *new_tree)
{
        struct tree_delta *delta = malloc(sizeof(struct tree_delta));
        if (!delta) {
                perror("malloc");
                return NULL;
        }

        delta->added_entries = NULL;
        delta->removed_entries = NULL;
        delta->modified_entries = NULL;

        struct tree_entry *old_entry = old_tree ? old_tree->entries : NULL;
        struct tree_entry *new_entry = new_tree ? new_tree->entries : NULL;

        while (old_entry || new_entry) {
                int comparison = compare_tree_entries(old_entry, new_entry);

                if (comparison < 0) {
                        process_removed_entry(&delta->removed_entries, old_entry);
                        old_entry = old_entry->next;
                } else if (comparison > 0) {
                        process_added_entry(&delta->added_entries, new_entry);
                        new_entry = new_entry->next;
                } else {
                        if (strcmp(old_entry->hash, new_entry->hash) != 0) {
                                process_modified_entry(&delta->modified_entries, old_entry, new_entry);
                        }
                        if (old_entry->subtree || new_entry->subtree) {
                                process_subtree_delta(delta, old_entry, new_entry);
                        }
                        old_entry = old_entry->next;
                        new_entry = new_entry->next;
                }
        }

        return delta;
}

