#pragma once

#include "errno.h"
#include "stdbool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

extern void sigchld_handler(int sig);

typedef enum JobState {
    JOB_RUNNING        = 1,
    JOB_BG_RUNNING     = 2,  
    JOB_STOPPED        = 4,
    JOB_DONE           = 8,
    JOB_TERMINATED     = 16,
} JobState;

typedef enum SubpidState {
    SUBPID_RUNNING     = 1,
    SUBPID_BG_RUNNING  = 2,
    SUBPID_STOPPED     = 4,
    SUBPID_DONE        = 8,
    SUBPID_TERMINATED  = 16,
} SubpidState;


typedef struct subpid_t{
    pid_t pid;
    SubpidState state;
} subpid_t;

typedef struct job_t{
    int job_id;
    pid_t pgid;
    subpid_t pids[10192];
    int pids_count;
    JobState state; //1-running 2-baskground_run 4-stoped 8- done 16 - terminated
    char command[256];
    struct job_t* next_job;
    struct job_t* prev_job;
} job_t;

typedef struct stack_tt{
    int ids[1024];
    int top;
} stack_tt;

typedef struct ht_l{ // head and tail of the list
    job_t* head;
    job_t* tail;
    size_t size;
    stack_tt stack_id;
}joblist;

job_t* init_job(joblist*, int, pid_t, char* );
joblist* init_ht_l();
void add_job_in_list(joblist*, job_t*);

job_t* get_job_by_id(joblist* , int );
job_t* get_job_by_pid(joblist* , pid_t );
void job_remove_pid(joblist* , pid_t);
void job_remove_id(joblist* , int);
void job_remove_job(joblist* list , job_t* job);

subpid_t* get_subpid_in_conv(job_t* job, pid_t pid);
void update_job_state(job_t* job);

int pop_stack_id(joblist* list);
void push_stack_id(joblist* list , int id);

void clean_up_done(joblist* );
void jobs_list(joblist*);