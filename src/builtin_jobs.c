#define _POSIX_C_SOURCE 200809L

#include "builtin_jobs.h"
#include "jobs.h"
#include "input.h"

// parse "%N" or "N" job spec (returns -1 if error)
int parse_job_spec(const char *s) {
    if (!s) return -1;
    if (s[0] == '%') s++;
    if (*s == '\0') return -1;
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') return -1;
    return (int)v;
}

int cmd_fg(char **tokens) {
    // tokens[0] == "fg", tokens[1] should be "%N" or N 
    if (!tokens || !tokens[1]) {
        fprintf(stderr, "fg: usage: fg %%<jobid>\n");
        return 1;
    }
    int jid = parse_job_spec(tokens[1]);
    if (jid <= 0) {
        fprintf(stderr, "fg: invalid job id: %s\n", tokens[1]);
        return 1;
    }
    job_t *job = find_job_by_id(jid);
    if (!job) {
        fprintf(stderr, "fg: no such job: %d\n", jid);
        return 1;
    }

    pid_t pgid = job->pgid;

    input_restore();  // restore terminal canonical mode for job control

    // give terminal control to job's process group
    if (tcsetpgrp(STDIN_FILENO, pgid) < 0) {
        perror("tcsetpgrp");
    }

    // if stopped, send CONTINUE signal to every job in the process group
    if (job->stopped) {
        if (kill(-pgid, SIGCONT) < 0) {
            perror("kill(SIGCONT)");
        }
    }

    // wait for the job to finish or stop again
    int status;
    pid_t w;
    do {
        w = waitpid(-pgid, &status, WUNTRACED);
    } while (w == -1 && errno == EINTR);

    // after wait, restore terminal to shell's pgid (shell should be in its pgid already)
    tcsetpgrp(STDIN_FILENO, getpgrp());

    input_init();  // re-enable raw mode for shell

    if (w <= 0) {
        if (errno != ECHILD && errno != 0) perror("waitpid");
    }

    if (WIFSTOPPED(status)) {
        job->stopped = 1;
        printf("\n[%d]+ Stopped  %s\n", job->id, job->cmdline);
        return 1;
    } else {
        // finished: remove job
        remove_job(job);
        return 1;
    }
}

int cmd_bg(char **tokens) {
    if (!tokens || !tokens[1]) {
        fprintf(stderr, "bg: usage: bg %%<jobid>\n");
        return 1;
    }
    int jid = parse_job_spec(tokens[1]);
    if (jid <= 0) {
        fprintf(stderr, "bg: invalid job id: %s\n", tokens[1]);
        return 1;
    }
    job_t *job = find_job_by_id(jid);
    if (!job) {
        fprintf(stderr, "bg: no such job: %d\n", jid);
        return 1;
    }

    if (job->stopped) {
        if (kill(-job->pgid, SIGCONT) < 0) {
            perror("kill(SIGCONT)");
            return 1;
        }
        job->stopped = 0;
        printf("[%d]+ %s &\n", job->id, job->cmdline);
    } else {
        printf("bg: job %d is not stopped\n", job->id);
    }
    return 1;
}
