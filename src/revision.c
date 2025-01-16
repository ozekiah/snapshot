#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "revision.h"

#include "delta.h"
#include "tree.h"

struct revision *create_base_revision(const char *dir_path) {
        struct revision *rev = malloc(sizeof(struct revision));
        if (!rev) {
                perror("malloc");
                return NULL;
        }

        rev->version = 0;
        rev->base_tree = form_tree(dir_path);
        if (!rev->base_tree) {
                free(rev);
                return NULL;
        }

        rev->delta = NULL;
        rev->base_version = -1;

        // Calculate hash of the tree
        FILE *tmp = tmpfile();
        if (!tmp) {
                free_tree(rev->base_tree);
                free(rev);
                return NULL;
        }

        if (serialize_tree(tmp, rev->base_tree) != 0) {
                fclose(tmp);
                free_tree(rev->base_tree);
                free(rev);
                return NULL;
        }

        fseek(tmp, 0, SEEK_END);
        size_t size = ftell(tmp);
        fseek(tmp, 0, SEEK_SET);

        unsigned char *buffer = malloc(size);
        if (!buffer) {
                fclose(tmp);
                free_tree(rev->base_tree);
                free(rev);
                return NULL;
        }

        if (fread(buffer, 1, size, tmp) != size) {
                free(buffer);
                fclose(tmp);
                free_tree(rev->base_tree);
                free(rev);
                return NULL;
        }

        SHA1(buffer, size, rev->hash);

        free(buffer);
        fclose(tmp);

        return rev;
}

struct revision *create_delta_revision(struct revision *base, const char *current_dir) {
        if (!base || !current_dir) return NULL;

        struct revision *rev = malloc(sizeof(struct revision));
        if (!rev) {
                perror("malloc");
                return NULL;
        }

        struct tree *current_tree = form_tree(current_dir);
        if (!current_tree) {
                free(rev);
                return NULL;
        }

        rev->version = base->version + 1;
        rev->base_tree = NULL;
        rev->delta = calculate_tree_delta(base->base_tree, current_tree);
        rev->base_version = base->version;

        if (!rev->delta) {
                free_tree(current_tree);
                free(rev);
                return NULL;
        }

        // Calculate hash based on base hash + delta
        FILE *tmp = tmpfile();
        if (!tmp) {
                free_tree_delta(rev->delta);
                free_tree(current_tree);
                free(rev);
                return NULL;
        }

        fwrite(base->hash, SHA_DIGEST_LENGTH, 1, tmp);
        if (serialize_tree_delta(tmp, rev->delta) != 0) {
                fclose(tmp);
                free_tree_delta(rev->delta);
                free_tree(current_tree);
                free(rev);
                return NULL;
        }

        fseek(tmp, 0, SEEK_END);
        size_t size = ftell(tmp);
        fseek(tmp, 0, SEEK_SET);

        unsigned char *buffer = malloc(size);
        if (!buffer) {
                fclose(tmp);
                free_tree_delta(rev->delta);
                free_tree(current_tree);
                free(rev);
                return NULL;
        }

        if (fread(buffer, 1, size, tmp) != size) {
                free(buffer);
                fclose(tmp);
                free_tree_delta(rev->delta);
                free_tree(current_tree);
                free(rev);
                return NULL;
        }

        SHA1(buffer, size, rev->hash);

        free(buffer);
        fclose(tmp);
        free_tree(current_tree);

        return rev;
}

int save_revision_to_file(const char *filepath, struct revision *rev) {
        if (!filepath || !rev) return -1;

        FILE *f = fopen(filepath, "wb");
        if (!f) {
                perror("fopen");
                return -1;
        }

        if (fwrite(&rev->version, sizeof(int), 1, f) != 1 ||
            fwrite(&rev->base_version, sizeof(int), 1, f) != 1 ||
            fwrite(rev->hash, SHA_DIGEST_LENGTH, 1, f) != 1) {
                fclose(f);
                return -1;
        }

        if (rev->base_tree) {
                if (serialize_tree(f, rev->base_tree) != 0) {
                        fclose(f);
                        return -1;
                }
        } else if (rev->delta) {
                if (serialize_tree_delta(f, rev->delta) != 0) {
                        fclose(f);
                        return -1;
                }
        }

        fclose(f);
        return 0;
}

struct revision *load_revision_from_file(const char *filepath) {
        if (!filepath) return NULL;

        FILE *f = fopen(filepath, "rb");
        if (!f) {
                perror("fopen");
                return NULL;
        }

        struct revision *rev = malloc(sizeof(struct revision));
        if (!rev) {
                fclose(f);
                return NULL;
        }

        if (fread(&rev->version, sizeof(int), 1, f) != 1 ||
            fread(&rev->base_version, sizeof(int), 1, f) != 1 ||
            fread(rev->hash, SHA_DIGEST_LENGTH, 1, f) != 1) {
                free(rev);
                fclose(f);
                return NULL;
        }

        if (rev->base_version == -1) {
                rev->delta = NULL;
                if (deserialize_tree(f, &rev->base_tree) != 0) {
                        free(rev);
                        fclose(f);
                        return NULL;
                }
        } else {
                rev->base_tree = NULL;
                if (deserialize_tree_delta(f, &rev->delta) != 0) {
                        free(rev);
                        fclose(f);
                        return NULL;
                }
        }

        fclose(f);
        return rev;
}

void free_revision(struct revision *rev) {
        if (!rev) return;

        if (rev->base_tree) {
                free_tree(rev->base_tree);
        }
        if (rev->delta) {
                free_tree_delta(rev->delta);
        }
        free(rev);
}

void print_usage(const char *program) 
{
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  Save current state:   %s <directory>\n", program);
        fprintf(stderr, "  Restore a revision:   %s -r <revision_number> <output_directory>\n", program);
}

int restore_specific_revision(int target_version, const char *output_dir) 
{
        char rev_path[32];
        snprintf(rev_path, sizeof(rev_path), "revision_%d", target_version);

        struct revision *rev = load_revision_from_file(rev_path);
        if (!rev) {
                fprintf(stderr, "Failed to load revision %d\n", target_version);
                return 1;
        }

        if (rev->base_tree) {
                // Base revision - restore directly
                if (restore_directory(rev->base_tree, output_dir) != 0) {
                        fprintf(stderr, "Failed to restore directory\n");
                        free_revision(rev);
                        return 1;
                }
        } else {
                // Delta revision - need to load base and apply deltas
                struct revision *base = load_revision_from_file("revision_0");
                if (!base) {
                        fprintf(stderr, "Failed to load base revision\n");
                        free_revision(rev);
                        return 1;
                }

                // Clone the base tree
                struct tree *working_tree = NULL;
                FILE *tmp = tmpfile();
                if (!tmp) {
                        free_revision(base);
                        free_revision(rev);
                        return 1;
                }

                serialize_tree(tmp, base->base_tree);
                fseek(tmp, 0, SEEK_SET);
                deserialize_tree(tmp, &working_tree);
                fclose(tmp);

                // apply all deltas up to the target version
                for (int i = 1; i <= target_version; i++) {
                        char delta_path[32];
                        snprintf(delta_path, sizeof(delta_path), "revision_%d", i);
                        struct revision *delta_rev = load_revision_from_file(delta_path);
                        if (!delta_rev) {
                                fprintf(stderr, "Failed to load revision %d\n", i);
                                free_tree(working_tree);
                                free_revision(base);
                                free_revision(rev);
                                return 1;
                        }

                        apply_tree_delta(working_tree, delta_rev->delta);
                        free_revision(delta_rev);
                }

                // restore the final state
                if (restore_directory(working_tree, output_dir) != 0) {
                        fprintf(stderr, "Failed to restore directory\n");
                        free_tree(working_tree);
                        free_revision(base);
                        free_revision(rev);
                        return 1;
                }

                free_tree(working_tree);
                free_revision(base);
        }

        free_revision(rev);
        printf("Successfully restored revision %d to %s\n", target_version, output_dir);
        return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-r") == 0) {
        if (argc != 4) {
            print_usage(argv[0]);
            return 1;
        }
        int revision = atoi(argv[2]);
        return restore_specific_revision(revision, argv[3]);
    }

    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *dir_path = argv[1];
    char base_path[] = "revision_0";

    if (access(base_path, F_OK) == -1) {
        struct revision *base = create_base_revision(dir_path);
        if (!base) {
            fprintf(stderr, "Failed to create base revision\n");
            return 1;
        }

        if (save_revision_to_file(base_path, base) != 0) {
            fprintf(stderr, "Failed to save base revision\n");
            free_revision(base);
            return 1;
        }

        printf("Created base revision\n");
        free_revision(base);
    } else {
        // load base and create delta
        struct revision *base = load_revision_from_file(base_path);
        if (!base) {
            fprintf(stderr, "Failed to load base revision\n");
            return 1;
        }

        struct revision *delta = create_delta_revision(base, dir_path);
        if (!delta) {
            fprintf(stderr, "Failed to create delta revision\n");
            free_revision(base);
            return 1;
        }

        char delta_path[20];
        snprintf(delta_path, sizeof(delta_path), "revision_%d", delta->version);
        
        if (save_revision_to_file(delta_path, delta) != 0) {
            fprintf(stderr, "Failed to save delta revision\n");
            free_revision(delta);
            free_revision(base);
            return 1;
        }

        printf("Created delta revision %d\n", delta->version);
        free_revision(delta);
        free_revision(base);
    }

    return 0;
}