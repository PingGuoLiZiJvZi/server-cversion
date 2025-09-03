#include "Tcp_Connection.h"
#include "log.h"
#include <stdio.h>
// #define WHOLE_SEND

struct Tcp_Connection *tcp_connection_init(int fd, struct Event_Loop *evloop)
{
	struct Tcp_Connection *conn = (struct Tcp_Connection *)malloc(sizeof(struct Tcp_Connection));
	conn->request = http_request_init();
	conn->response = http_response_init();
	conn->evloop = evloop;
	conn->readbuf = buffer_init(10240);
	conn->writebuf = buffer_init(10240);
	sprintf(conn->name, "connection-%d", fd);
	conn->channel = channel_init(fd, READEVENT, process_read, process_write, tcp_connection_destroy, conn);
	event_loop_add_task(evloop, conn->channel, ADD);
	Debug("build connection with client,thread name: %s,thread id: %lu,connect name: %s",
		  evloop->thread_name, evloop->threadid, conn->name);
	return conn;
}
int process_read(void *arg)
{
	struct Tcp_Connection *tcp_connection = (struct Tcp_Connection *)arg;
	int res = buffer_socket_read(tcp_connection->readbuf, tcp_connection->channel->fd);
	Debug("the http request recved: %s", tcp_connection->readbuf->data + tcp_connection->readbuf->readpos);
	// 若单次读取内容与content-length不符，就再次读取，此次读取直接写入文件
	if (res > 0)
	{
		int sock = tcp_connection->channel->fd;
#ifdef WHOLE_SEND
		write_event_enable(tcp_connection->channel, true);
		event_loop_add_task(tcp_connection->evloop, tcp_connection->channel, MODIFY);
#endif
		bool flag = parse_http_request(tcp_connection->request, tcp_connection->readbuf, tcp_connection->response, tcp_connection->writebuf, sock);

		if (!flag)
		{
			char *error_mes = "Http/1.1 400 Bad Request\r\n\r\n";
			buffer_append_string(tcp_connection->writebuf, error_mes);
		}
	}
	else if (res == 0)
	{
		Debug("client closed connection");
		event_loop_add_task(tcp_connection->evloop, tcp_connection->channel, DELETE);
	}
	else
	{
		Debug("failed to read data from client");
		event_loop_add_task(tcp_connection->evloop, tcp_connection->channel, DELETE);
	}
#ifndef WHOLE_SEND
	event_loop_add_task(tcp_connection->evloop, tcp_connection->channel, DELETE);
#endif
	return 0;
}
int process_write(void *arg)
{
	Debug("start to send data(based on the write event)...");
	struct Tcp_Connection *tcp_connection = (struct Tcp_Connection *)arg;
	int count = buffer_send_data(tcp_connection->writebuf, tcp_connection->channel->fd);
	if (count > 0)
	{
		if (buffer_readable_room(tcp_connection->readbuf) == 0)
		{
			write_event_enable(tcp_connection->channel, false);
			event_loop_add_task(tcp_connection->evloop, tcp_connection->channel, MODIFY);
			event_loop_add_task(tcp_connection->evloop, tcp_connection->channel, DELETE);
		}
	}
	return 0;
}
int tcp_connection_destroy(void *arg)
{
	struct Tcp_Connection *conn = (struct Tcp_Connection *)arg;
	if (conn == NULL)
	{
		return -1;
	}
	if (conn->readbuf && conn->writebuf)
	{
		Debug("connection closed connection name: %s", conn->name);
		destroy_channel(conn->evloop, conn->channel);
		Debug("after destroy channel");
		buffer_destroy(conn->readbuf);
		Debug("after destroy readbuf");
		buffer_destroy(conn->writebuf);
		Debug("after destroy writebuf");
		http_request_destroy(conn->request);
		Debug("after destroy request");
		http_response_destroy(conn->response);
		Debug("after destroy response");
		free(conn);
		conn = NULL;
		Debug("after free conn");
	}

	return 0;
}