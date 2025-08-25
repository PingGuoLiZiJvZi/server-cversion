#pragma once 
#include"Event_Loop.h"
#include"Channel.h"
struct Dispatcher
{
    void *(*init)();
    int (*add)(struct Channel *channel, struct Event_Loop *evloop);
    int (*modify)(struct Channel *channel, struct Event_Loop *evloop);
    int(*remove)(struct Channel *channel, struct Event_Loop *evloop);
    int(*dispatch)(struct Event_Loop *evloop,int time_out);
    int(*clear)(struct Event_Loop *evloop);
};
