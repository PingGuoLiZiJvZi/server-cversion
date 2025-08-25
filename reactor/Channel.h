#pragma once
#include <stdbool.h>
typedef int (*Handle_Func)(void *arg);
enum FD_Event
{
    READEVENT = 0x02,
    WRITEEVENT = 0x04,
};
struct Channel
{
    int fd;
    int events;
    // events call back funv ptr
    Handle_Func read_call_back;
    Handle_Func write_call_back;
    Handle_Func destroy_call_back;
    void *arg;
};
// init channel
struct Channel *channel_init(int fd, int events, Handle_Func read_func,
                             Handle_Func write_func, Handle_Func destroy_call_back, void *arg);
void write_event_enable(struct Channel *channel, bool flag);
bool is_write_event_enable(struct Channel *channel);