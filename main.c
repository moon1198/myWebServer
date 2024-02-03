#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)


int main () {

	//init my_addr
	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;	//ipv4
	my_addr.sin_port = htons(9006);	//port
	inet_aton("127.0.0.1", &my_addr.sin_addr);	//address


	int sfd = socket(AF_INET, SOCK_STREAM, 0);		//
	assert(sfd > 0);

	if (bind(sfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) == -1)
		handle_error("bind");

	int backlog = 4;
	if (listen(sfd, backlog) == -1)
		handle_error("listen");
	printf("Listening for incoming connections...\n");

	//peer_addr used to store client address
	//waitting after call accept until one request comes
	struct sockaddr_in peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);
	int cfd = accept(sfd, (struct sockaddr *) &peer_addr, &peer_addr_len);
	if (cfd == -1) 
		handle_error("accept");

	char buf[1024];
	while (1) {
		int recv_len = recv(cfd, buf, sizeof(buf), 0);
		buf[recv_len] = '\0';
		printf("%s", buf);
		int send_len = send(cfd, buf, recv_len, 0);
		if (strncmp(buf, "quit", recv_len - 1) == 0) {
			break;
		}
	}

	close(sfd);
	close(cfd);

	return 0;
}
