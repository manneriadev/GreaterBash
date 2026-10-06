#include "job_control.h"

job_t* init_job(joblist* list , int id , pid_t pgid , char* command)
{
    job_t* new_job = (job_t*)malloc(sizeof(job_t));
    if(!new_job)
    {
        perror("new_job");
        return NULL;
    }
    strncpy(new_job->command , command , 255);
    new_job->pids_count = 0;
    new_job->command[255] = '\0';
    new_job->job_id = id;
    new_job->pgid = pgid;
    new_job->state = JOB_RUNNING;
    new_job->next_job = NULL;
    new_job->prev_job = NULL;
    return new_job;
}

joblist* init_ht_l()
{
    joblist* list = (joblist*)malloc(sizeof(joblist));
    if(!list)
    {
        perror("job_list");
        return NULL;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    list->stack_id.top = 0;
    return list;
}

void add_job_in_list(joblist* list , job_t* job)
{

    int reused_id = pop_stack_id(list);

    if(list->size == 0)
    {
        list->head = job;
        list->tail = job;
        job->job_id = (reused_id > 0) ? reused_id : 1;
        list->size = 1;
    }
    else
    {
        job->job_id = (reused_id > 0) ? reused_id : (list->size + 1);
        list->tail->next_job = job;
        job->prev_job = list->tail;
        list->tail = job;
        ++list->size;
    }
}

job_t* get_job_by_id(joblist* list , int id)
{
    job_t* tmp = list->head;
    while(tmp)
    {
        if(tmp->job_id == id) return tmp;
        tmp = tmp->next_job;
    }
    return NULL;
}

job_t* get_job_by_pid(joblist* list , pid_t pid)
{
    job_t* tmp = list->head;
    while(tmp)
    {
        if(tmp->pgid == pid) return tmp;
        tmp = tmp->next_job;
    }
    return NULL;
}

void job_remove_pid(joblist* list , pid_t pid)
{
    if(!list) return;
    job_t* tmp = get_job_by_pid(list , pid);
    if(!tmp) return;
    
    push_stack_id(list , tmp->job_id);

    if(tmp->pgid == 0)
    {
        list->head = tmp->next_job;
        if(list->head) list->head->prev_job = NULL;
        --list->size;
        free(tmp);
        return;
    }
    else if(tmp == list->tail)
    {
        list->tail = tmp->prev_job;
        if(list->head)list->tail->next_job = NULL;
        --list->size;
        free(tmp);
        return;
    }
    else
    {
        tmp->prev_job->next_job = tmp->next_job;
        if(tmp->next_job) tmp->next_job->prev_job = tmp->prev_job;
        --list->size;
        free(tmp);
        return;
    }
}

void job_remove_id(joblist* list , int id)
{
    if(!list) return;
    job_t* tmp = get_job_by_id(list , id);
    if(!tmp) return;

    push_stack_id(list , tmp->job_id);

    if(tmp->job_id == 0)
    {
        list->head = tmp->next_job;
        if(list->head)list->head->prev_job = NULL;
        --list->size;
        free(tmp);
        return;
    }
    else if(tmp->job_id == list->size)
    {
        list->tail = tmp->prev_job;
        if(list->head)list->tail->next_job = NULL;
        --list->size;
        free(tmp);
        return;
    }
    else
    {
        tmp->prev_job->next_job = tmp->next_job;
        if(tmp->next_job) tmp->next_job->prev_job = tmp->prev_job;
        free(tmp);
        --list->size;
        return;
    }
}

void job_remove_job(joblist* list , job_t* job)
{
    if(!list || !job) return;

    push_stack_id(list , job->job_id);

    if(list->head == job)
    {
        job_t* old_head = list->head;
        list->head = list->head->next_job;
        if(list->head) list->head->prev_job = NULL;
        --list->size;
        free(old_head);
        return;
    }
    else if(list->tail == job)
    {
        job_t* old_tail = list->tail;
        list->tail = job->prev_job;
        if(list->tail) list->tail->next_job = NULL;
        --list->size;
        free(old_tail);
        return;
    }

    job_t* tmp = list->head;
    while(tmp)
    {
        if(job == tmp)
        {
            tmp->prev_job->next_job = tmp->next_job;
            if(tmp->next_job) tmp->next_job->prev_job = tmp->prev_job;
            --list->size;
            free(tmp);
            return;
        }
        tmp = tmp->next_job;
    }

}

void clean_up_done(joblist* list)
{
    job_t* tmp = list->head;
    while(tmp)
    {
        job_t* next = tmp->next_job;
        if(tmp->state & JOB_DONE || tmp->state & JOB_TERMINATED) job_remove_job(list , tmp);
        tmp = next;
    }
}

void jobs_list(joblist* list)
{
    if(!list) return;

    job_t* tmp = list->head;
    if(!tmp) return;

    while(tmp)
    {
        char* state;
        if(tmp->state & JOB_STOPPED) state = "Stopped";
        else if(tmp->state & JOB_BG_RUNNING ) state = "Background running";
        else if(tmp->state & JOB_RUNNING) state = "Running";
        else if(tmp->state & JOB_DONE) state = "Done";
        else state = "Terminated";
        printf("[%d](pgid: %d):%s -> %s\n" ,tmp->job_id,tmp->pgid,tmp->command,state);

        if(tmp->pids_count > 1)
        {
            for(size_t i = 0 ; i < tmp->pids_count ; ++i)
            {
                char* st;
                if(tmp->pids[i].state & SUBPID_STOPPED) st = "Stopped";
                else if(tmp->pids[i].state & SUBPID_BG_RUNNING) st = "Background running";
                else if(tmp->pids[i].state & SUBPID_RUNNING ) st = "Running";
                else if(tmp->pids[i].state & SUBPID_DONE) st = "Done";
                else st = "Terminated";
                printf("\t[%d] pid: %d state:%s\n" , i , tmp->pids[i].pid , st);
            }
        }

        tmp = tmp->next_job;
    }
}

subpid_t* get_subpid_in_conv(job_t* job, pid_t pid)
{
    if(!job || pid<=0) return NULL;
    for(size_t i = 0 ; i < job->pids_count ; ++i)
    {
        if(job->pids[i].pid == pid) return &(job->pids[i]);
    }
    return NULL;
}

void update_job_state(job_t* job)
{
    if(!job || job->pids_count == 0) return;

    JobState state = 0; // <- here i use enum but nums equels states

    for(size_t i = 0 ; i < job->pids_count ; ++i)
    {
        SubpidState s = job->pids[i].state;

        state |= ((s & SUBPID_RUNNING)||(s & SUBPID_BG_RUNNING)) ? JOB_RUNNING : (s & SUBPID_STOPPED) ? JOB_STOPPED : JOB_DONE;
    }

    if(state & JOB_RUNNING) job->state = (job->state & JOB_BG_RUNNING) ? JOB_BG_RUNNING : JOB_RUNNING;
    else if(state & JOB_STOPPED) job->state = JOB_STOPPED;
    else job->state = JOB_DONE;
}

void push_stack_id(joblist* list , int id)
{
    if(list->stack_id.top < 1023)
    {
        list->stack_id.ids[++list->stack_id.top] = id;
    }
}

int pop_stack_id(joblist* list)
{
    if(list->stack_id.top >= 0)
    {
        return list->stack_id.ids[list->stack_id.top--];
    }

    return 0;
}