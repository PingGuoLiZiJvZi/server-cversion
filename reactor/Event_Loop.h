#pragma once
#include "Dispatcher.h"
#include <stdbool.h>
#include "Channel_Map.h"
#include "pthread.h"
#include <sys/socket.h>
extern struct Dispatcher epoll_dispatcher;
extern struct Dispatcher poll_dispatcher;
extern struct Dispatcher select_dispatcher;
enum Elem_Type
{
    ADD,
    DELETE,
    MODIFY
};
struct Channel_Element
{
    int type;
    struct Channel *channel;
    struct Channel_Element *next;
};
struct Event_Loop
{
    bool is_quit;
    struct Dispatcher *dispatcher;
    void *dispatcher_data;
    struct Channel_Element *head;
    struct Channel_Element *tail;
    struct Channel_Map *channel_map;
    pthread_t threadid;
    char thread_name[32];
    pthread_mutex_t mutex;
    int socket_pair[2];
};
struct Event_Loop *event_loop_init(const char *thread_name);
struct Event_Loop *main_event_loop_init();
int event_loop_run(struct Event_Loop *evloop);
int read_local_message(void *arg);
// deal the fd activated
int event_activate(struct Event_Loop *evloop, int fd, int event);
int event_loop_add_task(struct Event_Loop *evloop, struct Channel *channel, int type);
void task_wake_up(struct Event_Loop *evloop);
int event_loop_process_task(struct Event_Loop *evloop);
int event_loop_add(struct Event_Loop *evloop, struct Channel *channel);
int event_loop_del(struct Event_Loop *evloop, struct Channel *channel);
int event_loop_mod(struct Event_Loop *evloop, struct Channel *channel);
int destroy_channel(struct Event_Loop *evloop, struct Channel *channel);