#include "Dispatcher.h"
#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#define MAX 1024
struct Select_Data
{
    fd_set read_set;
    fd_set write_set;
    fd_set except_set;
};
static void *select_init();
static int select_add(struct Channel *channel, struct Event_Loop *evloop);
static int select_modify(struct Channel *channel, struct Event_Loop *evloop);
static int select_remove(struct Channel *channel, struct Event_Loop *evloop);
static int select_dispatch(struct Event_Loop *evloop, int time_out);
static int select_clear(struct Event_Loop *evloop);
static int select_control(struct Channel *channel, struct Event_Loop *evloop, int option);
// static void set_fd_set(struct Channel *channel,struct Select_Data*data);
// static void clear_fd_set(struct Channel *channel,struct Select_Data*data);
struct Dispatcher select_dispatcher = {
    select_init,
    select_add,
    select_modify,
    select_remove,
    select_dispatch,
    select_clear};
static void *select_init()
{
    struct Select_Data *data = (struct Select_Data *)malloc(sizeof(struct Select_Data));
    FD_ZERO(&data->except_set);
    FD_ZERO(&data->read_set);
    FD_ZERO(&data->write_set);
    return data;
}
static int select_add(struct Channel *channel, struct Event_Loop *evloop)
{
    struct Select_Data *data = (struct Select_Data *)evloop->dispatcher_data;
    int events = 0;
    if (channel->fd >= MAX)
    {
        return -1;
    }
    if (channel->events & READEVENT)
    {
        FD_SET(channel->fd, &data->read_set);
    }
    if (channel->events & WRITEEVENT)
    {
        FD_SET(channel->fd, &data->write_set);
    }
    return 0;
}
static int select_modify(struct Channel *channel, struct Event_Loop *evloop)
{
    struct Select_Data *data = (struct Select_Data *)evloop->dispatcher_data;
    if (channel->fd >= MAX)
    {
        return -1;
    }
    if (channel->events & READEVENT)
    {
        FD_SET(channel->fd, &data->read_set);
        FD_CLR(channel->fd, &data->write_set);
    }
    if (channel->events & WRITEEVENT)
    {
        FD_SET(channel->fd, &data->write_set);
        FD_CLR(channel->fd, &data->read_set);
    }
    return 0;
}
static int select_remove(struct Channel *channel, struct Event_Loop *evloop)
{
    struct Select_Data *data = (struct Select_Data *)evloop->dispatcher_data;
    int events = 0;
    if (channel->fd >= MAX)
    {
        return -1;
    }
    if (channel->events & READEVENT)
    {
        FD_CLR(channel->fd, &data->read_set);
    }
    if (channel->events & WRITEEVENT)
    {
        FD_CLR(channel->fd, &data->write_set);
    }
    channel->destroy_call_back(channel->arg);
    return 0;
}
static int select_dispatch(struct Event_Loop *evloop, int time_out)
{
    struct Select_Data *data = (struct Select_Data *)evloop->dispatcher_data;
    struct timeval val;
    val.tv_sec = time_out;
    val.tv_usec = 0;
    fd_set temp_read;
    fd_set temp_write;
    fd_set temp_exception;
    temp_read = data->read_set;
    temp_write = data->write_set;
    temp_exception = data->except_set;
    int counts = select(MAX, &temp_read, &temp_write, &temp_exception, &val);
    if (counts == -1)
    {
        perror("select");
        exit(-1);
    }
    for (int i = 0; i < MAX; i++)
    {
        if (FD_ISSET(i, &temp_read))
        {
            event_activate(evloop, i, READEVENT);
        }
        if (FD_ISSET(i, &temp_write))
        {
            event_activate(evloop, i, WRITEEVENT);
        }
        if (FD_ISSET(i, &temp_exception))
        {
        }
    }
    return 0;
}
static int select_clear(struct Event_Loop *evloop)
{
    struct Select_Data *data = (struct Select_Data *)evloop->dispatcher_data;
    free(data);
    return 0;
}