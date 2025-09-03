#include "Http_Response.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define RES_HEADER_SIZE 16
struct Http_Response *http_response_init()
{
	struct Http_Response *response = (struct Http_Response *)malloc(sizeof(struct Http_Response));
	response->header_num = 0;
	int size = sizeof(struct Response_Header) * RES_HEADER_SIZE;
	response->headers = (struct Response_Header *)malloc(size);
	response->status_code = UNKNOWN;
	memset(response->headers, 0, size);
	memset(response->status_message, 0, sizeof(response->status_message));
	memset(response->file_name, 0, sizeof(response->file_name));
	response->send_data_func = NULL;
	return response;
}
void http_response_destroy(struct Http_Response *response)
{
	if (response == NULL)
	{
		return;
	}
	free(response->headers);
	response->headers = NULL;
	free(response);
	response = NULL;
	return;
}
void http_response_add_headers(struct Http_Response *response, const char *key, const char *value)
{
	if (response == NULL || key == NULL || value == NULL)
	{
		return;
	}
	memcpy(response->headers[response->header_num].key, key, strlen(key) + 1);
	memcpy(response->headers[response->header_num].value, value, strlen(value) + 1);
	response->header_num++;
	return;
}
void http_response_prepare(struct Http_Response *response, struct Buffer *sendbuf, int socket)
{
	// state line
	char temp[1024] = {0};
	sprintf(temp, "HTTP/1.1 %d %s\r\n", response->status_code, response->status_message);
	buffer_append_string(sendbuf, temp);
	// response head
	for (int i = 0; i < response->header_num; i++)
	{
		sprintf(temp, "%s: %s\r\n", response->headers[i].key, response->headers[i].value);
		buffer_append_string(sendbuf, temp);
#ifndef WHOLE_SEND
		buffer_send_data(sendbuf, socket);
#endif
	}
	// empty
	buffer_append_string(sendbuf, "\r\n");
#ifndef WHOLE_SEND
	buffer_send_data(sendbuf, socket);
#endif
	// reponse data
	response->send_data_func(response->file_name, sendbuf, socket);
	return;
}