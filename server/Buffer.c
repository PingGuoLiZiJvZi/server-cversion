#include "Buffer.h"
#include <sys/uio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "log.h"
struct Buffer *buffer_init(int size)
{
	struct Buffer *buffer = (struct Buffer *)malloc(sizeof(struct Buffer));
	if (buffer != NULL)
	{
		buffer->data = (char *)malloc(size);
		buffer->capacity = size;
		buffer->writepos = buffer->readpos = 0;
		memset(buffer->data, 0, size);
	}
	return buffer;
}
void buffer_destroy(struct Buffer *buffer)
{
	if (buffer != NULL)
	{
		if (buffer->data != NULL)
		{
			free(buffer->data);
			buffer->data = NULL;
		}
		free(buffer);
		buffer = NULL;
	}
}
void buffer_expand(struct Buffer *buffer, int size)
{
	// storage enough
	if (buffer_writable_room(buffer) >= size)
	{
		return;
	}
	// storage enough after merge
	else if (buffer_writable_room(buffer) + buffer->readpos >= size)
	{
		int readable = buffer_readable_room(buffer);
		memcpy(buffer->data, buffer->data + buffer->readpos, readable);
		buffer->readpos = 0;
		buffer->writepos = readable;
	}
	// storage is not enough
	else
	{
		void *temp = realloc(buffer->data, buffer->capacity + size);
		if (temp == NULL)
		{
			Debug("failed to expand buffer");
			return;
		}
		memset(temp + buffer->capacity, 0, size);
		buffer->data = temp;
		buffer->capacity += size;
	}
}
int buffer_writable_room(struct Buffer *buffer)
{
	return buffer->capacity - buffer->writepos;
}
int buffer_readable_room(struct Buffer *buffer)
{
	return buffer->writepos - buffer->readpos;
}
int buffer_append_data(struct Buffer *buffer, const char *data, int size)
{
	if (buffer == NULL || data == NULL || size <= 0)
	{
		return -1;
	}
	buffer_expand(buffer, size);
	memcpy(buffer->data + buffer->writepos, data, size);
	buffer->writepos += size;
	return 0;
}

int buffer_append_string(struct Buffer *buffer, const char *data)
{
	return buffer_append_data(buffer, data, strlen(data));
}
int buffer_socket_read(struct Buffer *buffer, int fd)
{
	// read recv readv
	struct iovec vec[2];
	int writable = buffer_writable_room(buffer);
	vec[0].iov_base = buffer->data + buffer->writepos;
	vec[0].iov_len = writable;
	char *temp_buf = (char *)malloc(40960);
	vec[1].iov_base = temp_buf;
	vec[1].iov_len = 40960;
	int res = readv(fd, vec, 2);
	if (res == -1)
	{
		return -1;
	}
	else if (res <= writable)
	{
		buffer->writepos += res;
	}
	else
	{
		buffer->writepos = buffer->capacity;
		buffer_append_data(buffer, temp_buf, res - writable);
	}
	free(temp_buf);
	temp_buf = NULL;
	return res;
}
char *buffer_find(const char *source, int source_length, const char *target, int target_length)
{
	if (source_length < target_length)
		return NULL;
	for (int i = 0; i < source_length; i++)
	{
		int j = 0;
		for (; j < target_length; j++)
		{
			if (source[i + j] != target[j])
			{
				break;
			}
		}
		if (j == target_length)
		{
			return &source[i];
		}
	}
	return NULL;
}
char *buffer_find_reverse(const char *source, int source_length, const char *target, int target_length, int find_length_source)
{
	if (source_length < target_length || source_length < find_length_source || target_length > find_length_source)
		return NULL;
	for (int i = source_length - 1; i >= source_length - find_length_source; i--)
	{
		int j = target_length - 1;
		for (; j >= 0; j--)
		{
			if (source[i - target_length + 1 + j] != target[j])
			{
				break;
			}
		}
		if (j == -1)
		{
			return &source[i];
		}
	}
	return NULL;
}
int buffer_send_data(struct Buffer *buffer, int socket)
{
	int readable = buffer_readable_room(buffer);
	if (readable > 0)
	{
		Debug("start to send data:%s", buffer->data + buffer->readpos);
		int count = send(socket, buffer->data + buffer->readpos, readable, MSG_NOSIGNAL);
		if (count)
		{
			buffer->readpos += count;
			struct timespec req;
			req.tv_sec = 0;
			req.tv_nsec = 1000000L; // 1 毫秒
			nanosleep(&req, NULL);	// 使用 nanosleep 代替 usleep
		}
	}
	return 0;
}