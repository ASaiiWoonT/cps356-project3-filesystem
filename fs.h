#ifndef PROJECT3_FS_H
#define PROJECT3_FS_H

#include "disk.h"
#include <stddef.h>

#define MAX_PATH_LENGTH 1024

// Shared file system helper functions
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

// Directory operations
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

// File operations
void fs_touch(struct Disk *disk,
              uint32_t current_id,
              const char *name);

void fs_write(struct Disk *disk,
              uint32_t current_id,
              const char *path,
              const char *content);

void fs_read(struct Disk *disk,
             uint32_t current_id,
             const char *path);
// Copy and move operations
void fs_cp(struct Disk *disk,
           uint32_t current_id,
           const char *source,
           const char *destination);

void fs_mv(struct Disk *disk,
           uint32_t current_id,
           const char *source,
           const char *destination);

#endif // PROJECT3_FS_H
