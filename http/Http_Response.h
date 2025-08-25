#pragma once
#include "Buffer.h"
enum Http_Status_Code
{
    UNKNOWN = 0,
    OK = 200,
    PAR_MOVED = 301,
    TEMP_MOVED = 302,
    BAD_REQ = 400,
    NO_FOUND = 404
};
struct Response_Header
{
    char key[32];
    char value[128];
};
typedef void (*Response_Body)(const char *file_name, struct Buffer *sendbuf, int socket);
struct Http_Response
{
    enum Http_Status_Code status_code;
    char status_message[128];
    char file_name[128];
    struct Response_Header *headers;
    int header_num;
    Response_Body send_data_func;
    // state line/ state code state descibe state version
};
struct Http_Response *http_response_init();
void http_response_destroy(struct Http_Response *response);
void http_response_add_headers(struct Http_Response *response, const char *key, const char *value);
void http_response_prepare(struct Http_Response *response, struct Buffer *sendbuf, int socket);