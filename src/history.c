#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *history[MAX_HISTORY];
static int hist_count = 0;

void load_history(void) {
    FILE *file = fopen(HISTORY_FILE, "r");
    if (!file) return; // no history yet

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0; // remove newline
        if (hist_count < MAX_HISTORY) {
            history[hist_count++] = strdup(line);
        }
    }
    fclose(file);
}

void add_history(const char *cmd) {
    if (hist_count < MAX_HISTORY) {
        history[hist_count++] = strdup(cmd);
    } else {
        // Rolling history: remove oldest
        free(history[0]);
        for (int i = 1; i < MAX_HISTORY; i++)
            history[i-1] = history[i];
        history[MAX_HISTORY-1] = strdup(cmd);
    }

    // Append to file, trim if exceeding MAX_HISTORY_FILE
    FILE *file = fopen(HISTORY_FILE, "a+");
    if (!file) return;

    fprintf(file, "%s\n", cmd);

    // Check line count
    fseek(file, 0, SEEK_SET);
    int lines = 0;
    char buf[1024];
    while (fgets(buf, sizeof(buf), file)) lines++;

    if (lines > MAX_HISTORY_FILE) {
        // Rewrite last MAX_HISTORY_FILE commands
        char *lines_buffer[MAX_HISTORY_FILE];
        int idx = 0;

        rewind(file);
        while (fgets(buf, sizeof(buf), file)) {
            buf[strcspn(buf, "\n")] = 0;
            if (idx < MAX_HISTORY_FILE) {
                lines_buffer[idx++] = strdup(buf);
            } else {
                // shift
                free(lines_buffer[0]);
                for (int j = 1; j < MAX_HISTORY_FILE; j++)
                    lines_buffer[j-1] = lines_buffer[j];
                lines_buffer[MAX_HISTORY_FILE-1] = strdup(buf);
            }
        }

        fclose(file);
        file = fopen(HISTORY_FILE, "w");
        if (file) {
            for (int i = 0; i < idx; i++) {
                fprintf(file, "%s\n", lines_buffer[i]);
                free(lines_buffer[i]);
            }
            fclose(file);
        }
    } else {
        fclose(file);
    }
}

void save_history(void) {
    // Optional: rewrite full history (if you want trimming)
    FILE *file = fopen(HISTORY_FILE, "w");
    if (!file) return;
    for (int i = 0; i < hist_count; i++) {
        fprintf(file, "%s\n", history[i]);
        free(history[i]); // free memory
    }
    fclose(file);
    hist_count = 0;
}

void print_history(void) {
    int start = hist_count > MAX_HISTORY_FILE ? hist_count - MAX_HISTORY_FILE : 0;
    for (int i = start; i < hist_count; i++) {
        printf("%d  %s\n", i+1, history[i]);
    }
}

int history_count(void) {
    return hist_count;
}

const char *history_get(int idx) {
    if (idx < 0 || idx >= hist_count) return NULL;
    return history[idx];
}
