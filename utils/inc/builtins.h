#pragma once

#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

#include "hash_table.h"
#include "job_control.h"

extern hash_table* table;
extern char** environ;
extern bool global_flag_while;
extern char* history[500];
extern int history_count;
extern joblist* global_list;
extern pid_t shell_pgid;

int cmd_exit(char* argv[]);
int cmd_quit(char* argv[]);

int cmd_jobs(char* argv[]);
int cmd_bg(char* argv[]);
int cmd_fg(char* argv[]);

int cmd_unexport(char **argv);
int cmd_export(char **argv);
int cmd_set(char* argv[]);
int cmd_unset(char* argv[]);

int cmd_help(char* argv[]);
int cmd_kill(char* argv[]);
int cmd_cd(char* argv[]);

int cmd_history(char* argv[]);
int cmd_67(char* argv[]);