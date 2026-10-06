#include "builtins.h"

void mlsleep(int n)
{
    struct timespec ts;
    ts.tv_sec = n/1000;
    ts.tv_nsec = (n%1000)*1000000;
    nanosleep(&ts , NULL);
}

int cmd_exit(char** argv)
{
    global_flag_while = false;
    return 0;
}

int cmd_quit(char** argv)
{
    global_flag_while = false;
    return 0;
}

int cmd_jobs(char **argv)
{
    jobs_list(global_list);
    clean_up_done(global_list);
    return 0;
}

int cmd_export(char **argv)
{
    if(!argv || !argv[1]) return -1;

    if(argv[1] != NULL && argv[2] != NULL)
    {
        setenv(argv[1] , argv[2] , 1);
        table_insert(table , argv[1] , argv[2]);
        return 0;
    }
    else if(argv[1] != NULL)
    {
        char* value = table_get(table , argv[1]);
        setenv(argv[1] , value , 1);
        return 0;
    }

    return 0;
}

int cmd_unexport(char **argv)
{
    if(!argv || !argv[1]) return -1;

    for(size_t i = 0; argv[i] ; ++i)
    {
        unsetenv(argv[i]);
    }
    
    return 0;
}

int cmd_fg(char** argv)
{
    job_t* tmp = NULL;

    if(!argv[1])
    {
        tmp = get_job_by_id(global_list , global_list->size);
    }
    else
    {
        int job_id = atoi(argv[1]);
        tmp = get_job_by_id (global_list , job_id);
    }

    if(!tmp)
    {
        fprintf(stdout , "No such job\n");
        return 1;
    }

    printf("%s\n" , tmp->command);

    tcsetpgrp(STDIN_FILENO , tmp->pgid);

    if(tmp->state & JOB_STOPPED)
    {
        kill(-tmp->pgid , SIGCONT);
        if(tmp->pids_count > 0)
        {
            for(size_t i = 0 ; i < tmp->pids_count ; ++i)
            {
                if(tmp->pids[i].state & SUBPID_STOPPED)tmp->pids[i].state = SUBPID_RUNNING;
            }
        }
    }

    tmp->state = JOB_RUNNING;

    int status;
    pid_t last_pid;
    
    if(tmp->pids_count > 1)
    {
        while((last_pid = waitpid(-tmp->pgid , &status , WUNTRACED | WCONTINUED)) > 0)
        {
            if(WIFSTOPPED(status))
            {
                subpid_t* subpid = get_subpid_in_conv(tmp , last_pid);
                if(subpid) subpid->state = SUBPID_STOPPED;
                update_job_state(tmp);

                if(tmp->state & JOB_STOPPED) break;
            }
            else if(WIFCONTINUED(status))
            {
                subpid_t* subpid = get_subpid_in_conv(tmp , last_pid);
                if(subpid) subpid->state = SUBPID_RUNNING;
            }
            else if(WIFEXITED(status) || WIFSIGNALED(status))
            {
                subpid_t* subpid = get_subpid_in_conv(tmp , last_pid);
                if(subpid) subpid->state = (WIFEXITED(status)) ? SUBPID_DONE : SUBPID_TERMINATED;
            }
        }
        update_job_state(tmp);
    }
    else
    {
        waitpid(-tmp->pgid , &status , WUNTRACED | WCONTINUED);
        if(WIFSTOPPED(status)) tmp->state = JOB_STOPPED;
        else if(WIFEXITED(status)) tmp->state = JOB_DONE;
        else if(WIFSIGNALED(status)) tmp->state = JOB_TERMINATED;
    }

    tcsetpgrp(STDIN_FILENO , shell_pgid);
    return 0;
}

int cmd_bg(char** argv)
{
    job_t* tmp = NULL;

    if(!argv[1])
    {
        tmp = get_job_by_id(global_list , global_list->size);
    }
    else
    {
        int job_id = atoi(argv[1]);
        tmp = get_job_by_id (global_list , job_id);
    }

    if(!tmp)
    {
        fprintf(stdout , "No such job\n");
        return 1;
    }

    printf("%s\n" , tmp->command);

    kill(-tmp->pgid , SIGCONT);
    tmp->state = JOB_RUNNING;

    if(tmp->pids_count > 0)
    {
        for(size_t i = 0 ; i < tmp->pids_count ; ++i)
        {
            if(tmp->pids[i].state & SUBPID_STOPPED)
            {
                tmp->pids[i].state = SUBPID_BG_RUNNING;
            }
        }
    }

    return 0;
}

int cmd_set(char** argv)
{
    if(argv[1]) table_insert(table , argv[1] , argv[2] ? argv[2] : "");
    else
    {
        for(char** i = environ ; *i ; ++i)
        {
            fprintf(stdout , "%s\n" , *i);
        }
        print_table(table);
    }
    return 0;
}

int cmd_unset(char** argv)
{
    size_t i;
    for(i = 1 ; argv[i] ; ++i) table_del(table , argv[i]);
    return 0;
}

int cmd_help(char* argv[])
{
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                    GreaterBash - Help                          ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("BUILT-IN COMMANDS:\n");
    printf("  cd [DIR]                      Change directory\n");
    printf("  pwd                           Print working directory\n");
    printf("  echo [TEXT...]                Print text\n");
    printf("  exit                          Exit shell\n");
    printf("  help                          Show this help\n");
    printf("  jobs                          List background jobs\n");
    printf("  fg [JOB_ID]                   Bring job to foreground\n");
    printf("  bg [JOB_ID]                   Resume stopped job in background\n");
    printf("  kill [PID] [SIG]              Terminate process\n");
    printf("  export [VAR]/[VAR][VALUE]     Export variable in environ\n");
    printf("  unexport [VAR]/[VAR][VALUE]   Remove variables from environ\n");
    printf("  set [VAR] [VALUE]             Add variable to variables list\n");
    printf("  unset [VAR...]                Remove variabls from variabels list\n");

    printf("  67                            Make 67\n\n");

    printf("OPERATORS:\n");
    printf("  |                             Pipe output\n");
    printf("  |&                            Pipe stdout and stderr\n");
    printf("  >                             Redirect stdout (overwrite)\n");
    printf("  >>                            Redirect stdout (append)\n");
    printf("  <                             Redirect stdin\n");
    printf("  &>                            Redirect stdout+stderr (overwrite)\n");
    printf("  &>>                           Redirect stdout+stderr (append)\n");
    printf("  ;                             Sequential execution\n");
    printf("  &&                            Execute if previous succeeded\n");
    printf("  ||                            Execute if previous failed\n");
    printf("  &                             Execute in background\n\n");

    printf("SPECIAL VARIABLES:\n");
    printf("  $?                            Exit code of last command\n");
    printf("  $$                            Shell PID\n");
    printf("  $!                            Last background PID\n");
    printf("  $VAR                          Environment variable\n\n");

    printf("FEATURES:\n");
    printf("  Single & double quotes with escaping\n");
    printf("  Variable substitution ($VAR) and creating new variables like \"a=b\"\n");
    printf("  Process grouping ()\n");
    printf("  Job control (foreground/background)\n");
    printf("  Comment support (#)\n\n");

    printf("EXAMPLES:\n");
    printf("  ls -l | grep file\n");
    printf("  cat file > output.txt\n");
    printf("  cmd1 && cmd2\n");
    printf("  long_task &\n");
    printf("  echo $HOME\n\n");

    return 0;
}

int cmd_cd(char** argv)
{
    const char* target = argv[1] ? argv[1] : getenv("HOME");
    if(chdir(target) != 0)
    {
        perror(argv[0]);
        return -1;
    }

    return 0;
}

int cmd_kill(char** argv)
{
    
    if(!argv[1])
    {
        perror("no argument");
        return -1;
    }

    pid_t target_pid = atoi(argv[1]);
    int sig = SIGTERM;
    if(argv[2])
    {
        if(!strcmp(argv[2] , "-CONT")) sig = SIGCONT;
        else if(!strcmp(argv[2] , "-INT")) sig = SIGINT;
        else if(!strcmp(argv[2] , "-STP")) sig = SIGTSTP;
        else if(!strcmp(argv[2] , "-TERM")) sig = SIGTERM;
        else if(!strcmp(argv[2] , "-KILL")) sig = SIGKILL; 
    }

    kill(target_pid, sig);
    return 0;
}

int cmd_history(char** argv)
{
    for(size_t i = 0 ; i < history_count ; ++i)
    {
        printf("%s\n" , history[i]);
    }
    return 0;
}

int cmd_67(char** argv)
{
    int t = 10;
    while(t > 0)
    {
        if(t == 0);
            printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            printf("                                _______                        \n");
            printf("                               /                                       \n");
            printf("           _______            /    7   | \\                            \n");
            printf("                  \\          |         |  \\                      \n");
            printf("         / |   6   \\         // // // //  \\\\                       \n");
            printf("        /  |        |        || || || ||    .                        \n");
            printf("       //  \\\\ \\\\ \\\\ \\\\       \\\\ \\\\ \\\\ \\\\         \n");
            printf("      .    || || || ||        .  .  .  .              \n");
            printf("           // // // //                      \n");
            printf("           .  .  .  .                      \n");
            mlsleep(75);
            printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            printf("                                                              \n");
            printf("           _______             _______                                \n");
            printf("                  \\           /                              \n");
            printf("         / |   6   \\         /    7   | \\                     \n");
            printf("        /  |        |       |         |  \\                     \n");
            printf("       //  \\\\ \\\\ \\\\ \\\\      // // // //  \\\\                                \n");
            printf("      .    || || || ||      || || || ||    .                   \n");
            printf("           // // // //      \\\\ \\\\ \\\\ \\\\                \n");
            printf("           .  .  .  .        .  .  .  .                        \n");
            printf("                                                            \n");        
            mlsleep(75);
            printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            printf("           _______                                             \n");
            printf("                  \\                                    \n");
            printf("         / |   6   \\          _______                   \n");
            printf("        /  |        |        /                 \n");
            printf("       //  \\\\ \\\\ \\\\ \\\\      /    7   | \\                                      \n");
            printf("      .    || || || ||     |         |  \\                   \n");
            printf("           // // // //    // // // //  \\\\        \n");
            printf("           .  .  .  .     || || || ||    .                         \n");
            printf("                          \\\\ \\\\ \\\\ \\\\                                \n");
            printf("                           .  .  .  .                               \n");        
            mlsleep(75);
            printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            printf("                                                              \n");
            printf("           _______             _______                                \n");
            printf("                  \\           /                              \n");
            printf("         / |   6   \\         /    7   | \\                     \n");
            printf("        /  |        |       |         |  \\                     \n");
            printf("       //  \\\\ \\\\ \\\\ \\\\      // // // //  \\\\                                \n");
            printf("      .    || || || ||      || || || ||    .                   \n");
            printf("           // // // //      \\\\ \\\\ \\\\ \\\\                \n");
            printf("           .  .  .  .        .  .  .  .                        \n");
            printf("                                                            \n");        
            mlsleep(75);
            --t;
        }
    fflush(stdout);
    return 0;
}