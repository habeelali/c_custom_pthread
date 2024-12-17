#include "time_control.h"
#include "util.h"
#include "thread.h"
#include <signal.h>
#include <stdio.h>

void signal_handler(int signum)
{

    scheduler();
}

struct itimerval timer;

sigset_t signal_mask;

void interrupt_generated(int signum);

int get_time()
{
    struct timeval t;
    gettimeofday(&t, NULL);

    return t.tv_sec * 1000 + t.tv_usec / 1000;
}

void timer_start()
{
    struct sigaction sa;
    struct itimerval timer;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGALRM, &sa, NULL) == -1)
    {
        perror("Error: cannot handle SIGALRM");
        exit(1);
    }

    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
    {
        perror("Error: cannot set timer");
        exit(1);
    }
}

void timer_stop()
{
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &timer, NULL);
}

void interrupt_generated(int signum)
{
}
