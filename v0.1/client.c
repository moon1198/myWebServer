#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[]) {
	int port = 9006;
	if (argc == 2) {
		port = atoi(argv[1]);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;   //ipv4 
	server_addr.sin_port = htons(port); //port
	inet_aton("127.0.0.1", &server_addr.sin_addr);  //address

	while (1) {
		static int count = 0;
		int cfd = socket(AF_INET, SOCK_STREAM, 0);
		assert(cfd > 0);
		if (connect(cfd, (const struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) 
		handle_error("connect");

		char buf[1024];
		++ count;
		sprintf(buf, "hello world %2d.\n", count % 100);

		send(cfd, buf, 16, 0);
		close(cfd);
		for (int i = 0; i < 100000000; ++ i);
	}
	



	return 0;
}
