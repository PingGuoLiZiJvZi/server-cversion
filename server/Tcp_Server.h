#pragma once
#include "Event_Loop.h"
#include "Thread_Pool.h"
#include<stdlib.h>
struct Listener
{
    int listenfd;
    unsigned short port;
};
struct Tcp_Server
{
    struct Event_Loop *mainloop;
    struct Thread_Pool *thread_pool;
    struct Listener *listener;
    int thread_num;
};
//init
struct Tcp_Server *tcp_server_init(unsigned short port, int thread_num);
//init listen
struct Listener *listener_init(unsigned short port);
unsigned int ipv4_to_int(const char *ip);
void tcp_server_run(struct Tcp_Server *tcp_server);
int accept_connection(void *arg);
