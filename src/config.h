#ifndef CONFIG_H
#define CONFIG_H

struct config {
        char *revisions;
        int compress_files;
};

void serialize_config(const struct config *cfg, const char *filename);
void deserialize_config(struct config *cfg, const char *filename);

#endif
