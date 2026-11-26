#ifndef PROJECT3_FS_H
#define PROJECT3_FS_H

#include "disk.h"
#include <stddef.h>

// Maximum absolute path length.
#define MAX_PATH_LENGTH 1024

// Member 2 - Shared file system helper functions
struct Item *fs_find_child(struct Disk *disk,
                           uint32_t parent_id,
                           const char *name);

struct Item *fs_resolve_path(struct Disk *disk,
                             uint32_t current_id,
                             const char *path);

int fs_get_path(struct Disk *disk,
                uint32_t current_id,
                char *path,
                size_t path_size);

int fs_has_children(struct Disk *disk,
                    uint32_t directory_id);

// Member 2 - Directory operations
void fs_mkdir(struct Disk *disk,
              uint32_t current_id,
              const char *name);

void fs_cd(struct Disk *disk,
           uint32_t *current_id,
           const char *path);

void fs_pwd(struct Disk *disk,
            uint32_t current_id);

void fs_ls(struct Disk *disk,
           uint32_t current_id);

#endif // PROJECT3_FS_H
