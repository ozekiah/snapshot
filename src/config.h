#ifndef CONFIG_H
#define CONFIG_H

struct config {
        char *path;
        char *format;
        int limit;
};

void serialize_config(const struct config *cfg, const char *filename);
void deserialize_config(struct config *cfg, const char *filename);

#endif
