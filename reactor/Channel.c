#include <stdlib.h>
#include "Channel.h"
struct Channel *channel_init(int fd, int events, Handle_Func read_func,
                             Handle_Func write_func, Handle_Func destroy_call_back, void *arg)
{
    struct Channel *channel = (struct Channel *)malloc(sizeof(struct Channel));
    channel->fd = fd;
    channel->events = events;
    channel->read_call_back = read_func;
    channel->write_call_back = write_func;
    channel->destroy_call_back = destroy_call_back;
    channel->arg = arg;
    return channel;
}
void write_event_enable(struct Channel *channel, bool flag)
{
    if (flag)
    {
        channel->events |= WRITEEVENT;
    }
    else
    {
        channel->events = channel->events & (~WRITEEVENT);
    }
}
bool is_write_event_enable(struct Channel *channel)
{
    return channel->events & WRITEEVENT;
}
