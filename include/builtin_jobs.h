#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>

int parse_job_spec(const char *s);
int cmd_fg(char **tokens);
int cmd_bg(char **tokens);