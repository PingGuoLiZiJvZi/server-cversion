#include "Dispatcher.h"
#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#define MAX 1024
struct Poll_Data
{
	int maxfd;
	struct pollfd *fds[MAX];
};
static void *poll_init();
static int poll_add(struct Channel *channel, struct Event_Loop *evloop);
static int poll_modify(struct Channel *channel, struct Event_Loop *evloop);
static int poll_remove(struct Channel *channel, struct Event_Loop *evloop);
static int poll_dispatch(struct Event_Loop *evloop, int time_out);
static int poll_clear(struct Event_Loop *evloop);
static int poll_control(struct Channel *channel, struct Event_Loop *evloop, int option);
struct Dispatcher poll_dispatcher = {
	poll_init,
	poll_add,
	poll_modify,
	poll_remove,
	poll_dispatch,
	poll_clear};
static void *poll_init()
{
	struct Poll_Data *data = (struct Poll_Data *)malloc(sizeof(struct Poll_Data));
	data->maxfd = 0;
	for (int i = 0; i < MAX; i++)
	{
		data->fds[i] = (struct pollfd *)malloc(sizeof(struct pollfd));
		data->fds[i]->fd = -1;
		data->fds[i]->events = 0;
		data->fds[i]->revents = 0;
	}
	return data;
}
static int poll_add(struct Channel *channel, struct Event_Loop *evloop)
{
	struct Poll_Data *data = (struct Poll_Data *)evloop->dispatcher_data;
	int events = 0;
	if (channel->events & READEVENT)
	{
		events |= POLLIN;
	}
	if (channel->events & WRITEEVENT)
	{
		events |= POLLOUT;
	}
	int i = 0;
	for (; i < MAX; i++)
	{

		if (data->fds[i]->fd == -1)
		{
			data->fds[i]->events = events;
			data->fds[i]->fd = channel->fd;
			data->maxfd = ((data->maxfd < channel->fd) ? channel->fd : data->maxfd);
			Debug("socket %d was add into poll dispatcher of %s,maxfd is %d", channel->fd, evloop->thread_name, data->maxfd);
			break;
		}
	}
	if (i >= MAX)
	{
		return -1;
	}
	return 0;
}
static int poll_modify(struct Channel *channel, struct Event_Loop *evloop)
{
	struct Poll_Data *data = (struct Poll_Data *)evloop->dispatcher_data;
	int events = 0;
	if (channel->events & READEVENT)
	{
		events |= POLLIN;
	}
	if (channel->events & WRITEEVENT)
	{
		events |= POLLOUT;
	}
	int i = 0;
	for (; i < MAX; i++)
	{

		if (data->fds[i]->fd == channel->fd)
		{
			data->fds[i]->events = events;
			break;
		}
	}
	if (i >= MAX)
	{
		return -1;
	}
	return 0;
}
static int poll_remove(struct Channel *channel, struct Event_Loop *evloop)
{
	struct Poll_Data *data = (struct Poll_Data *)evloop->dispatcher_data;
	int i = 0;
	for (; i < MAX; i++)
	{
		if (data->fds[i]->fd == channel->fd)
		{
			data->fds[i]->events = 0;
			data->fds[i]->revents = 0;
			data->fds[i]->fd = -1;
			break;
		}
	}
	channel->destroy_call_back(channel->arg);
	if (i == MAX)
	{
		return -1;
	}
	return 0;
}
static int poll_dispatch(struct Event_Loop *evloop, int time_out)
{
	struct Poll_Data *data = (struct Poll_Data *)evloop->dispatcher_data;
	int counts = poll(*data->fds, data->maxfd + 1, time_out * 1000);
	if (counts == -1)
	{
		perror("poll");
		exit(-1);
	}
	for (int i = 0; i < data->maxfd; i++)
	{
		if (data->fds[i]->fd == -1)
		{
			continue;
		}
		int events = data->fds[i]->revents;
		int fd = data->fds[i]->fd;
		// if (events & EPOLLERR || events & EPOLLHUP)
		// {
		//     // epoll_remove(Channel, evloop);
		//     continue;
		// }
		if (events & POLLIN)
		{
			event_activate(evloop, fd, READEVENT);
		}
		if (events & POLLOUT)
		{
			event_activate(evloop, fd, WRITEEVENT);
		}
	}
	return 0;
}
static int poll_clear(struct Event_Loop *evloop)
{
	struct Poll_Data *data = (struct Poll_Data *)evloop->dispatcher_data;
	for (int i = 0; i < MAX; i++)
	{
		free(data->fds[i]);
		data->fds[i] = NULL;
	}
	free(data);
	data = NULL;
	return 0;
}
