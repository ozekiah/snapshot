#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "config.h"

void serialize_config(const struct config *cfg, const char *filename)
{
        FILE *file = fopen(filename, "w");
        if (!file) {
                perror("Failed to open file for writing");
                return;
        }

        yaml_emitter_t emitter;
        yaml_event_t event;

        if (!yaml_emitter_initialize(&emitter)) {
                fprintf(stderr, "Failed to initialize emitter\n");
                fclose(file);
                return;
        }

        yaml_emitter_set_output_file(&emitter, file);
        yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);

        yaml_emitter_emit(&emitter, &event);
        yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 0);
        yaml_emitter_emit(&emitter, &event);

        yaml_mapping_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE);
        yaml_emitter_emit(&emitter, &event);
        yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char *)"path", strlen("path"), 1, 1, YAML_PLAIN_SCALAR_STYLE);
        yaml_emitter_emit(&emitter, &event);
        yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char *)cfg->path, strlen(cfg->path), 1, 1, YAML_PLAIN_SCALAR_STYLE);
        yaml_emitter_emit(&emitter, &event);
        yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char *)"format", strlen("format"), 1, 1, YAML_PLAIN_SCALAR_STYLE);
        yaml_emitter_emit(&emitter, &event);
        yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char *)cfg->format, strlen(cfg->format), 1, 1, YAML_PLAIN_SCALAR_STYLE);
        yaml_emitter_emit(&emitter, &event);
        yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char *)"limit", strlen("limit"), 1, 1, YAML_PLAIN_SCALAR_STYLE);
        yaml_emitter_emit(&emitter, &event);

        char limit_str[20];
        snprintf(limit_str, sizeof(limit_str), "%d", cfg->limit);
        yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char *)limit_str, strlen(limit_str), 1, 1, YAML_PLAIN_SCALAR_STYLE);
        yaml_emitter_emit(&emitter, &event);
        yaml_mapping_end_event_initialize(&event);
        yaml_emitter_emit(&emitter, &event);
        yaml_document_end_event_initialize(&event, 0);
        yaml_emitter_emit(&emitter, &event);
        yaml_stream_end_event_initialize(&event);
        yaml_emitter_emit(&emitter, &event);
        yaml_emitter_delete(&emitter);
        fclose(file);
}

void deserialize_config(struct config *cfg, const char *filename) 
{
        FILE *file = fopen(filename, "r");
        if (!file) {
                perror("Failed to open file for reading");
                return;
        }

        yaml_parser_t parser;
        yaml_event_t event;

        if (!yaml_parser_initialize(&parser)) {
                fprintf(stderr, "Failed to initialize parser\n");
                fclose(file);
                return;
        }

        yaml_parser_set_input_file(&parser, file);

        char key[50] = {0};
        while (yaml_parser_parse(&parser, &event)) {
                if (event.type == YAML_SCALAR_EVENT) {
                        const char *value = (const char *)event.data.scalar.value;

                        if (strlen(key) == 0) {
                                strncpy(key, value, sizeof(key) - 1);
                                key[sizeof(key) - 1] = '\0';
                        } else {
                                if (strcmp(key, "path") == 0) {
                                cfg->path = strdup(value);
                        } else if (strcmp(key, "format") == 0) {
                                cfg->format = strdup(value);
                        } else if (strcmp(key, "limit") == 0) {
                                cfg->limit = atoi(value);
                        }
                        
                        key[0] = '\0'; 
                    }
                }

                if (event.type == YAML_STREAM_END_EVENT) {
                        break;
                }

                yaml_event_delete(&event);
        }

        yaml_parser_delete(&parser);
        fclose(file);
}
