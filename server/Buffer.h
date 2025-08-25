#pragma once
#include <stdlib.h>

struct Buffer
{
    char *data;
    int capacity;
    int readpos;
    int writepos;
};
struct Buffer *buffer_init(int size);
void buffer_destroy(struct Buffer *buffer);
void buffer_expand(struct Buffer *buffer, int size);
int buffer_writable_room(struct Buffer *buffer);
int buffer_readable_room(struct Buffer *buffer);
// write directly
int buffer_append_data(struct Buffer *buffer, const char *data, int size);
int buffer_append_string(struct Buffer *buffer, const char *data);
// socket write
int buffer_socket_read(struct Buffer *buffer, int fd);
char *buffer_find(const char *source, int source_length, const char *target, int target_length);
char *buffer_find_reverse(const char *source, int source_length, const char *target, int target_length, int find_length_source);
int buffer_send_data(struct Buffer *buffer, int socket);
