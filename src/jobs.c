// jobs.c
#define _POSIX_C_SOURCE 200809L
#include "jobs.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* Simple linked-list job table (single-threaded shell) */
static job_t *jobs_head = NULL;
static int next_job_id = 1;

job_t* add_job(pid_t pgid, const char *cmdline, int stopped) {
    job_t *j = malloc(sizeof(job_t));
    if (!j) return NULL;
    j->id = next_job_id++;
    j->pgid = pgid;
    j->cmdline = cmdline ? strdup(cmdline) : strdup("");
    j->stopped = stopped ? 1 : 0;
    j->next = jobs_head;
    jobs_head = j;
    return j;
}

job_t* find_job_by_id(int id) {
    for (job_t *j = jobs_head; j; j = j->next) {
        if (j->id == id) return j;
    }
    return NULL;
}

job_t* find_job_by_pgid(pid_t pgid) {
    for (job_t *j = jobs_head; j; j = j->next) {
        if (j->pgid == pgid) return j;
    }
    return NULL;
}

void remove_job(job_t *job) {
    if (!job) return;
    job_t **pp = &jobs_head;
    while (*pp) {
        if (*pp == job) {
            *pp = job->next;
            free(job->cmdline);
            free(job);
            return;
        }
        pp = &((*pp)->next);
    }
}

/* mark/clear stopped flag */
void mark_job_stopped(job_t *job) {
    if (job) job->stopped = 1;
}
void mark_job_running(job_t *job) {
    if (job) job->stopped = 0;
}

/* print concise jobs list; you can improve format as desired */
void print_jobs(void) {
    for (job_t *j = jobs_head; j; j = j->next) {
        printf("[%d] %s  %s\n", j->id,
               j->stopped ? "Stopped" : "Running",
               j->cmdline ? j->cmdline : "");
    }
}

void free_jobs(void) {
    job_t *cur = jobs_head;
    while (cur) {
        job_t *next = cur->next;
        free(cur->cmdline);
        free(cur);
        cur = next;
    }
    jobs_head = NULL;
}
