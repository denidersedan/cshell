#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "builtin.h"
#include "files.h"
#include "warp.h"
#include "history.h"
#include "builtin_jobs.h"

int is_builtin(char* command) {
    char* builtins[] = {"exit", "cd", "pwd", "help", "peek", "seek", "warp", "history", "fg", "bg", "jobs"};
    int num_builtins = sizeof(builtins) / sizeof(char*);
    for (int i = 0; i < num_builtins; i++) {
        if (strcmp(command, builtins[i]) == 0) return 1;
    }
    return 0;
}

int execute_builtin(char** tokens) {
    if (strcmp(tokens[0], "exit") == 0) return 0;

    if (strcmp(tokens[0], "cd") == 0) {
        if (tokens[1] == NULL) printf("No directory provided for cd!\n"); //chdir(getenv("HOME"));
        else if (chdir(tokens[1]) != 0) perror("cd");
        return 1;
    }

    if (strcmp(tokens[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
        else perror("pwd");
        return 1;
    }

    if (strcmp(tokens[0], "peek") == 0) {
        char* path = (tokens[1]) ? tokens[1] : ".";
        printf("%s\n", path);
        list_files_recursive_tree(path, "");
        return 1;
    }

    if (strcmp(tokens[0], "seek") == 0) {
        int name_filter = 0;
        int size_filter = 0;
        char *pattern = NULL;
        long int bytes = 0;
        char *path = ".";
        
        // Parse options in any order: -n <pattern>, -s <max_size>
        int i = 1;
        while (tokens[i] != NULL && tokens[i][0] == '-') {
            if (strcmp(tokens[i], "-n") == 0) {
                if (tokens[i+1] == NULL) {
                    printf("seek: option -n requires an argument\n");
                    printf("Usage: seek -n <filename_pattern> -s <max_size> [path]\n");
                    return 1;
                }
                pattern = tokens[i+1];
                name_filter = 1;
                i += 2;
            } else if (strcmp(tokens[i], "-s") == 0) {
                if (tokens[i+1] == NULL) {
                    printf("seek: option -s requires an argument\n");
                    printf("Usage: seek -n <filename_pattern> -s <max_size> [path]\n");
                    return 1;
                }
                char *endptr = NULL;
                bytes = strtol(tokens[i+1], &endptr, 10);
                if (endptr == tokens[i+1] || *endptr != '\0' || bytes < 0) {
                    printf("seek: invalid size '%s'\n", tokens[i+1]);
                    return 1;
                }
                size_filter = 1;
                i += 2;
            } else {
                printf("seek: unknown option '%s'\n", tokens[i]);
                printf("Usage: seek -n <filename_pattern> -s <max_size> [path]\n");
                return 1;
            }
        }

        // Next non-option token (if any) is the directory
        if (tokens[i] != NULL) {
            path = tokens[i];
            i++;
        }

        // If extra tokens after directory -> error
        if (tokens[i] != NULL) {
            printf("seek: too many arguments\n");
            printf("Usage: seek -n <filename_pattern> -s <max_size> [path]\n");
            return 1;
        }

        // Require at least one filter
        if (!name_filter && !size_filter) {
            printf("Usage: seek -n <filename_pattern> -s <max_size> [path]\n");
            return 1;
        }

        // Call file walker with both filters possible
        list_files(path, size_filter, bytes, name_filter, pattern);
        return 1;
    }

    if (strcmp(tokens[0], "warp") == 0) {
        return warp_command(tokens);
    }

    if (strcmp(tokens[0], "history") == 0) {
        print_history();
        return 1;
    }

    if (strcmp(tokens[0], "fg") == 0) return cmd_fg(tokens);
    if (strcmp(tokens[0], "bg") == 0) return cmd_bg(tokens);


    if (strcmp(tokens[0], "help") == 0) {
        printf("My Shell - Built-in commands:\n");
        printf("  cd [dir]      Change directory\n");
        printf("  pwd           Print working directory\n");
        printf("  peek [dir]    List files recursively\n");
        printf("  seek [flags]  Find files (-n name, -s size)\n");
        printf("  warp [dir]    Fast directory navigation\n");
        printf("  history       Show command history\n");
        printf("  jobs          List background jobs\n");
        printf("  fg <pid>      Bring job to foreground\n");
        printf("  bg <pid>      Continue job in background\n");
        printf("  exit          Exit shell\n");
        printf("  help          Show help\n");
        return 1;
    }
    return 1;
}
