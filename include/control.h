#include "thread.h"
#ifndef CONTROL_H
#define CONTROL_H

extern thread_t *current_thread;

typedef struct semaphore_t
{
    int value;
    thread_t *waiting_queue;
} semaphore;

typedef struct mutex_t
{
    int locked;
    thread_t *waiting_queue;
} mutex;

void sem_init(semaphore *sem, int value);
void sem_wait(semaphore *sem);
void sem_post(semaphore *sem);

void mutex_init(mutex *m);
void mutex_acquire(mutex *m);
void mutex_release(mutex *m);

#endif
