/* main.c — updated */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/wait.h>

#include "include/parser.h"
#include "include/executor.h"
#include "include/history.h"
#include "include/input.h"
#include "include/jobs.h"

// globals used by terminal/job control
static pid_t shell_pgid;
static struct termios shell_tmodes;

// simple flag you can check in input.c if you want to react to SIGINT while editing
volatile sig_atomic_t got_sigint = 0;

// SIGINT handler for the shell: just set a flag
static void sigint_handler(int signo) {
    (void)signo;
    got_sigint = 1;
}

// SIGCHLD handler: reap children (non-blocking)
static void sigchld_handler(int signo) {
    (void)signo;
    int saved_errno = errno;
    while (1) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (pid <= 0) break;

        job_t* job = find_job_by_pgid(pid);
        if (job) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                // job terminated (exited or killed)
                fprintf(stderr, "\n[%d] Done    %s\n", job->id, job->cmdline);
                remove_job(job); // Remove from list
            } else if (WIFSTOPPED(status)) {
                // job was stopped
                fprintf(stderr, "\n[%d] Stopped %s\n", job->id, job->cmdline);
                job->stopped = 1;
            } else if (WIFCONTINUED(status)) {
                // job was continued
                job->stopped = 0;
            }
        }
    }
    errno = saved_errno;
}

// initialize shell process group, take terminal control, and install handlers
static void init_shell(void) {
    // put shell in its own process group
    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0 && errno != EACCES) {
        perror("setpgid");
    }

    // take control of the terminal
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
        perror("tcsetpgrp");
    }

    // save current terminal modes so we can restore them after running jobs
    if (tcgetattr(STDIN_FILENO, &shell_tmodes) < 0) {
    }

    // ignore terminal stop signals in the shell itself so it doesn't get suspended
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    // install SIGCHLD handler to reap background children (use sigaction)
    struct sigaction sa_chld;
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa_chld, NULL) < 0) {
        perror("sigaction(SIGCHLD)");
    }

    // install a lightweight SIGINT handler for the shell
    struct sigaction sa_int;
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_int, NULL) < 0) {
        perror("sigaction(SIGINT)");
    }
}

int main(void) {
    char *input;
    char **tokens;
    int status = 1;

    load_history();

    // make the shell its own process group and set up terminal / handlers before switching terminal modes or entering the input loop.
    init_shell();

    // input_init() sets raw-ish mode for line editing
    input_init();

    while (status) {
        fflush(stdout);

        input = read_input_line("$ ");
        if (!input) {
            printf("\n");
            break;
        }

        if (input[0] != '\0') add_history(input);

        tokens = parse_input(input);

        if (tokens && tokens[0]) {
            status = execute_command(tokens);
        }

        if (tokens) {
            free(tokens);
        }
        free(input);
    }

    save_history();

    // restore terminal modes and release
    tcsetattr(STDIN_FILENO, TCSANOW, &shell_tmodes);
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    input_restore();

    return 0;
}
