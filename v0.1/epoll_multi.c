#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#include <assert.h>

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define P_NUMS 2
#define MAX_EVENTS 10

void * handler_accept(int epollfd, int sfd) {

	struct sockaddr_in peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);

	//printf("sfd = %d\n", sfd);
	int cfd = accept(sfd, (struct sockaddr *) &peer_addr, &peer_addr_len);
	//printf("cfd = %d\n", cfd);
	if (cfd == -1) 
		return NULL;
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(peer_addr.sin_addr), ip, INET_ADDRSTRLEN);

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = cfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, cfd, &ev) == -1) 
		handle_error("epoll_ctl");

	printf("tid : <%lu> fd : <%d> : Contecting to %s : %hu.\n", pthread_self(), cfd, ip, ntohs(peer_addr.sin_port));
}

void * handler_read(void *arg) {
	void *(*argv)[2]  = (void *(*)[2]) arg;
	int cfd = *((int *) (*argv)[0]);
	int epollfd = *((int *) (*argv)[1]);

	//printf("cfd = %d ; epollfd = %d\n", cfd, epollfd);
	char buf[1024];
	int recv_len = recv(cfd, buf, sizeof(buf), 0);
	buf[recv_len] = '\0';
	printf("%s", buf);
	//printf("recv = %d\n", recv_len);
	//int send_len = send(cfd, buf, recv_len, 0);
		

	printf("tid : <%lu> fd : <%d> : Close connection.\n", pthread_self(), cfd);
	//if (epoll_ctl(epollfd, EPOLL_CTL_DEL, cfd, NULL) == -1) 
	//	handle_error("epoll_ctl");
	close(cfd);
	//free((*argv)[0]);
}

int main (int argc, char*argv[]) {

	int port = 9006;
	if (argc == 2) {
		port = atoi(argv[1]);
	}
	//init my_addr
	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;	//ipv4
	my_addr.sin_port = htons(port);	//port
	inet_aton("127.0.0.1", &my_addr.sin_addr);	//address


	int sfd = socket(AF_INET, SOCK_STREAM, 0);		//
	assert(sfd > 0);
	int bind_ret = bind(sfd, (const struct sockaddr *) &my_addr, sizeof(my_addr));
	assert(bind_ret != -1);
	listen(sfd, 4);

	struct sockaddr_in peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);

	int epollfd = epoll_create1(0);
	if (epollfd == -1) 
		handle_error("epoll_create1");
	struct epoll_event ev, events[MAX_EVENTS];
	//init event
	ev.events = EPOLLIN;
	ev.data.fd = sfd;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &ev) == -1) 
		handle_error("epoll_ctl");


	while (1) {
		int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1) 
			handle_error("epoll_wait");
		for (int i = 0; i < nfds; ++ i) {
			if (events[i].data.fd == sfd) {
				printf("accept\n");
				handler_accept(epollfd, sfd);
			}else {
				printf("read fd = %d\n", events[i].data.fd);
				void *argv[2];
				int *rfd = (int *) malloc(sizeof (int));
				*rfd = events[i].data.fd;
				argv[0] = rfd;
				argv[1] = &epollfd;
				pthread_t p2;
				if (epoll_ctl(epollfd, EPOLL_CTL_DEL, *rfd, NULL) == -1) 
					handle_error("epoll_ctl");
				int pret = pthread_create(&p2, NULL, handler_read, argv);
				if (pret != 0)
					handle_error("pthread creat");
				printf("Creat a read pthread <%ld>.\n", p2);
				//sleep(1);

			}
		}

	}

	printf("All have finished\n");

	close(sfd);
	return 0;
}
