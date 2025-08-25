#pragma once
#include <pthread.h>
#include "Event_Loop.h"
struct Worker_Thread
{
    pthread_t threadid;
    char name[24];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    struct Event_Loop *evloop;
};
int worker_thread_init(struct Worker_Thread *thread, int index); // index of the thread_pool
void worker_thread_run(struct Worker_Thread *thread);
void *subthread_running(void *arg);
