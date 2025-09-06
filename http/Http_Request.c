#include "Http_Request.h"
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <fcntl.h>
struct Http_Request *http_request_init()
{
	struct Http_Request *request = (struct Http_Request *)malloc(sizeof(struct Http_Request));
	http_request_reset(request);
	request->reqheaders = (struct Http_Header *)malloc(sizeof(struct Http_Header) * HEADER_SIZE);
	return request;
}
void http_request_reset(struct Http_Request *request)
{
	request->cur_state = PARSEREQLINE;
	request->url = NULL;
	request->version = NULL;
	request->method = NULL;
	request->boundary = NULL;
	request->filename = NULL;
	request->reqheader_nums = 0;
}
void http_request_free(struct Http_Request *request)
{
	free(request->url);
	request->url = NULL;
	free(request->method);
	request->method = NULL;
	free(request->version);
	request->version = NULL;
	if (request->boundary != NULL)
	{
		free(request->boundary);
		request->boundary = NULL;
	}
	if (request->filename != NULL)
	{
		free(request->filename);
		request->filename = NULL;
	}
	// http_request_reset(request);
}
void http_request_destroy(struct Http_Request *request)
{
	if (request == NULL)
	{
		return;
	}
	http_request_free(request);
	if (request->reqheaders != NULL)
	{
		for (int i = 0; i < request->reqheader_nums; i++)
		{
			free(request->reqheaders[i].key);
			request->reqheaders[i].key = NULL;
			free(request->reqheaders[i].value);
			request->reqheaders[i].value = NULL;
		}
		free(request->reqheaders);
		request->reqheaders = NULL;
	}
	free(request);
	request = NULL;
}
enum Http_Request_State http_request_state(struct Http_Request *request)
{
	return request->cur_state;
}
void http_request_add_header(struct Http_Request *request, const char *key, const char *value)
{
	request->reqheaders[request->reqheader_nums].key = key;
	request->reqheaders[request->reqheader_nums].value = value;
	request->reqheader_nums++;
}
char *http_request_get_header(struct Http_Request *request, const char *key)
{
	if (request == NULL)
	{
		return NULL;
	}
	for (int i = 0; i < request->reqheader_nums; i++)
	{
		if (strncasecmp(request->reqheaders[i].key, key, strlen(key)))
		{
			return request->reqheaders[i].value;
		}
	}
	return NULL;
}
bool parse_http_request_line(struct Http_Request *request, struct Buffer *readbuf)
{
	char *begin = readbuf->data + readbuf->readpos;
	char *end = buffer_find(begin, readbuf->capacity - readbuf->readpos, "\r\n", 2);
	int size = end - begin;
	if (size <= 0)
	{
		return false;
	}
	begin = parse_http_request_line_one(&request->method, begin, end, " ");
	if (begin == NULL)
	{
		return false;
	}
	begin = parse_http_request_line_one(&request->url, begin, end, " ");
	if (begin == NULL)
	{
		return false;
	}
	parse_http_request_line_one(&request->version, begin, end, "\r\n");
	readbuf->readpos += (size + 2);
	request->cur_state = PARSEREQHEADERS;
	return true;
}
char *parse_http_request_line_one(char **dest, char *start, char *end, char *sub)
{
	char *space = end;
	if (strcmp(sub, "\r\n") != '\0')
	{
		space = buffer_find(start, space - start, sub, strlen(sub));
		if (space == NULL)
		{
			return space;
		}
	}
	int size = space - start;
	(*dest) = (char *)malloc(size + 1);
	memcpy(*dest, start, size);
	(*dest)[size] = '\0';
	return space + strlen(sub);
}
bool parse_http_request_header(struct Http_Request *request, struct Buffer *readbuf)
{
	char *end = buffer_find(readbuf->data + readbuf->readpos, readbuf->capacity - readbuf->readpos, "\r\n", 2);
	if (end == NULL)
	{
		return false;
	}
	char *start = readbuf->data + readbuf->readpos;
	int line_size = end - start;
	char *middle = buffer_find(start, line_size, ": ", 2);
	if (middle == NULL)
	{
		// the request headers is phased
		readbuf->readpos += 2;
		if (strcasecmp(request->method, "post") == 0)
		{
			request->cur_state = PARSEREAQBODY;
			return true;
		}
		if (strcasecmp(request->method, "get") == 0)
		{
			request->cur_state = PARSEREQDONE;
			return true;
		}
		return false;
	}
	char *key = malloc(middle - start + 1);
	strncpy(key, start, middle - start);
	key[middle - start] = '\0';
	char *value = malloc(end - middle - 1);
	strncpy(value, middle + 2, end - middle - 2);
	value[end - middle - 2] = '\0';
	http_request_add_header(request, key, value);
	readbuf->readpos += (line_size + 2);
	return true;
}
bool parse_http_request_body(struct Http_Request *request, struct Buffer *readbuf)
{ // because of only single file could be updated only so parseonce
	char *begin = readbuf->data + readbuf->readpos;
	char *end = buffer_find(readbuf->data + readbuf->readpos, readbuf->capacity - readbuf->readpos, "\r\n", 2);
	if (end == NULL)
	{
		return false;
	}
	request->boundary = malloc(end - begin + 1);
	strncpy(request->boundary, begin, end - begin);
	request->boundary[end - begin] = '\0';
	readbuf->readpos += (end - begin + 2);
	begin = readbuf->data + readbuf->readpos;
	end = buffer_find(readbuf->data + readbuf->readpos, readbuf->capacity - readbuf->readpos, "filename=\"", 10);
	if (end == NULL)
	{
		return false;
	}
	readbuf->readpos += (end - begin + 10);
	begin = readbuf->data + readbuf->readpos;
	end = buffer_find(readbuf->data + readbuf->readpos, readbuf->capacity - readbuf->readpos, "\"", 1);
	if (end == NULL)
	{
		return false;
	}
	request->filename = malloc(end - begin + 1);
	strncpy(request->filename, begin, end - begin);
	request->filename[end - begin] = '\0';
	end = buffer_find(readbuf->data + readbuf->readpos, readbuf->capacity - readbuf->readpos, "\r\n\r\n", 4);
	if (end == NULL)
	{
		return false;
	}
	readbuf->readpos += (end - begin + 4);
	// 在此处将本次读取到的文件内容不写入，在回复函数中统一处理
	request->cur_state = PARSEREQDONE;
	return true;
}
bool write_posted_file(struct Http_Request *request, struct Buffer *readbuf, int socket)
{
	if (strcasecmp(request->method, "post") != 0)
	{
		return true;
	}
	if (strcasecmp(request->filename, "") == 0)
	{
		return false;
	}
	char path[1024];
	char name[256];
	char suffix[32];
	int has_suffix = 1;
	long long total_length = 0;
	char *dire_path = request->url + 1;
	int res = sscanf(request->filename, "%[^.].%s", name, suffix);
	if (res < 2)
		has_suffix = 0;

	sprintf(path, "%s/%s", dire_path, request->filename);
	struct stat st;
	if (stat(dire_path, &st) < 0)
	{
		mkdir(dire_path, 0755);
		perror("mkdir");
	}

	int record = 1;
	while (stat(path, &st) == 0)
	{
		if (has_suffix)
			sprintf(path, "%s/%s(%d).%s", dire_path, name, record++, suffix);
		else
			sprintf(path, "%s/%s(%d)", dire_path, name, record++);
	}

	int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (fd < 0)
	{
		perror("open");

		return false;
	}
	char *ptr = buffer_find_reverse(readbuf->data + readbuf->readpos, readbuf->writepos - readbuf->readpos, request->boundary, strlen(request->boundary), 256);
	while (ptr == NULL)
	{
		res = write(fd, readbuf->data + readbuf->readpos, readbuf->writepos - readbuf->readpos);
		readbuf->readpos += res;
		if (res < 0)
		{
			perror("write");
			close(fd);
			unlink(path);
			return false;
		}
		res = buffer_socket_read(readbuf, socket);
		if (res < 0)
		{
			perror("socket_read");
			close(fd);
			unlink(path);
			return false;
		}
		// ptr = buffer_find_reverse(readbuf->data + readbuf->readpos, res, request->boundary, strlen(request->boundary), 256) - strlen(request->boundary) + 1;
		ptr = buffer_find_reverse(readbuf->data + readbuf->readpos, res, request->boundary, strlen(request->boundary), 256);
	}
	res = write(fd, readbuf->data + readbuf->readpos, ptr - (readbuf->data + readbuf->readpos) - strlen(request->boundary) + 1);
	if (res < 0)
	{
		perror("write");
		close(fd);
		unlink(path);
		return false;
	}
	// off_t pos = lseek(fd, 0, SEEK_CUR);
	// if (pos >= 2)
	// {
	//     if (ftruncate(fd, pos - 2) < 0)
	//     {
	//         perror("ftruncate");
	//     }
	// }
	close(fd);
	return true;
}
bool parse_http_request(struct Http_Request *request, struct Buffer *readbuf, struct Http_Response *response, struct Buffer *sendbuf, int socket)
{
	bool flag = true;
	while (request->cur_state != PARSEREQDONE)
	{
		switch (request->cur_state)
		{
		case PARSEREQLINE:
			flag = parse_http_request_line(request, readbuf);
			break;
		case PARSEREQHEADERS:
			flag = parse_http_request_header(request, readbuf);
			break;
		case PARSEREAQBODY:
			flag = parse_http_request_body(request, readbuf);
			break;
		default:
			break;
		}
		if (!flag)
		{
			return flag;
		}
		if (request->cur_state == PARSEREQDONE)
		{ // 如果不是post请求，则下面的函数返回true
			if (!write_posted_file(request, readbuf, socket))
			{
				request->cur_state = PARSEREQLINE;
				return false;
			}
			if (!process_http_request(request, response))
			{
				request->cur_state = PARSEREQLINE;
				return false;
			}
			http_response_prepare(response, sendbuf, socket);
		}
	}
	request->cur_state = PARSEREQLINE;
	return flag;
}
int trans_utf8(char *out, char *in)
{
	int pre = 0;
	int i = 0;
	for (; in[i] != '\0'; i++)
	{
		out[i - pre] = in[i];
		if (in[i] == '%')
		{
			int dig1 = trans_hex_to_dec(in[i + 1]);
			if (in[i + 1] != '\0' && dig1 != -1)
			{
				int dig2 = trans_hex_to_dec(in[i + 2]);
				if (in[i + 2] != '\0' && dig2 != -1)
				{
					out[i - pre] = 16 * dig1 + dig2;
					pre += 2;
					i += 2;
				}
			}
		}
	}
	out[i - pre] = '\0';
	return 0;
}
char trans_hex_to_dec(char in)
{
	if (in >= '0' && in <= '9')
	{
		return in - '0';
	}
	if (in >= 'a' && in <= 'f')
	{
		return 10 + (in - 'a');
	}
	if (in >= 'A' && in <= 'F')
	{
		return 10 + (in - 'A');
	}
	return -1;
}
const char *get_file_type(const char *name)
{
	// 获取文件扩展名
	const char *dot = strrchr(name, '.');
	if (dot == NULL)
	{
		return "application/octet-stream; charset=UTF-8"; // 未知类型，默认二进制流
	}

	// 根据扩展名返回对应的 MIME 类型
	if (strcmp(dot, ".tif") == 0 || strcmp(dot, ".tiff") == 0)
		return "image/tiff; charset=UTF-8";
	if (strcmp(dot, ".001") == 0)
		return "application/x-001; charset=UTF-8";
	if (strcmp(dot, ".301") == 0)
		return "application/x-301; charset=UTF-8";
	if (strcmp(dot, ".323") == 0)
		return "text/h323; charset=UTF-8";
	if (strcmp(dot, ".906") == 0)
		return "application/x-906; charset=UTF-8";
	if (strcmp(dot, ".907") == 0)
		return "drawing/907; charset=UTF-8";
	if (strcmp(dot, ".a11") == 0)
		return "application/x-a11; charset=UTF-8";
	if (strcmp(dot, ".acp") == 0)
		return "audio/x-mei-aac; charset=UTF-8";
	if (strcmp(dot, ".ai") == 0)
		return "application/postscript; charset=UTF-8";
	if (strcmp(dot, ".aif") == 0 || strcmp(dot, ".aiff") == 0 || strcmp(dot, ".aifc") == 0)
		return "audio/aiff; charset=UTF-8";
	if (strcmp(dot, ".asa") == 0)
		return "text/asa; charset=UTF-8";
	if (strcmp(dot, ".asf") == 0 || strcmp(dot, ".asx") == 0)
		return "video/x-ms-asf; charset=UTF-8";
	if (strcmp(dot, ".asp") == 0)
		return "text/asp; charset=UTF-8";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic; charset=UTF-8";
	if (strcmp(dot, ".avi") == 0)
		return "video/avi; charset=UTF-8";
	if (strcmp(dot, ".awf") == 0)
		return "application/vnd.adobe.workflow; charset=UTF-8";
	if (strcmp(dot, ".bmp") == 0)
		return "application/x-bmp; charset=UTF-8";
	if (strcmp(dot, ".bot") == 0)
		return "application/x-bot; charset=UTF-8";
	if (strcmp(dot, ".c4t") == 0)
		return "application/x-c4t; charset=UTF-8";
	if (strcmp(dot, ".c90") == 0)
		return "application/x-c90; charset=UTF-8";
	if (strcmp(dot, ".cal") == 0)
		return "application/x-cals; charset=UTF-8";
	if (strcmp(dot, ".cat") == 0)
		return "application/vnd.ms-pki.seccat; charset=UTF-8";
	if (strcmp(dot, ".cdf") == 0)
		return "application/x-netcdf; charset=UTF-8";
	if (strcmp(dot, ".cdr") == 0)
		return "application/x-cdr; charset=UTF-8";
	if (strcmp(dot, ".cel") == 0)
		return "application/x-cel; charset=UTF-8";
	if (strcmp(dot, ".cer") == 0)
		return "application/x-x509-ca-cert; charset=UTF-8";
	if (strcmp(dot, ".css") == 0)
		return "text/css; charset=UTF-8";
	if (strcmp(dot, ".dll") == 0)
		return "application/x-msdownload; charset=UTF-8";
	if (strcmp(dot, ".doc") == 0 || strcmp(dot, ".dot") == 0)
		return "application/msword; charset=UTF-8";
	if (strcmp(dot, ".dtd") == 0)
		return "text/xml; charset=UTF-8";
	if (strcmp(dot, ".exe") == 0)
		return "application/x-msdownload; charset=UTF-8";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif; charset=UTF-8";
	if (strcmp(dot, ".htm") == 0 || strcmp(dot, ".html") == 0)
		return "text/html; charset=UTF-8";
	if (strcmp(dot, ".ico") == 0)
		return "image/x-icon; charset=UTF-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0 || strcmp(dot, ".jfif") == 0)
		return "image/jpeg; charset=UTF-8";
	if (strcmp(dot, ".js") == 0)
		return "application/x-javascript; charset=UTF-8";
	if (strcmp(dot, ".json") == 0)
		return "application/json; charset=UTF-8";
	if (strcmp(dot, ".png") == 0)
		return "image/png; charset=UTF-8";
	if (strcmp(dot, ".pdf") == 0)
		return "application/pdf; charset=UTF-8";
	if (strcmp(dot, ".txt") == 0)
		return "text/plain; charset=UTF-8";
	if (strcmp(dot, ".xml") == 0)
		return "text/xml; charset=UTF-8";
	if (strcmp(dot, ".zip") == 0)
		return "application/zip; charset=UTF-8";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mp3; charset=UTF-8";
	if (strcmp(dot, ".mp4") == 0)
		return "video/mpeg4; charset=UTF-8";

	return "application/octet-stream; charset=UTF-8"; // 未知类型
}

bool process_http_request(struct Http_Request *request, struct Http_Response *response)
{
	if (strcasecmp(request->method, "post") == 0)
	{
		char *file_name = "post.html";
		strcpy(response->file_name, file_name);
		response->status_code = OK;
		strcpy(response->status_message, "Post OK");
		http_response_add_headers(response, "Content-type", get_file_type(file_name));
		response->send_data_func = send_file;
		return true;
	}
	if (strcasecmp(request->method, "get") != 0)
	{
		perror("unsolveable request");
		return false;
	}
	char *file = NULL;
	if (strcmp(request->url, "/") == 0)
	{
		file = "./";
	}
	else
	{
		file = request->url + 1;
		trans_utf8(file, file);
	}
	// printf("path:%s file:%s\n", path, file);
	struct stat st;
	int res = stat(file, &st);
	if (res == -1)
	{
		char *file_name = "404.html";
		strcpy(response->file_name, file_name);
		response->status_code = NO_FOUND;
		strcpy(response->status_message, "Not Found");
		http_response_add_headers(response, "Content-type", get_file_type(file_name));
		response->send_data_func = send_file;
		return false;
	}
	strcpy(response->file_name, file);
	response->status_code = OK;
	strcpy(response->status_message, "OK");
	if (S_ISDIR(st.st_mode))
	{
		http_response_add_headers(response, "Content-type", get_file_type(".html"));
		response->send_data_func = send_dir;
	}
	else
	{
		char temp[32] = {0};
		sprintf(temp, "%ld", st.st_size);
		http_response_add_headers(response, "Content-type", get_file_type(file));
		http_response_add_headers(response, "Content-length", temp);
		response->send_data_func = send_file;
	}
	return true;
}
void send_file(const char *file_name, struct Buffer *sendbuf, int communicate_socket)
{
	printf("%s", file_name);
	int fd = open(file_name, O_RDONLY);
	assert(fd > 0);
	int length = 0;
	while (1)
	{
		char buf[10240];
		length = read(fd, buf, sizeof(buf));
		if (length > 0)
		{
			buffer_append_data(sendbuf, buf, length);
#ifndef WHOLE_SEND
			buffer_send_data(sendbuf, communicate_socket);
#endif
		}
		if (length == 0)
		{
			break;
		}
		if (length < 0)
		{
			perror("failed to read");
			break;
		}
	}
	// int size = lseek(fd, 0, SEEK_END);
	// sendfile(communicate_socket, fd, NULL, size);
	// printf("successed to send file %s to client %d size %d\n", file_name, communicate_socket, length);
	close(fd);
}
void send_dir(const char *dir_name, struct Buffer *sendbuf, int communicate_socket)
{
	char buf[10240] = {0};
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dir_name);
	sprintf(buf + strlen(buf), "<form action=\"/upload\" method=\"post\" enctype=\"multipart/form-data\">");
	sprintf(buf + strlen(buf), "<input type=\"file\" name=\"file\"/><br/><br/>");
	sprintf(buf + strlen(buf), "<button type=\"submit\">上传文件到 /upload 目录</button></form>");
	struct dirent **name_list;
	int num = scandir(dir_name, &name_list, NULL, NULL);
	for (int i = 0; i < num; i++)
	{
		char *name = name_list[i]->d_name;
		// if (name[0] == '.')
		// {
		// 	free(name_list[i]);
		// 	name_list[i] = NULL;
		// 	continue;
		// }
		char sub_path[1024] = {0};
		sprintf(sub_path, "%s/%s", dir_name, name);
		struct stat st;
		stat(sub_path, &st);
		if (S_ISDIR(st.st_mode))
		{
			sprintf(buf + strlen(buf),
					"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
					name, name, st.st_size);
		}
		else
		{
			sprintf(buf + strlen(buf),
					"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
					name, name, st.st_size);
		}
		buffer_append_string(sendbuf, buf);
#ifndef WHOLE_SEND
		buffer_send_data(sendbuf, communicate_socket);
#endif
		memset(buf, 0, sizeof(buf));
		free(name_list[i]);
		name_list[i] = NULL;
	}
	sprintf(buf, "</table></body></html>");
	buffer_append_string(sendbuf, buf);
#ifndef WHOLE_SEND
	buffer_send_data(sendbuf, communicate_socket);
#endif
	free(name_list);
	name_list = NULL;
	// printf("successed to send dir %s to client %d\n", dir_name, communicate_socket);
}