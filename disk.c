#define _POSIX_C_SOURCE 200809L
#include "disk.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// The version identifies this exact record layout.
static const char signature[] = "CPS356-DISK-1";

void disk_init(struct Disk *disk)
{
    memset(disk, 0, sizeof *disk);  // FREE is zero, so every slot starts free.
    disk->next_id = ROOT_ID + 1;
    disk->items[0] = (struct Item){
        .id = ROOT_ID,
        .name = "/",
        .type = DIRECTORY_ITEM,
        .state = ACTIVE
    };
}

int disk_free_slots(const struct Disk *disk)
{
    int count = 0;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (disk->items[i].state == FREE) {
            ++count;  // Recycled items still hold their slot.
        }
    }
    return count;
}

static int find_slot(const struct Disk *disk, uint32_t id)
{
    for (int i = 0; i < MAX_ITEMS; ++i) {
        const struct Item *item = &disk->items[i];
        if (item->state != FREE && item->id == id) {
            return i;  // An ID can refer to an active or recycled item.
        }
    }
    return -1;
}

struct Item *disk_find(struct Disk *disk, uint32_t id)
{
    int slot = find_slot(disk, id);
    return slot == -1 ? NULL : &disk->items[slot];
}

struct Item *disk_allocate(struct Disk *disk)
{
    if (disk->next_id == UINT32_MAX) {
        return NULL;                // Never wrap around and reuse an old ID.
    }
    for (int i = 1; i < MAX_ITEMS; ++i) {  // Slot 0 belongs to the root.
        struct Item *item = &disk->items[i];
        if (item->state == FREE) {
            *item = (struct Item){.id = disk->next_id++, .state = ACTIVE};
            return item;            // Caller supplies the name, type and parent.
        }
    }
    return NULL;
}

int disk_release(struct Disk *disk, uint32_t id)
{
    struct Item *item = disk_find(disk, id);
    if (item == NULL || item->state != RECYCLED) {
        return 0;                   // Permanent deletion applies to bin items.
    }
    memset(item, 0, sizeof *item);   // State FREE is zero and the slot is reusable.
    return 1;
}

static int valid_snapshot(const struct Disk *disk)
{
    const struct Item *root = &disk->items[0];
    if (disk->next_id <= ROOT_ID || root->id != ROOT_ID || root->parent_id != 0 ||
        root->type != DIRECTORY_ITEM || root->state != ACTIVE ||
        root->name[0] != '/' || root->name[1] != '\0') {
        return 0;
    }

    for (int i = 0; i < MAX_ITEMS; ++i) {
        const struct Item *item = &disk->items[i];
        if (item->state == FREE) {
            continue;
        }
        if ((item->state != ACTIVE && item->state != RECYCLED) ||
            (item->type != FILE_ITEM && item->type != DIRECTORY_ITEM) ||
            item->id == 0 || item->id >= disk->next_id ||
            find_slot(disk, item->id) != i ||  // Reject a duplicate ID.
            item->size > MAX_CONTENT ||
            (item->type == DIRECTORY_ITEM && item->size != 0) ||
            item->name[0] == '\0' ||
            memchr(item->name, '\0', sizeof item->name) == NULL ||
            memchr(item->original_path, '\0', sizeof item->original_path) == NULL) {
            return 0;
        }
        if (i == 0 || item->state == RECYCLED) {
            continue;               // A recycled item's original parent may be gone.
        }

        uint32_t parent_id = item->parent_id;
        int hops = 0;
        // A valid active chain reaches root. The hop limit catches cycles.
        while (parent_id != ROOT_ID) {
            int parent_slot = find_slot(disk, parent_id);
            if (parent_slot == -1 || ++hops >= MAX_ITEMS) {
                return 0;
            }
            const struct Item *parent = &disk->items[parent_slot];
            if (parent->state != ACTIVE || parent->type != DIRECTORY_ITEM) {
                return 0;
            }
            parent_id = parent->parent_id;
        }
    }
    return 1;
}

enum DiskResult disk_load(struct Disk *disk, const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return errno == ENOENT ? DISK_MISSING : DISK_IO_ERROR;
    }

    char header[sizeof signature];
    struct Disk loaded;  // Read into a candidate before touching the live disk.
    int complete = fread(header, sizeof header, 1, file) == 1 &&
                   memcmp(header, signature, sizeof header) == 0 &&
                   fread(&loaded, sizeof loaded, 1, file) == 1 &&
                   fgetc(file) == EOF; // Reject a truncated or differently sized image.
    int read_error = ferror(file);
    int close_error = fclose(file);
    if (read_error || close_error != 0) {
        return DISK_IO_ERROR;
    }
    if (!complete || !valid_snapshot(&loaded)) {
        return DISK_BAD_IMAGE;
    }

    *disk = loaded;                 // Replace the in-memory state after validation.
    return DISK_OK;
}

enum DiskResult disk_save(const struct Disk *disk, const char *path)
{
    if (!valid_snapshot(disk)) {
        return DISK_BAD_IMAGE;  // Leave the last valid save in place.
    }

    static const char suffix[] = ".tmp.XXXXXX";
    size_t path_length = strlen(path);
    char *temporary = malloc(path_length + sizeof suffix);
    if (temporary == NULL) {
        return DISK_IO_ERROR;
    }
    memcpy(temporary, path, path_length);
    memcpy(temporary + path_length, suffix, sizeof suffix);

    // Write beside the disk image so rename can replace it in one operation.
    int fd = mkstemp(temporary);
    if (fd == -1) {
        free(temporary);
        return DISK_IO_ERROR;
    }

    FILE *file = fdopen(fd, "wb");
    int complete = 0;
    if (file == NULL) {
        close(fd);
    } else {
        complete = fwrite(signature, sizeof signature, 1, file) == 1 &&
                   fwrite(disk, sizeof *disk, 1, file) == 1;
        if (fclose(file) != 0) {
            complete = 0;          // Buffered writes can fail when closing.
        }
    }

    enum DiskResult result = DISK_IO_ERROR;
    if (complete && rename(temporary, path) == 0) {
        result = DISK_OK;
    } else {
        unlink(temporary);         // Keep the previous image if replacement failed.
    }
    free(temporary);
    return result;
}
