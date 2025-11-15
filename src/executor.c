#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "executor.h"
#include "builtin.h"
#include "jobs.h"
#include "input.h"

struct sigaction old_int, old_tstp, sa_ignore;

char* reconstruct_cmdline(char** tokens) {
    if (!tokens || !tokens[0]) return NULL;

    // First, find total length needed
    size_t len = 0;
    for (int i = 0; tokens[i] != NULL; i++) {
        len += strlen(tokens[i]) + 1; // +1 for space or '\0'
    }

    char* cmdline = malloc(len);
    if (!cmdline) return NULL;

    cmdline[0] = '\0'; // start with empty string

    // Build the string
    for (int i = 0; tokens[i] != NULL; i++) {
        strcat(cmdline, tokens[i]);
        if (tokens[i + 1] != NULL)
            strcat(cmdline, " ");
    }

    return cmdline;
}


int execute_command(char** tokens) {
    if (tokens == NULL || tokens[0] == NULL) return 1;

    if (is_builtin(tokens[0])) {
        return execute_builtin(tokens);
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
        setpgid(0, 0);
        // child: execute external command
        
        signal(SIGINT, SIG_DFL); 
        signal(SIGTSTP, SIG_DFL);

        execvp(tokens[0], tokens);
        
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // parent
        char *input_line = reconstruct_cmdline(tokens);
        setpgid(pid, pid);
        if (!background) {

            pid_t shell_pgid = getpgrp();
            tcsetpgrp(STDIN_FILENO, pid); 

            sa_ignore.sa_handler = SIG_IGN;
            sigaction(SIGINT, &sa_ignore, &old_int);
            sigaction(SIGTSTP, &sa_ignore, &old_tstp);
            
            int status;
            waitpid(pid, &status, WUNTRACED);

            tcsetpgrp(STDIN_FILENO, shell_pgid);
            sigaction(SIGINT, &old_int, NULL);
            sigaction(SIGTSTP, &old_tstp, NULL);

            if (WIFSTOPPED(status)) {
                // capture the pointer to the new job
                job_t* job = add_job(pid, input_line, 1); // mark as stopped
    
                // print the job's ID
                if (job) {
                    printf("\n[%d]+ Stopped %s\n", job->id, job->cmdline);
                }
            }
        } else {
            // background: don't wait
            job_t *back_job = add_job(pid, input_line, 0);
            if (back_job) {
                printf("[bg] job [%d]\n", back_job->id);
            }
        }
        free(input_line);
    }
    return 1;
}
