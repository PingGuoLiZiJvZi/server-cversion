#include "Thread_Pool.h"
#include <stdlib.h>
#include <assert.h>
struct Thread_Pool *thread_pool_init(struct Event_Loop *mainloop, int count)
{
    struct Thread_Pool *thread_pool = (struct Thread_Pool *)malloc(sizeof(struct Thread_Pool));
    thread_pool->index = 0;
    thread_pool->is_start = false;
    thread_pool->mainloop = mainloop;
    thread_pool->thread_num = count;
    thread_pool->worker_threads = (struct Worker_Thread *)malloc(sizeof(struct Worker_Thread) * count);
    return thread_pool;
}
void thread_pool_run(struct Thread_Pool *pool)
{
    assert(pool && !pool->is_start);
    if (pool->mainloop->threadid != pthread_self())
    {
        exit(-1);
    }
    pool->is_start = true;
    if (pool->thread_num > 0)
    {
        for (int i = 0; i < pool->thread_num; i++)
        {
            worker_thread_init(&pool->worker_threads[i], i);
            worker_thread_run(&pool->worker_threads[i]);
        }
    }
}
struct Event_Loop *take_worker_event_loop(struct Thread_Pool *pool)
{
    assert(pool->is_start);
    if (pool->mainloop->threadid != pthread_self())
    {
        exit(-1);
    }
    struct Event_Loop *evloop = pool->mainloop;
    if (pool->thread_num > 0)
    {
        evloop = pool->worker_threads[pool->index].evloop;
        pool->index = (pool->index + 1) % pool->thread_num;
    }
    return evloop;
}