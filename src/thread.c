void thread_start(void);

void __attribute__((used)) scheduler();

#include "thread.h"
#include "jmpbuf-offsets.h"
#include "util.h"
#include "time_control.h"

extern struct itimerval timer;
thread_t *current_thread = NULL;

typedef struct mutex_t
{
    int locked;
    thread_t *waiting_queue;
} mutex;

typedef struct semaphore_t
{
    int value;
    thread_t *waiting_queue;
} semaphore_t;

static thread_t *threads = NULL;
static int num_threads = 0;
static int initialized = 0;

sigjmp_buf main_context;

void thread_yield();
void context_switch(thread_t *old_thread, thread_t *new_thread);
void init_lib();
int get_time();
void timer_start();
void timer_stop();

int thread_id = 0;

void init_lib(void)
{
    if (!initialized)
    {
        threads = (thread_t *)calloc(MAX_THREADS, sizeof(thread_t));
        if (!threads)
        {

            exit(1);
        }
        initialized = 1;
        num_threads = 0;
        current_thread = NULL;
    }
}

unsigned long align_stack(unsigned long sp)
{
    return sp & ~0xF;
}

int thread_create(thread_t **thread, void (*start_routine)(void *), void *arg)
{
    init_lib();
    if (!initialized || !threads)
    {

        return -1;
    }

    if (num_threads >= MAX_THREADS)
    {

        return -1;
    }

    thread_t *new_thread = &threads[num_threads];

    memset(new_thread, 0, sizeof(thread_t));
    new_thread->thread_id = thread_id++;
    new_thread->start_routine = start_routine;
    new_thread->arg = arg;
    new_thread->state = READY;

    new_thread->stack = malloc(STACK_SIZE);
    if (!new_thread->stack)
    {

        return -1;
    }

    if (sigsetjmp(new_thread->context, 1) == 0)
    {
        unsigned long sp = align_stack((unsigned long)(new_thread->stack + STACK_SIZE - sizeof(void *)));
        new_thread->context[0].__jmpbuf[6] = mangle(sp);

        new_thread->context[0].__jmpbuf[7] = mangle((unsigned long)thread_start);
        sigemptyset(&(new_thread->context)->__saved_mask);
    }

    if (num_threads > 0)
    {
        threads[num_threads - 1].next = new_thread;
    }
    new_thread->next = &threads[0];

    *thread = new_thread;
    num_threads++;

    return 0;
}

void thread_join(thread_t *thread)
{
    if (!thread)
        return;

    while (thread->state != EXITED)
    {
        thread_yield();
    }
}

void thread_sleep(unsigned int milliseconds)
{
    unsigned long start_time = get_time();
    while ((get_time() - start_time) < milliseconds)
    {
        thread_yield();
    }
}

void context_switch(thread_t *old_thread, thread_t *new_thread)
{
    if (old_thread != NULL)
    {
        if (sigsetjmp(old_thread->context, 1) == 0)
        {
            if (new_thread == NULL)
            {
                siglongjmp(main_context, 1);
            }
            else
            {
                current_thread = new_thread;
                siglongjmp(new_thread->context, 1);
            }
        }
    }
    else
    {
        if (new_thread != NULL)
        {
            current_thread = new_thread;
            siglongjmp(new_thread->context, 1);
        }
    }
}

void thread_start()
{
    if (current_thread)
    {

        if (current_thread->start_routine)
        {
            current_thread->state = RUNNING;
            current_thread->start_routine(current_thread->arg);
        }
        else
        {
        }

        current_thread->state = EXITED;

        thread_exit();
    }
    else
    {
    }
}

void thread_exit()
{
    if (current_thread)
    {

        current_thread->state = EXITED;
    }

    scheduler();
}

void thread_yield(void)
{
    if (!current_thread)
        return;

    current_thread->state = READY;

    scheduler();
}

thread_t *ready_queue = NULL;

void enqueue_thread(thread_t *thread)
{
    if (!ready_queue)
    {
        ready_queue = thread;
        thread->next = NULL;
    }
    else
    {
        thread_t *temp = ready_queue;
        while (temp->next)
        {
            temp = temp->next;
        }
        temp->next = thread;
        thread->next = NULL;
    }
}

thread_t *dequeue_thread()
{
    if (!ready_queue)
        return NULL;
    thread_t *thread = ready_queue;
    ready_queue = ready_queue->next;
    thread->next = NULL;
    return thread;
}

void scheduler()
{
    static thread_t *prev_thread = NULL;
    thread_t *next_thread = NULL;

    if (!current_thread)
    {
        for (int i = 0; i < num_threads; i++)
        {
            if (threads[i].state == READY)
            {
                current_thread = &threads[i];
                break;
            }
        }
        if (!current_thread)
        {

            return;
        }
    }

    if (prev_thread && prev_thread->state == EXITED && prev_thread->stack != NULL)
    {
        free(prev_thread->stack);
        prev_thread->stack = NULL;
    }

    next_thread = current_thread->next;
    while (next_thread != current_thread)
    {
        if (next_thread->state == READY)
        {
            break;
        }
        next_thread = next_thread->next;
    }

    if (next_thread == current_thread && current_thread->state != READY)
    {

        if (current_thread->state == EXITED && current_thread->stack != NULL)
        {
            free(current_thread->stack);
            current_thread->stack = NULL;
        }

        for (int i = 0; i < num_threads; i++)
        {
            if (threads[i].state == EXITED && threads[i].stack != NULL)
            {
                free(threads[i].stack);
                threads[i].stack = NULL;
            }
        }
        return;
    }

    prev_thread = current_thread;

    if (prev_thread != next_thread)
    {

        if (sigsetjmp(prev_thread->context, 1) == 0)
        {
            current_thread = next_thread;
            current_thread->state = RUNNING;
            siglongjmp(current_thread->context, 1);
        }
        else
        {
        }
    }
    else
    {

        current_thread->state = RUNNING;
    }
}

void start_scheduler()
{

    timer_start();
    scheduler();
}

void stop_scheduler()
{
    timer_stop();
}
