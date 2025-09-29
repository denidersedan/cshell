#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "executor.h"
#include "builtin.h"

int execute_command(char** tokens) {
    if (tokens == NULL || tokens[0] == NULL) return 1;

    if (is_builtin(tokens[0])) {
        return execute_builtin(tokens);   // builtins.c
    }

    // Detect background (& as last token)
    int background = 0;
    int last = 0;
    while (tokens[last] != NULL) last++;
    if (last > 0 && strcmp(tokens[last-1], "&") == 0) {
        background = 1;
        tokens[last-1] = NULL; // remove "&" so execvp doesn't see it
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Child: execute external command
        // Restore default signals if you install handlers in parent (optional)
        // signal(SIGINT, SIG_DFL); signal(SIGTSTP, SIG_DFL);

        execvp(tokens[0], tokens);
        // If execvp returns, it's an error
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // Parent
        if (!background) {
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
            }
        } else {
            // background: don't wait
            printf("[bg] pid %d\n", pid);
        }
    }
    return 1;
}
