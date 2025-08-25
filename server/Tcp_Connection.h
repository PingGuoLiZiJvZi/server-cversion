#pragma once
#include "Buffer.h"
#include "Channel.h"
#include "Event_Loop.h"
#include "Http_Request.h"
#include "Http_Response.h"
struct Tcp_Connection
{
    char name[32];
    struct Event_Loop *evloop;
    struct Channel *channel;
    struct Buffer *readbuf;
    struct Buffer *writebuf;
    struct Http_Request *request;
    struct Http_Response *response;
};
struct Tcp_Connection *tcp_connection_init(int fd, struct Event_Loop *evloop);
int process_read(void *arg);
int process_write(void *arg);
int tcp_connection_destroy(void *arg);