// jobs.h
#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

typedef struct job {
    int id;             // small job id (1,2,3...)
    pid_t pgid;         // process group id
    char *cmdline;      // strdup'd command line
    int stopped;        // 1 if stopped, 0 if running
    struct job *next;
} job_t;

/* jobs management */
job_t* add_job(pid_t pgid, const char *cmdline, int stopped);
job_t* find_job_by_id(int id);
job_t* find_job_by_pgid(pid_t pgid);
void remove_job(job_t *job);
void mark_job_stopped(job_t *job);
void mark_job_running(job_t *job);
void print_jobs(void);
void free_jobs(void); /* free all jobs at shell exit */

#endif // JOBS_H
