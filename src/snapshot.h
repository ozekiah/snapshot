#ifndef SNAPSHOT_H
#define SNAPSHOT_H

int create_snapshot(const char *dir_path);
int restore_snapshot(const char *dir_path, const int version);
int discard_snapshot(const char *dir_path);
int list_snapshot(const char *dir_path);

#endif
