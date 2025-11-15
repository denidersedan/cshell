#include "warp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WARP_FILE ".warpdirs"
#define MAX_LINE 1024

// get full path to warp file in HOME
static char* get_warp_file_path() {
    char* home = getenv("HOME");
    if (!home) return NULL;

    static char path[1024];
    snprintf(path, sizeof(path), "%s/%s", home, WARP_FILE);
    return path;
}

// add or update alias
static void warp_add(const char* alias) {
    char* path = get_warp_file_path();
    if (!path) {
        fprintf(stderr, "warp: cannot determine HOME\n");
        return;
    }

    FILE *fp = fopen(path, "r");
    FILE *tmp = fopen("/tmp/.warp_tmp", "w");
    char line[MAX_LINE];
    int found = 0;

    if (!tmp) {
        perror("warp");
        return;
    }

    // copy existing lines, update if alias exists
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            char cur_alias[MAX_LINE];
            char cur_path[MAX_LINE];
            if (sscanf(line, "%[^=]=%s", cur_alias, cur_path) == 2) {
                if (strcmp(cur_alias, alias) == 0) {
                    // alias already there, update it
                    char cwd[1024];
                    if (!getcwd(cwd, sizeof(cwd))) {
                        perror("getcwd");
                        return;
                    }
                    fprintf(tmp, "%s=%s\n", alias, cwd);
                    found = 1;
                } else {
                    // add entire line to the end
                    fputs(line, tmp);
                }
            }
        }
        fclose(fp);
    }

    if (!found) {
        char cwd[1024];
        if (!getcwd(cwd, sizeof(cwd))) {
            perror("getcwd");
            return;
        }
        fprintf(tmp, "%s=%s\n", alias, cwd);
    }

    fclose(tmp);
    rename("/tmp/.warp_tmp", path);
    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd))) {
        perror("getcwd");
        return;
    }
    printf("warp: added alias '%s' -> %s\n", alias, cwd);
}

// list all aliases
static void warp_list() {
    char* path = get_warp_file_path();
    if (!path) return;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        printf("No warp aliases defined.\n");
        return;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    fclose(fp);
}

// jump to alias
static void warp_go(const char* alias) {
    char* path = get_warp_file_path();
    if (!path) return;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "warp: alias '%s' not found\n", alias);
        return;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        char cur_alias[MAX_LINE], cur_path[MAX_LINE];
        if (sscanf(line, "%[^=]=%s", cur_alias, cur_path) == 2) {
            if (strcmp(cur_alias, alias) == 0) {
                if (chdir(cur_path) != 0) {
                    perror("warp");
                }
                fclose(fp);
                return;
            }
        }
    }

    fclose(fp);
    fprintf(stderr, "warp: alias '%s' not found\n", alias);
}

// warp command
int warp_command(char **tokens) {
    if (!tokens[1]) {
        printf("Usage: warp <alias> | warp add <alias> | warp list\n");
        return 1;
    }

    if (strcmp(tokens[1], "add") == 0) {
        if (!tokens[2]) {
            printf("Usage: warp add <alias>\n");
            return 1;
        }
        warp_add(tokens[2]);
        return 1;
    }

    if (strcmp(tokens[1], "list") == 0) {
        warp_list();
        return 1;
    }

    // else, warp <alias>
    warp_go(tokens[1]);
    return 1;
}
