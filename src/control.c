#include "control.h"
#include "thread.h"
#include "util.h"

#include <stdio.h>

void sem_init(semaphore *sem, int value)
{
    sem->value = value;
    sem->waiting_queue = NULL;
}

void sem_enqueue_thread(thread_t **queue, thread_t *thread)
{
    if (!thread)
    {
        fprintf(stderr, "Error: Attempting to enqueue a NULL thread\n");
        exit(1);
    }
    printf("Enqueuing thread ID: %d\n", thread->thread_id);

    thread_t *original_next = thread->next;

    if (!*queue)
    {
        *queue = thread;
        thread->next = NULL;
    }
    else
    {
        thread_t *temp = *queue;
        while (temp->next)
        {
            temp = temp->next;
        }
        temp->next = thread;
        thread->next = NULL;
    }
}

thread_t *sem_dequeue_thread(thread_t **queue)
{
    if (!*queue)
    {
        fprintf(stderr, "Error: Semaphore queue is empty\n");
        return NULL;
    }
    thread_t *thread = *queue;
    *queue = (*queue)->next;
    printf("Dequeuing thread %d from semaphore queue\n", thread->thread_id);
    return thread;
}

void thread_block()
{
    current_thread->state = BLOCKED;
    thread_yield();
}

void thread_unblock(thread_t *thread)
{
    if (thread->state == BLOCKED)
    {
        thread->state = READY;
        enqueue_thread(thread);
    }
}

void print_semaphore_queue(thread_t *queue)
{
    printf("Semaphore queue: ");
    while (queue)
    {
        printf("%d -> ", queue->thread_id);
        queue = queue->next;
    }
    printf("NULL\n");
}

void sem_wait(semaphore *sem)
{
    disable_interrupts();

    sem->value--;
    if (sem->value < 0)
    {
        thread_t *current = current_thread;
        current->waiting_next = sem->waiting_queue;
        sem->waiting_queue = current;
        thread_block();
    }

    enable_interrupts();
}

void sem_post(semaphore *sem)
{
    disable_interrupts();

    sem->value++;
    if (sem->value <= 0)
    {
        thread_t *unblocked_thread = sem->waiting_queue;
        if (unblocked_thread)
        {
            sem->waiting_queue = unblocked_thread->waiting_next;
            thread_unblock(unblocked_thread);
        }
    }

    enable_interrupts();
}

void mutex_init(mutex *m)
{
    if (!m)
    {
        fprintf(stderr, "Error: Attempting to initialize a NULL mutex\n");
        exit(1);
    }
    m->locked = 0;
    m->waiting_queue = NULL;
}

void mutex_acquire(mutex *m)
{
    disable_interrupts();

    if (!m->locked)
    {

        m->locked = 1;
    }
    else
    {

        if (m->waiting_queue == NULL)
        {
            m->waiting_queue = current_thread;
            current_thread->waiting_next = NULL;
        }
        else
        {
            thread_t *temp = m->waiting_queue;
            while (temp->waiting_next != NULL)
            {
                temp = temp->waiting_next;
            }
            temp->waiting_next = current_thread;
            current_thread->waiting_next = NULL;
        }

        thread_block();
    }

    enable_interrupts();
}

void mutex_release(mutex *m)
{
    disable_interrupts();

    if (m->waiting_queue != NULL)
    {

        thread_t *next_thread = m->waiting_queue;
        m->waiting_queue = next_thread->waiting_next;
        next_thread->waiting_next = NULL;
        thread_unblock(next_thread);

        thread_yield();
    }
    else
    {

        m->locked = 0;
    }

    enable_interrupts();
}
