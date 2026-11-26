#ifndef DISK_H
#define DISK_H

#include <stdint.h>

#define MAX_ITEMS 32     // Includes the root and all recycled records.
#define MAX_CONTENT 512  // Maximum bytes in one file.
#define ROOT_ID 1u       // Stable ID of the root directory.

enum ItemState { FREE = 0, ACTIVE, RECYCLED };  // A zeroed slot is free.
enum ItemType { FILE_ITEM, DIRECTORY_ITEM };

struct Item {
    uint32_t id;                   // A slot can be reused. Its old ID cannot.
    uint32_t parent_id;            // Parent ID, or 0 for the root.
    char name[32];
    enum ItemType type;
    enum ItemState state;
    uint32_t size;                 // Count the content bytes only.
    char content[MAX_CONTENT];
    char original_path[256];       // Parent path at removal, for display in the bin.
    int64_t deleted_at;            // Set by the Recycle Bin code.
};

struct Disk {
    uint32_t next_id;  // Save this so IDs stay fresh across restarts.
    struct Item items[MAX_ITEMS];
};

enum DiskResult { DISK_OK, DISK_MISSING, DISK_IO_ERROR, DISK_BAD_IMAGE };

void disk_init(struct Disk *disk);
int disk_free_slots(const struct Disk *disk);
struct Item *disk_find(struct Disk *disk, uint32_t id);
// Validate the command first, then fill the returned active record before saving.
// NULL means no slot or no new ID is available.
struct Item *disk_allocate(struct Disk *disk);
int disk_release(struct Disk *disk, uint32_t id);
enum DiskResult disk_save(const struct Disk *disk, const char *path);
enum DiskResult disk_load(struct Disk *disk, const char *path);

#endif
