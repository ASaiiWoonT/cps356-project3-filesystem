#include "fs.h"
#include <stdio.h>
#include <string.h>

// Find an active child inside a directory.
struct Item *fs_find_child(struct Disk *disk,
                           uint32_t parent_id,
                           const char *name)
{
    for (int i = 0; i < MAX_ITEMS; i++) {
        struct Item *item = &disk->items[i];

        if (item->state == ACTIVE &&
            item->parent_id == parent_id &&
            strcmp(item->name, name) == 0) {
            return item;
        }
    }

    return NULL;
}

// Check whether a directory contains active items.
int fs_has_children(struct Disk *disk,
                    uint32_t directory_id)
{
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (disk->items[i].state == ACTIVE &&
            disk->items[i].parent_id == directory_id) {
            return 1;
        }
    }

    return 0;
}

// Resolve absolute and relative paths.
struct Item *fs_resolve_path(struct Disk *disk,
                             uint32_t current_id,
                             const char *path)
{
    char copy[MAX_PATH_LENGTH];

    if (path == NULL || path[0] == '\0' ||
        strlen(path) >= sizeof(copy)) {
        return NULL;
    }

    strcpy(copy, path);

    uint32_t id = current_id;

    if (path[0] == '/') {
        id = ROOT_ID;
    }

    struct Item *current = disk_find(disk, id);

    if (current == NULL || current->state != ACTIVE) {
        return NULL;
    }

    char *part = strtok(copy, "/");

    while (part != NULL) {

        if (current->type != DIRECTORY_ITEM) {
            return NULL;
        }

        if (strcmp(part, ".") == 0) {
            // Stay in the current directory.
        }
        else if (strcmp(part, "..") == 0) {

            if (current->id != ROOT_ID) {
                current = disk_find(disk,
                                    current->parent_id);
            }
        }
        else {
            current = fs_find_child(disk,
                                    current->id,
                                    part);
        }

        if (current == NULL ||
            current->state != ACTIVE) {
            return NULL;
        }

        part = strtok(NULL, "/");
    }

    if (path[strlen(path) - 1] == '/' &&
        current->type != DIRECTORY_ITEM) {
        return NULL;
    }

    return current;
}

// Build the absolute path using parent IDs.
int fs_get_path(struct Disk *disk,
                uint32_t current_id,
                char *path,
                size_t path_size)
{
    const char *names[MAX_ITEMS];
    int count = 0;

    struct Item *item = disk_find(disk, current_id);

    while (item != NULL && item->id != ROOT_ID) {

        if (item->state != ACTIVE ||
            count >= MAX_ITEMS) {
            return 0;
        }

        names[count++] = item->name;
        item = disk_find(disk, item->parent_id);
    }

    if (item == NULL || item->state != ACTIVE ||
        path_size < 2) {
        return 0;
    }

    strcpy(path, "/");

    for (int i = count - 1; i >= 0; i--) {

        size_t required = strlen(path)
                        + strlen(names[i])
                        + (i > 0 ? 1 : 0)
                        + 1;

        if (required > path_size) {
            return 0;
        }

        strcat(path, names[i]);

        if (i > 0) {
            strcat(path, "/");
        }
    }

    return 1;
}

// Validate a file or directory name.
static int valid_name(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        return 0;
    }

    if (strlen(name) >= sizeof(((struct Item *)0)->name)) {
        return 0;
    }

    if (strchr(name, '/') != NULL ||
        strcmp(name, ".") == 0 ||
        strcmp(name, "..") == 0) {
        return 0;
    }

    return 1;
}

// Create a new directory.
void fs_mkdir(struct Disk *disk,
              uint32_t current_id,
              const char *name)
{
    if (!valid_name(name)) {
        printf("Error: Invalid directory name.\n");
        return;
    }

    if (fs_find_child(disk, current_id, name) != NULL) {
        printf("Error: An item with this name already exists.\n");
        return;
    }

    struct Item *item = disk_allocate(disk);

    if (item == NULL) {
        printf("Error: Disk full or no available ID.\n");
        return;
    }

    item->parent_id = current_id;
    item->type = DIRECTORY_ITEM;
    item->state = ACTIVE;
    item->size = 0;

    strcpy(item->name, name);

    printf("Directory created successfully.\n");
}

// Change the current directory.
void fs_cd(struct Disk *disk,
           uint32_t *current_id,
           const char *path)
{
    struct Item *item =
        fs_resolve_path(disk, *current_id, path);

    if (item == NULL) {
        printf("Error: Directory not found.\n");
        return;
    }

    if (item->type != DIRECTORY_ITEM) {
        printf("Error: Not a directory.\n");
        return;
    }

    *current_id = item->id;
}

// Display the current absolute path.
void fs_pwd(struct Disk *disk,
            uint32_t current_id)
{
    char path[MAX_PATH_LENGTH];

    if (fs_get_path(disk, current_id,
                    path, sizeof(path))) {
        printf("%s\n", path);
    }
    else {
        printf("Error: Could not determine path.\n");
    }
}

// List active items in the current directory.
void fs_ls(struct Disk *disk,
           uint32_t current_id)
{
    int found = 0;

    for (int i = 0; i < MAX_ITEMS; i++) {

        struct Item *item = &disk->items[i];

        if (item->state != ACTIVE ||
            item->parent_id != current_id) {
            continue;
        }

        found = 1;

        if (item->type == DIRECTORY_ITEM) {
            printf("\"%s\" [Directory]\n",
                   item->name);
        }
        else {
            printf("\"%s\" (%lu bytes)\n",
                   item->name,
                   (unsigned long)item->size);
        }
    }

    if (!found) {
        printf("Directory is empty.\n");
    }
}

// Create an empty file.
void fs_touch(struct Disk *disk,
              uint32_t current_id,
              const char *name)
{
    if (!valid_name(name)) {
        printf("Error: Invalid file name.\n");
        return;
    }

    if (fs_find_child(disk, current_id, name) != NULL) {
        printf("Error: An item with this name already exists.\n");
        return;
    }

    struct Item *item = disk_allocate(disk);

    if (item == NULL) {
        printf("Error: Disk full or no available ID.\n");
        return;
    }

    item->parent_id = current_id;
    item->type = FILE_ITEM;
    item->state = ACTIVE;
    item->size = 0;

    strcpy(item->name, name);
    memset(item->content, 0, sizeof(item->content));

    printf("File created successfully.\n");
}

// Write content into an existing file.
void fs_write(struct Disk *disk,
              uint32_t current_id,
              const char *path,
              const char *content)
{
    struct Item *item =
        fs_resolve_path(disk, current_id, path);

    if (item == NULL) {
        printf("Error: File not found.\n");
        return;
    }

    if (item->type != FILE_ITEM) {
        printf("Error: Not a file.\n");
        return;
    }

    size_t length = strlen(content);

    if (length > MAX_CONTENT) {
        printf("Error: Content exceeds the maximum file size.\n");
        return;
    }

    memset(item->content, 0, sizeof(item->content));
    memcpy(item->content, content, length);

    item->size = (uint32_t)length;

    printf("File written successfully.\n");
}

// Read the contents of an existing file.
void fs_read(struct Disk *disk,
             uint32_t current_id,
             const char *path)
{
    struct Item *item =
        fs_resolve_path(disk, current_id, path);

    if (item == NULL) {
        printf("Error: File not found.\n");
        return;
    }

    if (item->type != FILE_ITEM) {
        printf("Error: Not a file.\n");
        return;
    }

    fwrite(item->content, 1, item->size, stdout);
    putchar('\n');
}

// Resolve the destination directory and item name.
static int resolve_destination(struct Disk *disk,
                               uint32_t current_id,
                               const char *destination,
                               const char *source_name,
                               uint32_t *parent_id,
                               char *name)
{
    if (destination == NULL ||
        destination[0] == '\0' ||
        strlen(destination) >= MAX_PATH_LENGTH) {
        printf("Error: Invalid destination.\n");
        return 0;
    }

    struct Item *target =
        fs_resolve_path(disk, current_id, destination);

    // Destination is an existing directory.
    if (target != NULL) {

        if (target->type != DIRECTORY_ITEM) {
            printf("Error: Destination already exists.\n");
            return 0;
        }

        *parent_id = target->id;
        strcpy(name, source_name);

        return 1;
    }

    char path[MAX_PATH_LENGTH];
    strcpy(path, destination);

    char *slash = strrchr(path, '/');
    const char *new_name = path;

    *parent_id = current_id;

    if (slash != NULL) {

        new_name = slash + 1;

        if (!valid_name(new_name)) {
            printf("Error: Invalid destination name.\n");
            return 0;
        }

        strcpy(name, new_name);

        if (slash == path) {
            *parent_id = ROOT_ID;
        }
        else {
            *slash = '\0';

            struct Item *parent =
                fs_resolve_path(disk, current_id, path);

            if (parent == NULL ||
                parent->type != DIRECTORY_ITEM) {
                printf("Error: Destination directory not found.\n");
                return 0;
            }

            *parent_id = parent->id;
        }
    }
    else {

        if (!valid_name(new_name)) {
            printf("Error: Invalid destination name.\n");
            return 0;
        }

        strcpy(name, new_name);
    }

    return 1;
}

// Copy an existing file.
void fs_cp(struct Disk *disk,
           uint32_t current_id,
           const char *source,
           const char *destination)
{
    struct Item *item =
        fs_resolve_path(disk, current_id, source);

    if (item == NULL) {
        printf("Error: Source file not found.\n");
        return;
    }

    if (item->type != FILE_ITEM) {
        printf("Error: Only files can be copied.\n");
        return;
    }

    uint32_t parent_id;
    char name[32];

    if (!resolve_destination(disk, current_id,
                             destination, item->name,
                             &parent_id, name)) {
        return;
    }

    if (fs_find_child(disk, parent_id, name) != NULL) {
        printf("Error: Destination already exists.\n");
        return;
    }

    struct Item *copy = disk_allocate(disk);

    if (copy == NULL) {
        printf("Error: Disk full or no available ID.\n");
        return;
    }

    copy->parent_id = parent_id;
    copy->type = FILE_ITEM;
    copy->state = ACTIVE;
    copy->size = item->size;

    strcpy(copy->name, name);
    memset(copy->content, 0, sizeof(copy->content));
    memcpy(copy->content, item->content, item->size);

    printf("File copied successfully.\n");
}

// Move or rename an existing file or directory.
void fs_mv(struct Disk *disk,
           uint32_t current_id,
           const char *source,
           const char *destination)
{
    struct Item *item =
        fs_resolve_path(disk, current_id, source);

    if (item == NULL) {
        printf("Error: Source item not found.\n");
        return;
    }

    if (item->id == ROOT_ID) {
        printf("Error: Cannot move the root directory.\n");
        return;
    }

    uint32_t parent_id;
    char name[32];

    if (!resolve_destination(disk, current_id,
                             destination, item->name,
                             &parent_id, name)) {
        return;
    }

    if (fs_find_child(disk, parent_id, name) != NULL) {
        printf("Error: Destination already exists.\n");
        return;
    }

    // Prevent moving a directory into itself
    // or one of its own subdirectories.
    if (item->type == DIRECTORY_ITEM) {

        uint32_t check_id = parent_id;
        int count = 0;

        while (check_id != 0) {

            if (check_id == item->id) {
                printf("Error: Cannot move a directory into itself.\n");
                return;
            }

            if (count++ >= MAX_ITEMS) {
                printf("Error: Invalid directory structure.\n");
                return;
            }

            struct Item *parent =
                disk_find(disk, check_id);

            if (parent == NULL ||
                parent->state != ACTIVE ||
                parent->type != DIRECTORY_ITEM) {
                printf("Error: Invalid destination directory.\n");
                return;
            }

            check_id = parent->parent_id;
        }
    }

    item->parent_id = parent_id;
    strcpy(item->name, name);

    printf("Item moved successfully.\n");
}
