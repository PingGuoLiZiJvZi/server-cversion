#pragma once
#include "Event_Loop.h"
#include <stdbool.h>
#include "Worker_Thread.h"
struct Thread_Pool
{
    struct Event_Loop *mainloop;
    bool is_start;
    int thread_num;
    struct Worker_Thread *worker_threads;
    int index;
};
struct Thread_Pool *thread_pool_init(struct Event_Loop *mainloop, int count);
void thread_pool_run(struct Thread_Pool *pool);
struct Event_Loop *take_worker_event_loop(struct Thread_Pool *pool);