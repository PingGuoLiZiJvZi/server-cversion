#include "Worker_Thread.h"
#include <stdio.h>
int worker_thread_init(struct Worker_Thread *thread, int index)
{
    thread->evloop = NULL;
    thread->threadid = 0;
    sprintf(thread->name, "subthread-%d", index);
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);
    return 0;
}
void *subthread_running(void *arg)
{
    struct Worker_Thread *thread = (struct Worker_Thread *)arg;
    pthread_mutex_lock(&thread->mutex);
    thread->evloop = event_loop_init(thread->name);
    pthread_mutex_unlock(&thread->mutex);
    pthread_cond_signal(&thread->cond);
    event_loop_run(thread->evloop);
    return NULL;
}
void worker_thread_run(struct Worker_Thread *thread)
{
    pthread_create(&thread->threadid, NULL, subthread_running, thread);
    // block the main thread
    pthread_mutex_lock(&thread->mutex);
    while (thread->evloop == NULL)
    {
        pthread_cond_wait(&thread->cond, &thread->mutex);
    }
    pthread_mutex_unlock(&thread->mutex);
}