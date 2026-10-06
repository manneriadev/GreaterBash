#include "execute.h"

extern hash_table* table;
extern char** environ;
extern int history_fd;
extern char* history[500];
extern int history_count;
extern void free_history();
extern bool global_flag_while;

extern joblist* global_list;
extern pid_t shell_pgid;

pid_t prev_terminal_owner = -1;

int last_exit_code;
pid_t last_bg_pid;

int builtins_execute(char* argv[]);
void make_sign();
void accept_redirs(ASTNode* root);
int execute_pipe(ASTNode* root , int in_fd , int out_fd , bool is_bg , pid_t* pgid_ptr);
int execute_cmd(ASTNode* root , int in_fd , int out_fd , bool is_bg , pid_t* pgid_ptr);
int execute_group(ASTNode* root , int in_fd , int out_fd , bool is_bg , pid_t* pgid_ptr);
int execute(ASTNode* , int, int , bool , pid_t*);
int chld_work(ASTNode* root , int in_fd , int out_fd , pid_t* pgid_ptr , char** argv);


Functions funcs[] = {
    {cmd_exit , "exit"},
    {cmd_quit , "quit"},
    {cmd_jobs , "jobs"},
    {cmd_bg , "bg"},
    {cmd_fg , "fg"},
    {cmd_set , "set"},
    {cmd_unset , "unset"},
    {cmd_help , "help"},
    {cmd_kill , "kill"},
    {cmd_cd , "cd"},
    {cmd_history , "history"},
    {cmd_67 , "67"},
    {cmd_export , "export"},
    {cmd_unexport , "unexport"}
};

int shell_execute(ASTNode* root , ASTNode* tree , TokenList* list)
{
    return execute(root , STDIN_FILENO , STDOUT_FILENO , false , NULL);
}

int execute(ASTNode* root , int in_fd , int out_fd , bool is_bg , pid_t* pgid_ptr)
{
    if(!root) return 0;

    switch(root->type)
    {
        case NODE_BINARY:

            switch(root->binary.btype)
            {
                case BNODE_OR_AND:
                case BNODE_OR:
                {
                    return execute_pipe(root , in_fd , out_fd , is_bg , pgid_ptr);
                }
                case BNODE_OR_OR:
                {
                    int left = execute(root->binary.left , in_fd , out_fd , is_bg , pgid_ptr);
                    if(left != 0) return execute(root->binary.right, in_fd , out_fd , is_bg , NULL);
                    return left;
                }
                case BNODE_AND_AND:
                {
                    int left = execute(root->binary.left , in_fd , out_fd , is_bg , pgid_ptr);
                    if(left == 0) return execute(root->binary.right , in_fd , out_fd , is_bg , NULL);
                    return left;
                }
                case BNODE_SEMI:
                case BNODE_NL:
                {
                    int left = execute(root->binary.left , in_fd , out_fd , is_bg , pgid_ptr);
                    return execute(root->binary.right , in_fd , out_fd , is_bg , NULL);
                }
                case BNODE_AND:
                {
                    int left = execute(root->binary.left , in_fd , out_fd , true , pgid_ptr);
                    return execute(root->binary.right , in_fd , out_fd , is_bg , pgid_ptr);
                }
                default: break;
            }
        case NODE_GROUP:
            return execute_group(root , in_fd , out_fd , is_bg , pgid_ptr);
        case NODE_CMD:
            return execute_cmd(root , in_fd , out_fd , is_bg , pgid_ptr);
        default: break;
        }
    return 0;
}

int builtins_execute(char** argv)
{
    if(!argv) return -1;

    if(argv[0])
    {
        for(size_t i = 0 ; i < 14 ; ++i)
        {
            if(strcmp(argv[0] , funcs[i].name) == 0)
            {
                return funcs[i].func(argv);
            }
        }
    }
    return -1;
}

void make_sign()
{
    signal(SIGINT , SIG_DFL);
    signal(SIGTSTP , SIG_DFL);
    signal(SIGQUIT , SIG_DFL);   
    signal(SIGTERM , SIG_DFL);
    signal(SIGTTIN , SIG_DFL);
    signal(SIGTTOU , SIG_DFL);
}

int execute_cmd(ASTNode* root , int in_fd , int out_fd , bool is_bg , pid_t* pgid_ptr)
{
    if(root->cmd.args && root->cmd.args[0] && root->cmd.args[0]->string.flag == true) return 0;

    size_t size = 0;
    if(root->cmd.args[size]) while(root->cmd.args[size]) ++size;
    
    char** argv = (char**)malloc(sizeof(char*)*(size+1));
    if(!argv)
    {
        fprintf(stderr, "error of malloc(execute)\n");
        return -1;
    }
    for(size_t i = 0 ; i < size ; ++i)
        argv[i] = root->cmd.args[i]->string.name;
    argv[size] = NULL;

    int tmp_ret = builtins_execute(argv);
    if(tmp_ret == 0)
    {
        free(argv);
        return tmp_ret;
    }

    bool bgflag = is_bg || root->cmd.bg;

    if(pgid_ptr != NULL) chld_work(root , in_fd  , out_fd , pgid_ptr , argv);

    pid_t pid = fork();
    if(pid < 0)
    {
        fprintf(stderr , "pid < 0(execute)\n");
        free(argv);
        return -1;
    }

    if(pid == 0)
    {
        chld_work(root , in_fd  , out_fd , pgid_ptr , argv);
    }

    if(pgid_ptr != NULL && *pgid_ptr > 0) setpgid(pid , *pgid_ptr);
    else setpgid(pid , pid);

    if(in_fd != STDIN_FILENO) close(in_fd);
    if(out_fd != STDOUT_FILENO) close(out_fd);

    if(bgflag)
    {
        job_t* new_job = init_job(global_list , 0 , pid , argv[0]);
        if(new_job)
        {
            new_job->state = JOB_BG_RUNNING;
            add_job_in_list(global_list , new_job);
            printf("[%d] %d &\n" , new_job->job_id , new_job->pgid);
        }
        last_exit_code = 0;
        last_bg_pid = pid;
        free(argv);
        return 0;
    }

    if(pgid_ptr != NULL)
    {
        free(argv);
        return 0;
    }
    
    prev_terminal_owner = (pgid_ptr != NULL) ? *pgid_ptr : shell_pgid;
    tcsetpgrp(STDIN_FILENO , pid);

    int status = 0;
    waitpid(pid , &status , WUNTRACED | WCONTINUED);

    if(WIFSTOPPED(status))
    {
        job_t* job = init_job(global_list , 0 , pid , argv[0]);
        if(job) 
        {
            add_job_in_list(global_list , job);
            job->state = JOB_STOPPED;
        }
        if(job) printf("\n[%d]+ Stopped\n" ,  job->job_id);
    }
    else if(WIFEXITED(status))
        (last_exit_code) = WEXITSTATUS(status);
    else if(WIFSIGNALED(status))
        (last_exit_code) = 128 + WTERMSIG(status);
    else (last_exit_code) = -1;

    tcsetpgrp(STDIN_FILENO , prev_terminal_owner);
    free(argv);
    return (last_exit_code);
}

int chld_work(ASTNode* root , int in_fd , int out_fd , pid_t* pgid_ptr , char** argv)
{
    make_sign();

        if(pgid_ptr != NULL && *pgid_ptr > 0) setpgid(0,*pgid_ptr);
        else setpgid(0,0);

        if(in_fd != STDIN_FILENO)
        {
            dup2(in_fd , STDIN_FILENO);
            close(in_fd);
        }
        if(out_fd != STDOUT_FILENO)
        {
            dup2(out_fd , STDOUT_FILENO);
            close(out_fd);
        }

        accept_redirs(root);
        execvp(argv[0] , argv);

        switch(errno)
        {
            case ENOENT:
                fprintf(stderr , "greaterbash: %s: command not found\n" , argv[0]);
                break;
            case EACCES:
                fprintf(stderr , "greaterbash: %s: permission denied\n" , argv[0]);
                break;
            case ENOEXEC:
                fprintf(stderr , "greaterbash: %s: cannot execute bianry file\n" , argv[0]);
                break;
            default: perror(argv[0]);
        }
        exit(-1);
}

int execute_pipe(ASTNode* root , int in_fd , int out_fd , bool is_bg , pid_t* pgid_ptr)
{
    int fd[2];
    if(pipe(fd) < 0)
    {
        fprintf(stderr , "pipe not init(execute)\n");
        return -1;
    }

    pid_t local_pgid = -1;
    pid_t* pgid = (pgid_ptr == NULL) ? &local_pgid : pgid_ptr;

    pid_t pid1 = fork();
    if(pid1 < 0)
    {
        fprintf(stderr , "pid < 0(execute)\n");
        close(fd[0]);
        close(fd[1]);
        return -1;
    }

    if(pid1 == 0)
    {
        if(*pgid > 0) setpgid(0 , *pgid);
        else setpgid(0,0);

        make_sign();

        close(fd[0]);
        dup2(fd[1] , STDOUT_FILENO);
        if(root->binary.btype == BNODE_OR_AND) dup2(fd[1] , STDERR_FILENO);
        close(fd[1]);

        if(in_fd != STDIN_FILENO)
        {
            dup2(in_fd , STDIN_FILENO);
            close(in_fd);
        }

        int ret = execute(root->binary.left , in_fd , STDOUT_FILENO , false , pgid);
        exit(ret);
    }

    if(*pgid <= 0)
    {
        *pgid = pid1;
        setpgid(pid1 , pid1);
    }
    else
    {
        setpgid(pid1 , *pgid);
    }

    pid_t pid2 = fork();
    if(pid2 < 0)
    {
        fprintf(stderr , "pid < 0(execute)\n");
        close(fd[0]);
        close(fd[1]);
        return -1;
    }

    if(pid2 == 0)
    {
        setpgid(0 , *pgid);

        make_sign();

        close(fd[1]);
        dup2(fd[0] , STDIN_FILENO);
        close(fd[0]);

        if(out_fd != STDOUT_FILENO)
        {
            dup2(out_fd , STDOUT_FILENO);
            close(out_fd);
        }

        int ret = execute(root->binary.right , STDIN_FILENO , out_fd , false , pgid);
        exit(ret);
    }

    setpgid(pid2 , *pgid);

    close(fd[0]);
    close(fd[1]);
    if(in_fd != STDIN_FILENO) close(in_fd);
    if(out_fd != STDOUT_FILENO) close(out_fd);

    if(pgid_ptr != NULL) return 0;

    char cmd[512] = "";
    if(root->binary.left->type == NODE_CMD &&
    root->binary.left->cmd.args &&
    root->binary.left->cmd.args[0]) strncat(cmd , root->binary.left->cmd.args[0]->string.name , 200);
    strncat(cmd , " | " , 4);
    if(root->binary.right->type == NODE_CMD &&
    root->binary.right->cmd.args &&
    root->binary.right->cmd.args[0]) strncat(cmd , root->binary.right->cmd.args[0]->string.name , 200);
    
    job_t* new_job = init_job(global_list , 0 , *pgid , cmd);
    if(is_bg)
    {
        if(new_job)
        {
            new_job->pids[new_job->pids_count++] = (subpid_t){pid1 , 1};
            new_job->pids[new_job->pids_count++] = (subpid_t){pid2 , 1};
            new_job->state = JOB_BG_RUNNING;
            add_job_in_list(global_list , new_job);
            printf("[%d]+ %d &\n" , new_job->job_id , new_job->pgid);
        }
        last_bg_pid = (pgid_ptr != NULL) ? *pgid_ptr : *pgid;
        return 0;
    }
    if(new_job)
    {
        new_job->pids[new_job->pids_count] = (subpid_t){pid1 , 1 };
        new_job->pids[new_job->pids_count + 1] = (subpid_t){pid2 , 1};
        new_job->pids_count += 2;
        new_job->state = JOB_RUNNING;
        add_job_in_list(global_list , new_job);
    }

    prev_terminal_owner = (pgid_ptr != NULL) ? *pgid_ptr : shell_pgid;
    tcsetpgrp(STDIN_FILENO , *pgid);    

    int status;
    pid_t last_pid;
    while((last_pid = waitpid(-(*pgid) , &status , WUNTRACED | WCONTINUED)) > 0)
    {
        if(WIFSTOPPED(status))
        {
            if(new_job)
            {
                subpid_t* subpid = get_subpid_in_conv(new_job , last_pid);
                if(subpid) subpid->state = SUBPID_STOPPED;

                update_job_state(new_job);

                printf("\n[%d]+ Stopped\t%s\n" , new_job->job_id , new_job->command);
            }
            break;
        }
        if(WIFEXITED(status))
        {
            if(new_job)
            {
                subpid_t* subpid = get_subpid_in_conv(new_job , last_pid);
                if(subpid) subpid->state = SUBPID_DONE; // done;
            }
            last_exit_code = WEXITSTATUS(status);
        }
        else if(WIFSIGNALED(status))
        {
            if(new_job)
            {
                subpid_t* subpid = get_subpid_in_conv(new_job , last_pid);
                if(subpid) subpid->state = SUBPID_TERMINATED; // done;
            }
            last_exit_code = 128 + WTERMSIG(status);
        }
    }

    if(new_job)
    {
        update_job_state(new_job);
        if((new_job->state & JOB_DONE || new_job->state & JOB_TERMINATED)) job_remove_job(global_list , new_job);
    }
    tcsetpgrp(STDIN_FILENO , prev_terminal_owner);
    return last_exit_code;
}

int execute_group(ASTNode* root , int in_fd , int out_fd , bool is_bg , pid_t* pgid_ptr)
{
    if(is_bg && root->unary.base->type == NODE_BINARY)
    {
        pid_t pid = fork();
        if(pid < 0)
        {
            fprintf(stderr , "pid < 0(execute)\n");
            return -1;
        }

        if(pid == 0)
        {
            if(pgid_ptr != NULL) setpgid(0,*pgid_ptr);
            else setpgid(0,0);
            
            int ret = execute(root->unary.base , in_fd , out_fd , false , NULL);
            exit(ret);
        }
        job_t* new_job = init_job(global_list , 0,pid, "(group)");
        if(new_job)
        {
            new_job->state = JOB_BG_RUNNING;
            add_job_in_list(global_list , new_job);
            printf("[%d]+ %d &\n", new_job->job_id , pid);
        }
        last_bg_pid = pid;
        return 0;
    }
    return execute(root->unary.base , in_fd , out_fd , is_bg , pgid_ptr);
}

void accept_redirs(ASTNode* root)
{
    if(root->cmd.redirs)
    {
        for(size_t i = 0 ; root->cmd.redirs[i] ; ++i)
        {
            ASTNode* redir = root->cmd.redirs[i];

            int fd = -1;
            int flags;

            switch(redir->type)
            {
                case NODE_GT:
                    flags = O_WRONLY | O_CREAT | O_TRUNC;
                    fd = open(redir->unary.base->string.name , flags , 0644);
                    if(fd < 0)
                    {
                        fprintf(stderr , "fd '%s' not open >\n" , redir->string.name);
                        exit(-1);
                    }
                    dup2(fd , STDOUT_FILENO);
                    close(fd);
                    break;
                case NODE_GT_GT:
                    flags = O_WRONLY | O_CREAT | O_APPEND;
                    fd = open(redir->unary.base->string.name , flags , 0644);
                    if(fd < 0)
                    {
                        fprintf(stderr , "fd '%s' not open >>\n" , redir->string.name);
                        exit(-1);
                    }
                    dup2(fd , STDOUT_FILENO);
                    close(fd);
                    break;
                case NODE_AND_GT:
                    flags = O_WRONLY | O_CREAT | O_TRUNC;
                    fd = open(redir->unary.base->string.name , flags , 0644);
                    if(fd < 0)
                    {
                        fprintf(stderr , "fd '%s' not open &>\n" , redir->string.name);
                        exit(-1);
                    }
                    dup2(fd , STDERR_FILENO);
                    dup2(fd , STDOUT_FILENO);
                    close(fd);
                    break;
                case NODE_AND_GT_GT:
                    flags = O_WRONLY | O_CREAT | O_APPEND;
                    fd = open(redir->unary.base->string.name , flags , 0644);
                    if(fd < 0)
                    {
                        fprintf(stderr , "fd '%s' not open &>>\n" , redir->string.name);
                        exit(-1);
                    }
                    dup2(fd , STDERR_FILENO);
                    dup2(fd , STDOUT_FILENO);
                    close(fd);
                    break;
                case NODE_LT:
                    fd = open(redir->unary.base->string.name , O_RDONLY);
                    if(fd < 0)
                    {
                        fprintf(stderr , "fd '%s' not open <\n" , redir->string.name);
                        exit(-1);
                    }
                    dup2(fd , STDIN_FILENO);
                    close(fd);
                    break;
            }
        }
    }
}