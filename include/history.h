#ifndef HISTORY_H
#define HISTORY_H

#define HISTORY_FILE ".cshell_history"
#define MAX_HISTORY 1000
#define MAX_HISTORY_FILE 100


void load_history(void);
void add_history(const char *cmd);
void save_history(void);
void print_history(void);
int history_count(void);
const char *history_get(int idx);
int history_last_index(void);

#endif
