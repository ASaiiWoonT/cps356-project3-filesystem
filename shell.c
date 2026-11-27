
#include "disk.h"
#include "fs.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT 1024
#define MAX_ARGS 10

// Parse commands and quoted arguments.
static int parse_command(char *input, char *args[])
{
    int argc = 0;
    char *p = input;

    while (*p != '\0') {

        while (isspace((unsigned char)*p)) {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        if (argc >= MAX_ARGS) {
            return -1;
        }

        if (*p == '"') {
            p++;
            args[argc++] = p;

            while (*p != '\0' && *p != '"') {
                p++;
            }

            if (*p != '"') {
                return -1;
            }

            *p = '\0';
            p++;

            if (*p != '\0' &&
                !isspace((unsigned char)*p)) {
                return -1;
            }
        }
        else {
            args[argc++] = p;

            while (*p != '\0' &&
                   !isspace((unsigned char)*p)) {

                if (*p == '"') {
                    return -1;
                }

                p++;
            }

            if (*p != '\0') {
                *p = '\0';
                p++;
            }
        }
    }

    return argc;
}

// Interactive shell loop.
void shell_run(struct Disk *disk, const char *disk_path)
{
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    char current_path[MAX_PATH_LENGTH];

    // Start in the root directory.
    uint32_t current_id = ROOT_ID;

    while (1) {

        // Display the current directory in the prompt.
        if (fs_get_path(disk, current_id,
                        current_path, sizeof(current_path))) {
            printf("os_shell%s> ", current_path);
        }
        else {
            printf("os_shell?> ");
        }

        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // Reject excessively long input.
        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            int ch;

            while ((ch = getchar()) != '\n' && ch != EOF) {
            }

            printf("Error: Input is too long.\n");
            continue;
        }

        input[strcspn(input, "\r\n")] = '\0';

        int argc = parse_command(input, args);

        if (argc < 0) {
            printf("Error: Invalid command format.\n");
            continue;
        }

        if (argc == 0) {
            continue;
        }

        // Create a directory.
        if (strcmp(args[0], "mkdir") == 0) {

            if (argc != 2) {
                printf("Usage: mkdir [dirname]\n");
                continue;
            }

            fs_mkdir(disk, current_id, args[1]);
        }

        // Change the current directory.
        else if (strcmp(args[0], "cd") == 0) {

            if (argc != 2) {
                printf("Usage: cd [path]\n");
                continue;
            }

            fs_cd(disk, &current_id, args[1]);
        }

        // Display the current directory.
        else if (strcmp(args[0], "pwd") == 0) {

            if (argc != 1) {
                printf("Usage: pwd\n");
                continue;
            }

            fs_pwd(disk, current_id);
        }

        // List directory contents.
        else if (strcmp(args[0], "ls") == 0) {

            if (argc != 1) {
                printf("Usage: ls\n");
                continue;
            }

            fs_ls(disk, current_id);
        }

        // Create an empty file.
        else if (strcmp(args[0], "touch") == 0) {

            if (argc != 2) {
                printf("Usage: touch [filename]\n");
                continue;
            }

            fs_touch(disk, current_id, args[1]);
        }

        // Write content into a file.
        else if (strcmp(args[0], "write") == 0) {

            if (argc != 3) {
                printf("Usage: write [filename] [content]\n");
                continue;
            }

            fs_write(disk, current_id, args[1], args[2]);
        }

        // Read file contents.
        else if (strcmp(args[0], "read") == 0) {

            if (argc != 2) {
                printf("Usage: read [filename]\n");
                continue;
            }

            fs_read(disk, current_id, args[1]);
        }

        // Copy a file.
        else if (strcmp(args[0], "cp") == 0) {

            if (argc != 3) {
                printf("Usage: cp [source] [destination]\n");
                continue;
            }

            fs_cp(disk, current_id, args[1], args[2]);
        }

        // Move or rename an item.
        else if (strcmp(args[0], "mv") == 0) {

            if (argc != 3) {
                printf("Usage: mv [source] [destination]\n");
                continue;
            }

            fs_mv(disk, current_id, args[1], args[2]);
        }

        // Save the virtual disk and exit.
        else if (strcmp(args[0], "exit") == 0) {

            if (argc != 1) {
                printf("Usage: exit\n");
                continue;
            }

            printf("Saving virtual file system ... ");

            if (disk_save(disk, disk_path) == DISK_OK) {
                printf("Done.\n");
                return;
            }

            printf("Failed.\n");
        }

        // Handle unknown commands.
        else {
            printf("Unknown command: %s\n", args[0]);
        }
    }
}
