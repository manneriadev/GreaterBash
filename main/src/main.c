#include "lexer.h"
#include "parser.h"
#include "substitution.h"
#include "execute.h"
#include "job_control.h"
#include "my_read.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <ctype.h>
#include <termios.h>

#define STR_SIZE 1024

bool global_flag_while = true;

void get_history();
void free_history();
void disable_raw_mode();
void enable_raw_mode();
void print_promt();

extern hash_table* table;
int history_fd;
char* global_fname;

char input[STR_SIZE];

char* history[500];
int history_count = 0;
int current_pos = -1;

struct termios orig_termios;

joblist* global_list = NULL;
pid_t shell_pgid = 0;

bool sigchld_flag;

void sigchld_handler(int sig)
{
    sigchld_flag = true;
}

void sigchld_function()
{
    pid_t pid;
    int status;
    while((pid = waitpid(-1 , &status , WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    {
        job_t* tmp = global_list->head;
        bool found = false;

        while(tmp && !found)
        {
            if(tmp->pgid == pid && tmp->pids_count == 0)
            {
                found = true;

                if(WIFCONTINUED(status))
                {
                    tmp->state = JOB_BG_RUNNING;
                }
                else if(WIFSTOPPED(status))
                {
                    tmp->state = JOB_STOPPED;
                }
                else if(WIFEXITED(status))
                {
                    tmp->state = JOB_DONE;
                }
                else if(WIFSIGNALED(status))
                {
                    tmp->state = JOB_TERMINATED;
                }
                break;
            }
        
            if(tmp->pids_count > 0)
            {
                subpid_t* subpid = get_subpid_in_conv(tmp , pid);
                if(subpid)
                {
                    found = true;

                    if(WIFCONTINUED(status))
                    {
                        subpid->state = SUBPID_BG_RUNNING;
                    }
                    else if(WIFSTOPPED(status))
                    {
                        subpid->state = SUBPID_STOPPED;
                    }
                    else if(WIFSIGNALED(status) || WIFEXITED(status))
                    {
                        subpid->state = WIFEXITED(status) ? SUBPID_DONE : SUBPID_TERMINATED;
                    }
                    update_job_state(tmp);
                    break;
                }
            }
            tmp = tmp->next_job;
        }
    }
}

void set_sigchld_handler()
{
    struct sigaction sa;
    memset(&sa, 0 ,sizeof(sa));    
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGCHLD , &sa , NULL);
}

void process_table(ASTNode* root)
{
    if(!root) return;

    switch (root->type)
    {
        case NODE_BINARY:
            process_table(root->binary.left);
            process_table(root->binary.right);
            break;
        case NODE_CMD:
            if(root->cmd.args)
            {
                for(size_t i = 0 ; root->cmd.args[i] ; ++i)
                {
                    ASTNode* arg = root->cmd.args[i];
                    if(arg->string.flag)
                    {
                        char* assignment = arg->string.name;
                        char* eq = strchr(assignment , '=');
                        if(eq)
                        {
                            size_t len = eq - assignment;
                            char* var = strndup(assignment , len);
                            char* value = eq + 1;

                            char* expanded_value = sub(value , table);

                            if(expanded_value && table)
                            {
                                table_insert(table , var , expanded_value);
                            }
                            else
                            {
                                table_insert(table , var , value);
                            }
                            if(expanded_value) free(expanded_value);
                            free(var);
                        }
                    }
                }
            }
            break;
    }
}

void print_promt()
{
    struct passwd *pw;
    char path[512];
    char hostname[128];

    if(gethostname(hostname , 127) < 0)
    {
        strcpy(hostname , "localhost");
    }
    hostname[127] = '\0';

    pw = getpwuid(getuid());
    const char* username = pw ? pw->pw_name : "user";

    if(!getcwd(path , 511))
    {
        strcpy(path , "?");
    }
    path[511]= '\0';

    char* home = getenv("HOME");
    if(home)
    {
        size_t t = strlen(home);
        if(strcmp(path , home) == 0)
        {
            strcpy(path , "~");
        }
        else if(strncmp(path , home , t) == 0 && path[t] == '/')
        {
            char tmp[512];
            sprintf(tmp , "~%s" , path + t);
            strcpy(path , tmp);
        }
    }

    printf("%s@%s:%s$ " , username , hostname , path);
}

int main(){

    chdir(getenv("HOME"));

    global_list = init_ht_l();
    shell_pgid = getpgrp();

    signal(SIGINT , SIG_IGN);
    signal(SIGTSTP , SIG_IGN);
    signal(SIGQUIT , SIG_IGN);
    signal(SIGTERM , SIG_IGN);
    signal(SIGTTIN , SIG_IGN);
    signal(SIGTTOU , SIG_IGN);

    set_sigchld_handler();

    struct passwd *pw = getpwuid(getuid());
    char* fname = pw->pw_dir;
    strcat(fname , "/bash_history.txt");
    global_fname = fname;
    history_fd = open(fname , O_RDWR | O_CREAT | O_APPEND , 0600);

    get_history();

    while(global_flag_while)
    {
        if(sigchld_flag) sigchld_function();

        if(!table) table = create_table();

        struct timespec ts = {0 , 1000000};
        nanosleep(&ts , NULL);

        print_promt();
        fflush(stdout);

        read_command(input , 1024);
        disable_raw_mode();

        if(input[0] == '\0')
        {
            continue;
        }

        size_t len = strlen(input);
        if(len && input[len - 1] == '\n') input[len - 1] = '\0';

        TokenList* list = Tokenize(input);
        if(!list)
        {
            fprintf(stderr , "lexical error\n");
            exit(1);
        }
        
        ASTNode* tree = parse(list);
        
        if(input != '\0') add_to_history(input);
        if(!tree)
        {
            free_tokens(list);
            continue;
        }

        process_table(tree);
        expand(tree);

        shell_execute(tree , tree , list);

        free_tokens(list);
        ast_clean(tree);
    }

    table_free(table);
    close(history_fd);
    free_history();
    if(global_list)
    {
        job_t* tmp = global_list->head;
        for(size_t i = 0 ; i < global_list->size ; ++i)
        {
            job_t* htmp = tmp;
            tmp = tmp->next_job;
            free(htmp);
        }
    }
    free(global_list);

}

//for cheking:
// x=.t'x't ; ls -la >f'i'l"e"$x
//echo "hello world" &> f1.txt ; ls -f > file.txt | ls >> f2.txt ; echo $PATH ; VAR=value && /bin/ls
// echo "hello world" &> f1.txt ; ls -f > file.txt | ls >> f2.txt ; echo $PATH ; VAR=value ; ( echo "hello world" &> f1.txt ; ls -f > file.txt | ls >> f2.txt ; echo $PATH ; export var=value )