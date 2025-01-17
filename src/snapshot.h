#ifndef SNAPSHOT_H
#define SNAPSHOT_H

int create_snapshot(const char *dir_path);
int restore_snapshot(const char *dir_path, const int version);

#endif
