#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <time.h>

#define QUEUE_SIZE 10
#define MAX_ITEMS 15

typedef struct {
    int data[QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int producer_alive;
    int consumer_alive;
    int producer_fd;
    int consumer_fd;
} SharedQueue;

void queue_init(SharedQueue *q) {
    q->head = q->tail = q->count = 0;
    q->producer_alive = q->consumer_alive = 1;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&q->mutex, &attr);

    pthread_condattr_t cattr;
    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&q->cond, &cattr);

    q->producer_fd = eventfd(0, EFD_NONBLOCK);
    q->consumer_fd = eventfd(0, EFD_NONBLOCK);
}

int queue_push(SharedQueue *q, int value) {
    pthread_mutex_lock(&q->mutex);

    while (q->count == QUEUE_SIZE && q->consumer_alive) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    if (!q->consumer_alive) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }

    q->data[q->tail] = value;
    q->tail = (q->tail + 1) % QUEUE_SIZE;
    q->count++;

    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);

    return 0;
}

int queue_pop(SharedQueue *q, int *value) {
    pthread_mutex_lock(&q->mutex);

    while (q->count == 0 && q->producer_alive) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    if (q->count == 0) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }

    *value = q->data[q->head];
    q->head = (q->head + 1) % QUEUE_SIZE;
    q->count--;

    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

void queue_destroy(SharedQueue *q) {
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
    close(q->producer_fd);
    close(q->consumer_fd);
}

void producer_process(SharedQueue *q) {
    srand(getpid());
    printf("[Producer %d] Started\n", getpid());

    for (int i = 0; i < MAX_ITEMS; i++) {
        pthread_mutex_lock(&q->mutex);
        int alive = q->consumer_alive;
        pthread_mutex_unlock(&q->mutex);

        if (!alive) break;

        int value = rand() % 100;
        if (queue_push(q, value) == 0) {
            printf("[Producer] Pushed: %d\n", value);
        } else {
            break;
        }

        uint64_t u = 1;
        write(q->producer_fd, &u, sizeof(u));
        usleep(200000);
    }

    pthread_mutex_lock(&q->mutex);
    q->producer_alive = 0;
    pthread_cond_broadcast(&q->cond);
    pthread_mutex_unlock(&q->mutex);

    printf("[Producer] Finished\n");
}

void consumer_process(SharedQueue *q) {
    printf("[Consumer %d] Started\n", getpid());

    while (1) {
        int value;
        if (queue_pop(q, &value) == 0) {
            printf("[Consumer] Popped: %d\n", value);
        } else {
            break;
        }

        uint64_t u = 1;
        write(q->consumer_fd, &u, sizeof(u));
        usleep(300000);
    }

    pthread_mutex_lock(&q->mutex);
    q->consumer_alive = 0;
    pthread_cond_broadcast(&q->cond);
    pthread_mutex_unlock(&q->mutex);

    printf("[Consumer] Finished\n");
}

int main() {
    printf("=== Host Started ===\n");

    SharedQueue *q = mmap(NULL, sizeof(SharedQueue),
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    queue_init(q);

    int epfd = epoll_create1(0);
    struct epoll_event ev;
    ev.events = EPOLLIN;

    ev.data.fd = q->producer_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, q->producer_fd, &ev);

    ev.data.fd = q->consumer_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, q->consumer_fd, &ev);

    pid_t prod_pid = fork();
    if (prod_pid == 0) {
        producer_process(q);
        exit(0);
    }

    pid_t cons_pid = fork();
    if (cons_pid == 0) {
        consumer_process(q);
        exit(0);
    }

    printf("[Host] Monitoring PID %d and %d\n", prod_pid, cons_pid);

    int prod_done = 0, cons_done = 0;

    while (!prod_done || !cons_done) {
        struct epoll_event events[2];
        int n = epoll_wait(epfd, events, 2, 1000);

        for (int i = 0; i < n; i++) {
            uint64_t u;
            read(events[i].data.fd, &u, sizeof(u));

            if (events[i].data.fd == q->producer_fd) {
                printf("[Host] Producer alive\n");
            } else {
                printf("[Host] Consumer alive\n");
            }
        }

        if (!prod_done && waitpid(prod_pid, NULL, WNOHANG) == prod_pid) {
            printf("[Host] Producer exited\n");
            prod_done = 1;
        }

        if (!cons_done && waitpid(cons_pid, NULL, WNOHANG) == cons_pid) {
            printf("[Host] Consumer exited\n");
            cons_done = 1;
        }
    }

    close(epfd);
    queue_destroy(q);
    munmap(q, sizeof(SharedQueue));

    printf("=== Host Completed ===\n");
 