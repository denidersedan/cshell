#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "files.h"

void list_files(char *path, int size_filter, long int bytes, int name_filter, char *string) {
    struct dirent *entry;
    struct stat file_meta;
    DIR *dir = opendir(path);

    if (!dir) {
        printf("ERROR: invalid directory path\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char full_path[1024];
        if (path[strlen(path) - 1] == '/') snprintf(full_path, sizeof(full_path), "%s%s", path, entry->d_name);
        else snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &file_meta) < 0) {
            perror("stat");
            continue;
        }

        int ok = 1;

        if (size_filter) {
            if (!S_ISREG(file_meta.st_mode) || (long int)file_meta.st_size >= bytes) {
                ok = 0;
            }
        }

        
        if (name_filter == 1 && string != NULL) {
            int string_len = strlen(string);
            int file_len = strlen(entry->d_name);
            if (string_len > file_len) {
                ok = 0;
            } else {
                if(strncmp(entry->d_name, string, string_len) != 0) {
                    ok = 0;
                }
            }
        }
        
        if (ok) {
            printf("%s\n", full_path);
        }
    }
    closedir(dir);
}

void list_files_recursive_tree(char *path, char *prefix) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "ERROR: cannot open directory '%s'\n", path);
        return;
    }

    struct dirent *entry;
    struct stat file_meta;

    // First collect entries (skip "." and "..") so we know which is last
    char **names = NULL;
    size_t count = 0, cap = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (count + 1 > cap) {
            cap = cap ? cap * 2 : 16;
            names = realloc(names, cap * sizeof(char *));
            if (!names) { perror("realloc"); closedir(dir); return; }
        }
        names[count++] = strdup(entry->d_name);
    }
    closedir(dir);

    // Sort entries alphabetically (optional, remove if you don't want sorting)
    if (count > 1) {
        qsort(names, count, sizeof(char*), (int(*)(const void*,const void*)) strcmp);
    }

    // Iterate entries and print with tree characters
    for (size_t i = 0; i < count; ++i) {
        char *name = names[i];
        int is_last = (i == count - 1);

        // Build full path
        char full_path[PATH_MAX];
        if (snprintf(full_path, sizeof(full_path), "%s/%s", path, name) >= (int)sizeof(full_path)) {
            fprintf(stderr, "Path too long: %s/%s\n", path, name);
            free(name);
            continue;
        }

        // Get metadata
        if (stat(full_path, &file_meta) < 0) {
            perror("stat");
            free(name);
            continue;
        }

        // Print this entry with branch characters
        printf("%s%s%s\n", prefix, (is_last ? "└── " : "├── "), name);

        // If directory, recurse with extended prefix
        if (S_ISDIR(file_meta.st_mode)) {
            // new_prefix = prefix + (is_last ? "    " : "│   ")
            char new_prefix[PATH_MAX];
            if (snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, (is_last ? "    " : "│   ")) >= (int)sizeof(new_prefix)) {
                fprintf(stderr, "Prefix too long\n");
            } else {
                list_files_recursive_tree(full_path, new_prefix);
            }
        }

        free(name);
    }

    free(names);
}