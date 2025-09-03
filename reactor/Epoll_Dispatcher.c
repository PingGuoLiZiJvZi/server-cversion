#include "Dispatcher.h"
#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <log.h>
#define MAX 500
struct Epoll_Data
{
	int epfd;
	struct epoll_event *events;
};
static void *epoll_init();
static int epoll_add(struct Channel *channel, struct Event_Loop *evloop);
static int epoll_modify(struct Channel *channel, struct Event_Loop *evloop);
static int epoll_remove(struct Channel *channel, struct Event_Loop *evloop);
static int epoll_dispatch(struct Event_Loop *evloop, int time_out);
static int epoll_clear(struct Event_Loop *evloop);
static int epoll_control(struct Channel *channel, struct Event_Loop *evloop, int option);
struct Dispatcher epoll_dispatcher = {
	epoll_init,
	epoll_add,
	epoll_modify,
	epoll_remove,
	epoll_dispatch,
	epoll_clear};
static void *epoll_init()
{
	struct Epoll_Data *data = (struct Epoll_Data *)malloc(sizeof(struct Epoll_Data));
	data->epfd = epoll_create(1);
	if (data->epfd == -1)
	{
		perror("epoll_create");
		exit(-1);
	}
	data->events = (struct epoll_event *)calloc(MAX, sizeof(struct epoll_event));
	return data;
}
static int epoll_add(struct Channel *channel, struct Event_Loop *evloop)
{
	Debug("socket %d was added into epoll dispatcher of %s", channel->fd, evloop->thread_name);
	int res = epoll_control(channel, evloop, EPOLL_CTL_ADD);
	if (res == -1)
	{
		perror("epoll control add");
		exit(-1);
	}

	return res;
}
static int epoll_modify(struct Channel *channel, struct Event_Loop *evloop)
{
	int res = epoll_control(channel, evloop, EPOLL_CTL_MOD);
	if (res == -1)
	{
		perror("epoll control mod");
		exit(-1);
	}
	return res;
}
static int epoll_remove(struct Channel *channel, struct Event_Loop *evloop)
{
	int res = epoll_control(channel, evloop, EPOLL_CTL_DEL);
	if (res == -1)
	{
		perror("epoll control del");
		exit(-1);
	}
	channel->destroy_call_back(channel->arg);
	return res;
}
static int epoll_control(struct Channel *channel, struct Event_Loop *evloop, int option)
{
	struct Epoll_Data *data = (struct Epoll_Data *)evloop->dispatcher_data;
	struct epoll_event ev;
	ev.data.fd = channel->fd;
	int events = 0;
	if (channel->events & READEVENT)
	{
		events |= EPOLLIN;
	}
	if (channel->events & WRITEEVENT)
	{
		events |= EPOLLOUT;
	}
	ev.events = events;
	int res = epoll_ctl(data->epfd, option, channel->fd, &ev);
	return res;
}
static int epoll_dispatch(struct Event_Loop *evloop, int time_out)
{
	struct Epoll_Data *data = (struct Epoll_Data *)evloop->dispatcher_data;
	int counts = epoll_wait(data->epfd, data->events, MAX, time_out * 1000);
	for (int i = 0; i < counts; i++)
	{
		int events = data->events[i].events;
		int fd = data->events[i].data.fd;
		if (events & EPOLLERR || events & EPOLLHUP)
		{
			Debug("channel %d has error", fd);
			epoll_remove(evloop->channel_map->list[fd], evloop);
		}
		if (events & EPOLLIN)
		{
			event_activate(evloop, fd, READEVENT);
		}
		if (events & EPOLLOUT)
		{
			event_activate(evloop, fd, WRITEEVENT);
		}
	}
	return 0;
}
static int epoll_clear(struct Event_Loop *evloop)
{
	struct Epoll_Data *data = (struct Epoll_Data *)evloop->dispatcher_data;
	free(data->events);
	data->events = NULL;
	close(data->epfd);
	free(data);
	data = NULL;
	return 0;
}