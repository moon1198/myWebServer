#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <assert.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define P_NUMS 2

void * handler(void *arg) {
	int sfd = *((int *) arg);

	struct sockaddr_in peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);
	while (1) {
	int cfd = accept(sfd, (struct sockaddr *) &peer_addr, &peer_addr_len);
	if (cfd == -1) 
		handle_error("accept");
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(peer_addr.sin_addr), ip, INET_ADDRSTRLEN);
	printf("tid : <%lu> fd : <%d> : Contecting to %s : %hu.\n", pthread_self(), cfd, ip, ntohs(peer_addr.sin_port));

	while (1) {
		char buf[1024];
		int recv_len = recv(cfd, buf, sizeof(buf), 0);
		buf[recv_len] = '\0';
		printf("%s", buf);
		int send_len = send(cfd, buf, recv_len, 0);
		
		if (strncmp(buf, "quit", recv_len - 1) == 0) {
			break;
		}
	}
	printf("tid : <%lu> fd : <%d> : Close connection to %s : %hu.\n", pthread_self(), cfd, ip, ntohs(peer_addr.sin_port));
	close(cfd);

	}

	return NULL;
}

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

	int backlog = P_NUMS;
	if (listen(sfd, backlog) == -1)
		handle_error("listen");
	printf("Listening for incoming connections...\n");

	//peer_addr used to store client address
	//waitting after call accept until one request comes

	pthread_t p[P_NUMS] = {};
	for (int i = 0; i < P_NUMS; ++ i) {
		p[i] = 0;
	}
	for (int i = 0; i < P_NUMS; ++ i) {

		int pret = pthread_create(&p[i], NULL, handler, &sfd);
		if (pret != 0) 
			handle_error("phread creat");

		printf("Creat a pthread <%ld>.\n", p[i]);

	}
	for (int i = 0; i < P_NUMS; ++ i) {
		if (p[i] == 0)
			continue;
		pthread_join(p[i], NULL);
	}
	printf("All have finished\n");

	close(sfd);
	return 0;
}
