#include <iostream>
#include <string>


#include <cassert>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "processpool.h"


#define handle_error(msg) \
	do {perror(msg); exit(EXIT_FAILURE); } while (0)

#define MAX_LISTEN_QUEUE 10
#define MAX_PROCESS_NUM 4
int main(int argc, char *argv[]) {
	int port = 9006;
	if (argc == 2) {
		port = std::stoi(argv[1], 0, 10);
	}
	//std::cout << port << std::endl;

	struct sockaddr_in my_addr, peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);

	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	inet_aton("127.0.0.1", &my_addr.sin_addr);

	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) 
		handle_error("socket");
	int bind_ret = bind(sfd, (const struct sockaddr *) &my_addr, sizeof(my_addr));
	assert(bind_ret != -1);
	assert(listen(sfd, MAX_LISTEN_QUEUE) != -1);
	std::cout << "Listening ......" << std::endl;

	processpool<int> *pool = processpool<int>::create(sfd, MAX_PROCESS_NUM); //listen fd; the number of processs;
	pool->run();
	//processpool<echo> *pool = processpool<echo>::create(sfd, 8); //listen fd; the number of processs;
	//pool->run;


	//int cfd = accept(sfd, (struct sockaddr *) &peer_addr, &peer_addr_len);
	//assert(cfd > 0);
	//std::cout << "Request to link" << std::endl;

	//close(cfd);

	close(sfd);

	return 0;
}
