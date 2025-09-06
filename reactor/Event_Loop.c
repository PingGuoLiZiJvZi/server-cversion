#include "Event_Loop.h"
#include <string.h>
#include "assert.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <log.h>
struct Event_Loop *main_event_loop_init()
{
	Debug("the id of main thread is %lu", pthread_self());
	return event_loop_init(NULL);
}
void task_wake_up(struct Event_Loop *evloop)
{
	const char *message = "wake up the subthread\n";
	write(evloop->socket_pair[0], message, strlen(message));
}
int read_local_message(void *arg)
{
	struct Event_Loop *evloop = (struct Event_Loop *)arg;
	char buf[256] = {0};
	read(evloop->socket_pair[1], buf, sizeof(buf));
	return 0;
}
struct Event_Loop *event_loop_init(const char *thread_name)
{
	struct Event_Loop *evloop = (struct Event_Loop *)malloc(sizeof(struct Event_Loop));
	evloop->is_quit = false;
	evloop->threadid = pthread_self();
	pthread_mutex_init(&evloop->mutex, NULL);
	strcpy(evloop->thread_name, (thread_name == NULL) ? "main_thread" : thread_name);
	evloop->dispatcher = &epoll_dispatcher; // select the underlying model
	evloop->dispatcher_data = evloop->dispatcher->init();
	evloop->head = NULL;
	evloop->tail = NULL;
	evloop->channel_map = channel_map_init(128);
	int res = socketpair(AF_UNIX, SOCK_STREAM, 0, evloop->socket_pair);
	if (res == -1)
	{
		perror("socketpair");
		exit(-1);
	} // pair[0] send pair[1] recv
	struct Channel *channel = channel_init(evloop->socket_pair[1], READEVENT, read_local_message, NULL, NULL, evloop);
	// add channel to task
	event_loop_add_task(evloop, channel, ADD);
	return evloop;
}
int event_loop_run(struct Event_Loop *evloop)
{
	assert(evloop != NULL);
	struct Dispatcher *dispatcher = evloop->dispatcher;
	if (evloop->threadid != pthread_self())
	{
		return -1;
	}
	Debug("the event loop of thread %s is running", evloop->thread_name);
	while (!evloop->is_quit)
	{
		event_loop_process_task(evloop);
		dispatcher->dispatch(evloop, 2);
	}
	Debug("the event loop of thread %s quit", evloop->thread_name);
	return 0;
}
int event_activate(struct Event_Loop *evloop, int fd, int event)
{
	if (fd < 0 || evloop == NULL)
	{
		return -1;
	}
	struct Channel *channel = evloop->channel_map->list[fd];
	if (channel == NULL)
	{
		return -1;
	}
	assert(fd == channel->fd);
	if (event & READEVENT && channel->read_call_back != NULL)
	{
		channel->read_call_back(channel->arg);
	}
	if (event & WRITEEVENT && channel->write_call_back != NULL)
	{
		channel->write_call_back(channel->arg);
	}
	return 0;
}
int event_loop_add_task(struct Event_Loop *evloop, struct Channel *channel, int type)
{
	// create new node
	struct Channel_Element *node = (struct Channel_Element *)malloc(sizeof(struct Channel_Element));
	node->channel = channel;
	node->type = type;
	node->next = NULL;
	Debug("task %d has added into the evloop %s for socket %d", type, evloop->thread_name, channel->fd);
	// lock to protect data
	pthread_mutex_lock(&evloop->mutex);
	if (evloop->head == NULL)
	{
		evloop->head = node;
		evloop->tail = node;
	}
	else
	{
		evloop->tail->next = node;
		evloop->tail = node;
	}
	pthread_mutex_unlock(&evloop->mutex);
	// process the node DONT let the main thread to process the node
	// the main thread could only add node
	if (pthread_self() == evloop->threadid)
	{
		// event_loop_process_task(evloop);
	}
	else
	{
		task_wake_up(evloop);
		// notice the sub thread
		// 1.subthread is working
		// 2.subthread is blocked by dispatcher 1.select 2.poll 3.epoll
		//(add an extra fd to subthread to activate it)
	}
	return 0;
}
int event_loop_process_task(struct Event_Loop *evloop)
{
	pthread_mutex_lock(&evloop->mutex);
	struct Channel_Element *head = evloop->head;
	while (head != NULL)
	{
		struct Channel *channel = head->channel;
		if (head->type == ADD)
		{
			Debug("socket %d was added into the evloop %s", channel->fd, evloop->thread_name);
			// if(channel->fd==3){
			//     printf("???");
			// }
			event_loop_add(evloop, channel);
		}
		else if (head->type == DELETE)
		{
			Debug("socket %d was deleted from the evloop %s", channel->fd, evloop->thread_name);
			event_loop_del(evloop, channel);
		}
		else if (head->type == MODIFY)
		{
			Debug("socket %d was modified in the evloop %s", channel->fd, evloop->thread_name);
			event_loop_mod(evloop, channel);
		}
		struct Channel_Element *temp = head->next;
		free(head);
		head = temp;
	}
	evloop->head = evloop->tail = NULL;
	pthread_mutex_unlock(&evloop->mutex);
	return 0;
}
int event_loop_add(struct Event_Loop *evloop, struct Channel *channel)
{
	int fd = channel->fd;
	struct Channel_Map *channel_map = evloop->channel_map;
	if (fd >= channel_map->size)
	{
		// there is no enough space to store fd
		if (!expand_channel_map(channel_map, fd, sizeof(struct Channel *)))
		{
			return -1;
		}
	}
	// find space of fd
	if (channel_map->list[fd] == NULL)
	{
		channel_map->list[fd] = channel;
		Debug("socket %d was added into the channel map of %s", fd, evloop->thread_name);
		evloop->dispatcher->add(channel, evloop);
	}
	return 0;
}
int event_loop_del(struct Event_Loop *evloop, struct Channel *channel)
{
	int fd = channel->fd;
	struct Channel_Map *channel_map = evloop->channel_map;
	if (fd >= channel_map->size || channel_map->list[fd] == NULL)
	{
		return -1;
	}
	// find space of fd
	int res = evloop->dispatcher->remove(channel, evloop);
	return res;
}
int event_loop_mod(struct Event_Loop *evloop, struct Channel *channel)
{
	int fd = channel->fd;
	struct Channel_Map *channel_map = evloop->channel_map;
	if (fd >= channel_map->size || channel_map->list[fd] == NULL)
	{
		return -1;
	}
	// find space of fd

	int res = evloop->dispatcher->modify(channel, evloop);
	return res;
}
int destroy_channel(struct Event_Loop *evloop, struct Channel *channel)
{
	// delete the map relation between channel and fd
	evloop->channel_map->list[channel->fd] = NULL;
	close(channel->fd);
	free(channel);
	channel = NULL;
	return 0;
}