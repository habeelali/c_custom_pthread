
#ifndef THREAD_H
#define THREAD_H

#include <signal.h>
#include <setjmp.h>
#include <stdio.h>

#define STACK_SIZE 4096
#define MAX_THREADS 64

enum THREAD_STATES
{
    RUNNING,
    READY,
    BLOCKED,
    COMPLETED,
    EXITED
};

typedef struct thread thread_t;

struct thread
{
    int thread_id;
    sigjmp_buf context;
    void (*start_routine)(void *);
    void *arg;
    enum THREAD_STATES state;
    char *stack;
    thread_t *next;
    thread_t *waiting_next;
};

extern thread_t *current_thread;
extern sigjmp_buf main_context;

void init_lib(void);
void enqueue_thread(thread_t *thread);
thread_t *dequeue_thread();
int thread_create(thread_t **thread, void (*start_routine)(void *), void *arg);
void thread_exit(void);
void thread_join(thread_t *thread);
void thread_yield(void);
void thread_sleep(unsigned int milliseconds);
void thread_start(void);
void scheduler(void);

#endif
