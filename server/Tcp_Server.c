#include "Tcp_Server.h"
#include "Tcp_Connection.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <math.h>
#include "log.h"
struct Tcp_Server *tcp_server_init(unsigned short port, int thread_num)
{
    struct Tcp_Server *tcp_server = (struct Tcp_Server *)malloc(sizeof(struct Tcp_Server));
    tcp_server->listener = listener_init(port);
    tcp_server->mainloop = main_event_loop_init();
    tcp_server->thread_num = thread_num;
    tcp_server->thread_pool = thread_pool_init(tcp_server->mainloop, thread_num);
    return tcp_server;
}
unsigned int ipv4_to_int(const char *ip)
{
    int stand = '0';
    unsigned int res = 0;
    int dig = 24;
    char num[4];
    int ava = 0;
    for (int i = 0;; i++)
    {
        if (ip[i] != '.' && ip[i] != '\0')
        {
            num[ava] = ip[i];
            ava++;
        }
        else // 2130706433
        {
            for (int j = 0; j < ava; j++)
            {
                res += (num[j] - stand) * pow(10, ava - j - 1) * pow(2, dig);
            }
            ava = 0;
            dig -= 8;
            if (ip[i] == '\0')
            {
                break;
            }
        }
    }
    return res;
}
struct Listener *listener_init(unsigned short port)
{

    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1)
    {
        perror("failed to init listen socket");
        return NULL;
    }
    int option = 1;
    int ret = setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (ret == -1)
    {
        perror("failed to reuse the port");
        return NULL;
    }
    struct sockaddr_in host_addr;
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(port);
    host_addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(listen_socket, (struct sockaddr *)&host_addr, sizeof(host_addr));
    if (ret == -1)
    {
        perror("failed to bind");
        return NULL;
    }
    ret = listen(listen_socket, 128);
    if (ret == -1)
    {
        perror("failed to listen");
        return NULL;
    }
    Debug("listen socket %d inited successfully\n", listen_socket);
    struct Listener *listener = (struct Listener *)malloc(sizeof(struct Listener));
    listener->listenfd = listen_socket;
    listener->port = port;
    return listener;
}
int accept_connection(void *arg)
{
    Debug("start to accept...");
    struct Tcp_Server *tcp_server = (struct Tcp_Server *)arg;
    int cfd = accept(tcp_server->listener->listenfd, NULL, NULL);
    struct Event_Loop *evloop = take_worker_event_loop(tcp_server->thread_pool);
    tcp_connection_init(cfd, evloop);
    return 0;
}
void tcp_server_run(struct Tcp_Server *tcp_server)
{
    thread_pool_run(tcp_server->thread_pool);
    struct Channel *channel = channel_init(tcp_server->listener->listenfd, READEVENT, accept_connection, NULL, NULL, tcp_server);
    event_loop_add_task(tcp_server->mainloop, channel, ADD);
    event_loop_run(tcp_server->mainloop);
    Debug("the server started to run...");
}