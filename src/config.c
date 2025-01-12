#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define MAX_LINE_LENGTH 256

void trim_whitespace(char* str) {
        int start = 0;
        int end = strlen(str) - 1;

        while (str[start] == ' ' || str[start] == '\t') {
                start++;
        }

        while (end >= start && (str[end] == ' ' || str[end] == '\t')) {
                end--;
        }

        str[end + 1] = '\0';
        if (start > 0) {
                memmove(str, str + start, end - start + 2);
        }
}

int read_ini(const char* filename, struct config* cfg) {
        FILE* file = fopen(filename, "r");
        if (!file) {
                perror("Error opening file");
                return -1;
        }

        char line[MAX_LINE_LENGTH];
        char current_section[MAX_LINE_LENGTH] = "";

        while (fgets(line, sizeof(line), file)) {
                trim_whitespace(line);

                if (line[0] == '\0' || line[0] == ';' || line[0] == '#') {
                        continue;
                }

                if (line[0] == '[') {
                        char* section_end = strchr(line, ']');
                        if (section_end) {
                                *section_end = '\0'; 
                                strcpy(current_section, line + 1); 
                        }
                        continue;
                }

                char* key = strtok(line, "=");
                char* value = strtok(NULL, "=");

                if (key && value) {
                    trim_whitespace(key);
                    trim_whitespace(value);

                    if (strcmp(current_section, "general") == 0) {
                            if (strcmp(key, "path") == 0) {
                                strncpy(cfg->path, value, sizeof(cfg->path) - 1);
                            } else if (strcmp(key, "format") == 0) {
                                strncpy(cfg->format, value, sizeof(cfg->format) - 1);
                            } else if (strcmp(key, "limit") == 0) {
                                cfg->limit = atoi(value); 
                            }
                    }
                }
        }

        fclose(file);
        return 0;
}
