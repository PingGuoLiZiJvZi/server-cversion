#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "Buffer.h"
#include "Http_Response.h"
#include "Tcp_Connection.h"
#define HEADER_SIZE 32
struct Http_Header
{
    char *key;
    char *value;
};
enum Http_Request_State
{
    PARSEREQLINE,
    PARSEREQHEADERS,
    PARSEREAQBODY,
    PARSEREQDONE
};
struct Http_Request
{
    char *method;
    char *url;
    char *version;
    char *filename;
    char *boundary;
    struct Http_Header *reqheaders;
    int reqheader_nums;
    enum Http_Request_State cur_state;
};
struct Http_Request *http_request_init();
void http_request_reset(struct Http_Request *request);
void http_request_destroy(struct Http_Request *request);
void http_request_free(struct Http_Request *request);
enum Http_Request_State http_request_state(struct Http_Request *request);
void http_request_add_header(struct Http_Request *request, const char *key, const char *value);
char *http_request_get_header(struct Http_Request *request, const char *key);
bool parse_http_request_line(struct Http_Request *request, struct Buffer *readbuf);
char *parse_http_request_line_one(char **dest, char *start, char *end, char *sub);
bool parse_http_request_header(struct Http_Request *request, struct Buffer *readbuf);
bool parse_http_request(struct Http_Request *request, struct Buffer *readbuf, struct Http_Response *response, struct Buffer *sendbuf, int socket);
bool parse_http_request_body(struct Http_Request *request, struct Buffer *readbuf);
bool write_posted_file(struct Http_Request *request, struct Buffer *readbuf, int socket);
bool process_http_request(struct Http_Request *request, struct Http_Response *response);
int trans_utf8(char *out, char *in);
char trans_hex_to_dec(char in);
const char *get_file_type(const char *name);
void send_dir(const char *dir_name, struct Buffer *sendbuf, int communicate_socket);
void send_file(const char *file_name, struct Buffer *sendbuf, int communicate_socket);
