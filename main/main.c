#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include "Tcp_Server.h"
int main(int argc, char *argv[])
{
	unsigned short port = 11111;
	char *source_dir = "/home";
	if (argc >= 2)
	{
		port = atoi(argv[1]);
	}
	if (argc >= 3)
	{
		source_dir = argv[2];
	}
	chdir(source_dir);
	struct Tcp_Server *server = tcp_server_init(port, 4);
	tcp_server_run(server);
	return 0;
}
