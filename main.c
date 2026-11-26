#include "disk.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static int create_example(struct Disk *disk)
{
    disk_init(disk);  // This creates the root and leaves 31 slots free.
    struct Item *school = disk_allocate(disk);
    struct Item *notes = disk_allocate(disk);
    if (school == NULL || notes == NULL) {
        return 0;
    }

    school->type = DIRECTORY_ITEM;
    school->parent_id = ROOT_ID;  // The root contains /school.
    strcpy(school->name, "school");

    notes->parent_id = school->id;
    strcpy(notes->name, "notes.txt");
    memcpy(notes->content, "hello", 5);
    notes->size = 5;                // The file holds five bytes, with no terminator.
    notes->state = RECYCLED;
    strcpy(notes->original_path, "/school");
    notes->deleted_at = 1;          // A stand-in for the deletion timestamp.
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 3 || (strcmp(argv[1], "seed") != 0 && strcmp(argv[1], "show") != 0)) {
        fprintf(stderr, "Usage: %s seed|show image-path\n", argv[0]);
        return 1;
    }

    struct Disk disk;
    enum DiskResult loaded = disk_load(&disk, argv[2]);
    if (strcmp(argv[1], "seed") == 0) {
        if (loaded != DISK_MISSING) {
            fprintf(stderr, "Choose a new path for the demonstration image.\n");
            return 1;
        }
        if (!create_example(&disk) || disk_save(&disk, argv[2]) != DISK_OK) {
            fprintf(stderr, "Could not save the demonstration image.\n");
            return 1;
        }
        puts("Saved the example.");
        return 0;
    }

    if (loaded != DISK_OK) {
        fprintf(stderr, "Could not load %s (result %d).\n", argv[2], loaded);
        return 1;
    }

    for (int i = 0; i < MAX_ITEMS; ++i) {
        const struct Item *item = &disk.items[i];
        if (item->state == FREE) {
            continue;
        }
        printf("ID %" PRIu32 ", parent %" PRIu32 ": %s",
               item->id, item->parent_id, item->name);
        if (item->state == RECYCLED) {
            printf(" [recycled from %s]", item->original_path);
        }
        if (item->type == FILE_ITEM) {
            fputs(" = ", stdout);
            fwrite(item->content, 1, item->size, stdout);  // Use the stored byte count.
        }
        putchar('\n');
    }
    printf("Free slots: %d\n", disk_free_slots(&disk));
    return 0;
}
