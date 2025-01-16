#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tree.h"
#include "delta.h"

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
        strncpy(clone->name, original->name, sizeof(clone->name));
        memcpy(clone->hash, original->hash, sizeof(original->hash));
        clone->next = NULL;
        clone->delta = NULL;

        if (original->blob) {
                clone->blob = malloc(sizeof(struct blob));
                if (!clone->blob) {
                        free(clone);
                        return NULL;
                }

                memcpy(clone->blob, original->blob, sizeof(struct blob));
                
                if (original->blob->data) {
                        clone->blob->data = malloc(original->blob->size);
                        if (!clone->blob->data) {
                                free(clone->blob);
                                free(clone);
                                return NULL;
                        }
                        memcpy(clone->blob->data, original->blob->data, original->blob->size);
                }
        } else {
                clone->blob = NULL;
        }

        if (original->subtree) {
                clone->subtree = malloc(sizeof(struct tree));
                if (!clone->subtree) {
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
                                if (clone->blob) {
                                        free(clone->blob->data);
                                        free(clone->blob);
                                }
                                free(clone->subtree);
                                free(clone);
                                return NULL;
                        }
                        current_original = current_original->next;
                        current_clone = &(*current_clone)->next;
                }
        } else {
                clone->subtree = NULL;
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
        if (added) {
                append_tree_entry_to_list(added_list, added);
        }
}

static void process_removed_entry(struct tree_entry **removed_list, const struct tree_entry *entry) 
{
        struct tree_entry *removed = clone_tree_entry(entry);
        if (removed) {
                append_tree_entry_to_list(removed_list, removed);
        }
}

static void process_modified_entry(struct tree_entry **modified_list,
                                   const struct tree_entry *old_entry,
                                   const struct tree_entry *new_entry) 
{
        struct tree_entry *modified = clone_tree_entry(new_entry);
        if (!modified) return;

        modified->delta = malloc(sizeof(struct file_delta));
        if (modified->delta) {
                modified->delta->offset = 0;
                modified->delta->deleted_size = old_entry->blob ? old_entry->blob->size : 0;
                modified->delta->added_size = new_entry->blob ? new_entry->blob->size : 0;
                
                if (new_entry->blob && new_entry->blob->size > 0) {
                        modified->delta->added_data = malloc(new_entry->blob->size);
                        if (modified->delta->added_data) {
                                memcpy(modified->delta->added_data, new_entry->blob->data, 
                                       new_entry->blob->size);
                        }
                } else {
                        modified->delta->added_data = NULL;
                }

                if (old_entry->blob && old_entry->blob->size > 0) {
                        modified->delta->deleted_data = malloc(old_entry->blob->size);
                        if (modified->delta->deleted_data) {
                                memcpy(modified->delta->deleted_data, old_entry->blob->data,
                                       old_entry->blob->size);
                        }
                } else {
                        modified->delta->deleted_data = NULL;
                }
        }

        append_tree_entry_to_list(modified_list, modified);
}

static void process_subtree_delta(struct tree_delta *delta,
                                  const struct tree_entry *old_entry,
                                  const struct tree_entry *new_entry) 
{
        struct tree_delta *sub_delta = calculate_tree_delta(old_entry->subtree,
                                                          new_entry->subtree);
        if (sub_delta) {
                struct tree_entry *current;
                
                current = sub_delta->added_entries;
                while (current) {
                        process_added_entry(&delta->added_entries, current);
                        current = current->next;
                }

                current = sub_delta->removed_entries;
                while (current) {
                        process_removed_entry(&delta->removed_entries, current);
                        current = current->next;
                }

                current = sub_delta->modified_entries;
                while (current) {
                        append_tree_entry_to_list(&delta->modified_entries, 
                                                clone_tree_entry(current));
                        current = current->next;
                }

                free_tree_delta(sub_delta);
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
                        if (memcmp(old_entry->hash, new_entry->hash, SHA_DIGEST_LENGTH) != 0) {
                                process_modified_entry(&delta->modified_entries,
                                                    old_entry, new_entry);
                        }
                        if (old_entry->subtree && new_entry->subtree) {
                                process_subtree_delta(delta, old_entry, new_entry);
                        }
                        old_entry = old_entry->next;
                        new_entry = new_entry->next;
                }
        }

        return delta;
}

void free_tree_delta(struct tree_delta *delta) 
{
        if (!delta) return;
        
        free_tree_entries(delta->added_entries);
        free_tree_entries(delta->removed_entries);
        free_tree_entries(delta->modified_entries);
        
        free(delta);
}

int serialize_tree_delta(FILE *out, struct tree_delta *delta) 
{
        if (!out || !delta) return -1;

        struct tree_entry *entry = delta->added_entries;
        while (entry) {
                char type = 'A';
                if (fwrite(&type, 1, 1, out) != 1) return -1;
                if (serialize_tree(out, create_tree(entry)) != 0) return -1;
                entry = entry->next;
        }

        entry = delta->removed_entries;
        while (entry) {
                char type = 'R';
                if (fwrite(&type, 1, 1, out) != 1) return -1;
                if (serialize_tree(out, create_tree(entry)) != 0) return -1;
                entry = entry->next;
        }

        entry = delta->modified_entries;
        while (entry) {
                char type = 'M'; 
                if (fwrite(&type, 1, 1, out) != 1) return -1;
                if (serialize_tree(out, create_tree(entry)) != 0) return -1;
                
                if (entry->delta) {
                        if (fwrite(&entry->delta->offset, sizeof(off_t), 1, out) != 1 ||
                            fwrite(&entry->delta->deleted_size, sizeof(size_t), 1, out) != 1 ||
                            fwrite(&entry->delta->added_size, sizeof(size_t), 1, out) != 1) {
                                return -1;
                        }

                        if (entry->delta->deleted_size > 0 && 
                            fwrite(entry->delta->deleted_data, 1, entry->delta->deleted_size, out) 
                            != entry->delta->deleted_size) {
                                return -1;
                        }

                        if (entry->delta->added_size > 0 &&
                            fwrite(entry->delta->added_data, 1, entry->delta->added_size, out) != entry->delta->added_size) {
                                return -1;
                        }
                }
                entry = entry->next;
        }

        return 0;
}

int deserialize_tree_delta(FILE *in, struct tree_delta **delta) 
{
        if (!in || !delta) return -1;

        *delta = malloc(sizeof(struct tree_delta));
        if (!*delta) return -1;

        (*delta)->added_entries = NULL;
        (*delta)->removed_entries = NULL;
        (*delta)->modified_entries = NULL;

        char type;
        struct tree *temp_tree;
        while (fread(&type, 1, 1, in) == 1) {
                if (deserialize_tree(in, &temp_tree) != 0) {
                        free_tree_delta(*delta);
                        *delta = NULL;
                        return -1;
                }

                struct tree_entry *entry = temp_tree->entries;
                temp_tree->entries = NULL;
                free_tree(temp_tree);

                switch (type) {
                        case 'A':
                                append_tree_entry_to_list(&(*delta)->added_entries, entry);
                                break;
                        case 'R':
                                append_tree_entry_to_list(&(*delta)->removed_entries, entry);
                                break;
                        case 'M':
                                entry->delta = malloc(sizeof(struct file_delta));
                                if (!entry->delta) {
                                        free_tree_entry(entry);
                                        free_tree_delta(*delta);
                                        *delta = NULL;
                                        return -1;
                                }

                                if (fread(&entry->delta->offset, sizeof(off_t), 1, in) != 1 ||
                                    fread(&entry->delta->deleted_size, sizeof(size_t), 1, in) != 1 ||
                                    fread(&entry->delta->added_size, sizeof(size_t), 1, in) != 1) {
                                        free_tree_entry(entry);
                                        free_tree_delta(*delta);
                                        *delta = NULL;
                                        return -1;
                                }

                                if (entry->delta->deleted_size > 0) {
                                        entry->delta->deleted_data = malloc(entry->delta->deleted_size);
                                        if (!entry->delta->deleted_data ||
                                            fread(entry->delta->deleted_data, 1, 
                                                  entry->delta->deleted_size, in) != 
                                                  entry->delta->deleted_size) {
                                                free_tree_entry(entry);
                                                free_tree_delta(*delta);
                                                *delta = NULL;
                                                return -1;
                                        }
                                } else {
                                        entry->delta->deleted_data = NULL;
                                }

                                if (entry->delta->added_size > 0) {
                                        entry->delta->added_data = malloc(entry->delta->added_size);
                                        if (!entry->delta->added_data ||
                                            fread(entry->delta->added_data, 1,
                                                  entry->delta->added_size, in) != 
                                                  entry->delta->added_size) {
                                                free_tree_entry(entry);
                                                free_tree_delta(*delta);
                                                *delta = NULL;
                                                return -1;
                                        }
                                } else {
                                        entry->delta->added_data = NULL;
                                }

                                append_tree_entry_to_list(&(*delta)->modified_entries, entry);
                                break;
                        default:
                                free_tree_entry(entry);
                                free_tree_delta(*delta);
                                *delta = NULL;
                                return -1;
                }
        }

        return 0;
}

void apply_tree_delta(struct tree *tree, const struct tree_delta *delta) 
{
        if (!tree || !delta) {
                fprintf(stderr, "Invalid arguments to apply_tree_delta\n");
                return;
        }

        // Handle added entries
        struct tree_entry *added_entry = delta->added_entries;
        while (added_entry) {
                struct tree_entry *cloned_entry = clone_tree_entry(added_entry);
                if (!cloned_entry) {
                        fprintf(stderr, "Failed to clone added entry '%s'\n", added_entry->name);
                        return;
                }

                append_tree_entry_to_list(&tree->entries, cloned_entry);
                added_entry = added_entry->next;
        }

        // Handle removed entries
        struct tree_entry *removed_entry = delta->removed_entries;
        while (removed_entry) {
                struct tree_entry **current = &tree->entries;
                while (*current) {
                        if (strcmp((*current)->name, removed_entry->name) == 0) {
                                struct tree_entry *to_free = *current;
                                *current = (*current)->next;
                                free_tree_entry(to_free);
                                break;
                        }

                        current = &(*current)->next;
                }

                removed_entry = removed_entry->next;
        }

        // Handle modified entries
        struct tree_entry *modified_entry = delta->modified_entries;
        while (modified_entry) {
                struct tree_entry **current = &tree->entries;
                while (*current) {
                        if (strcmp((*current)->name, modified_entry->name) == 0) {
                                if ((*current)->blob && modified_entry->blob) {
                                        free((*current)->blob->data);
                                        (*current)->blob->data = malloc(modified_entry->blob->size);
                                        if ((*current)->blob->data) {
                                                memcpy((*current)->blob->data, modified_entry->blob->data, modified_entry->blob->size);
                                                (*current)->blob->size = modified_entry->blob->size;
                                        }
                                }
                                break;
                        }
                    current = &(*current)->next;
                }
                modified_entry = modified_entry->next;
        }
}
