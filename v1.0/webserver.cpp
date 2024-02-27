#include "webserver.h"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>

void setnonblock(int fd);
void addfd(int epollfd, int fd, bool one_shot);

WebServer::WebServer() : m_threadpool(NULL), users(NULL) {

}

WebServer::~WebServer() {
	if (m_threadpool != NULL) {
		delete m_threadpool;
	}
	if (users != NULL) {
		delete users;
	}
}

void WebServer::init(int port = 9006, int thread_num = 8) {
	m_port = port;
	m_thread_num = thread_num;
}

void WebServer::threadpool_init() {
	m_threadpool = new Threadpool<Http_client>(m_thread_num);
	assert(m_threadpool != NULL);
	users = new Http_client[MAX_FD];
}

void WebServer::event_listen() {
	int ret = 0;
	m_lisfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(m_lisfd > 0);

	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(m_port);
	inet_aton("127.0.0.1", &my_addr.sin_addr);
	//sockaddr_t addr_len= sizeof();

	ret = bind(m_lisfd, (const struct sockaddr *) &my_addr, sizeof(my_addr)); 
	assert(ret != -1);

	ret = listen(m_lisfd, 10);
	assert(ret != -1);

	m_epollfd = epoll_create1(0);
	assert(m_epollfd != -1);
	Http_client::m_epollfd = m_epollfd;
	addfd(m_epollfd, m_lisfd, false);
}

void WebServer::event_loop() {

	bool stop = false;
	epoll_event events[MAX_EVENT_NUM];

	while (!stop) {
		std::cout << "listen..." << std::endl;
		int event_num = epoll_wait(m_epollfd, events, MAX_EVENT_NUM, -1);
		assert(event_num >= 0);

		for (int i = 0; i < event_num; ++ i) {
			int sockfd = events[i].data.fd;
			std::cout << "sockfd = " << sockfd << std::endl;
			std::cout << "events[i].events = " << events[i].events << std::endl;

			//new connection request
			if (sockfd == m_lisfd) {
				
				struct sockaddr peer_addr;
				socklen_t peer_addr_len = sizeof(peer_addr);
				int peer_fd = accept(m_lisfd, &peer_addr, &peer_addr_len);
				if (Http_client::m_user_num >= MAX_FD) {
					std::cout << "fd connection too busy!" << std::endl;
					continue;
				}

				users[peer_fd].new_user(peer_fd, (struct sockaddr_in*) &peer_addr);
				continue;
			}

			if (events[i].events & EPOLLIN) {
				if (users[sockfd].Read()) {
					m_threadpool->push(&users[sockfd]);
				}
				continue;
			}

			if (events[i].events & EPOLLOUT) {
				std::cout << "write....." << std::endl;

				users[sockfd].Write();
				continue;
			}
		}

	}
}

void setnonblock(int fd) {
	int old_stat = fcntl(fd, F_GETFL);
	int new_stat = old_stat | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_stat);
}

void addfd(int epollfd, int fd, bool one_shot) {
	epoll_event ev;
	ev.data.fd = fd;
	//edge triggle and half-close
	ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	//触发一次事件通知后，该描述符将disable，无法收到事件通知，需要再一次修改ev.events
	//可避免竞态
	if (one_shot) 
		ev.events |= EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
	setnonblock(fd);
}

void modfd(int epollfd, int fd, int mode, bool one_shot) {
	epoll_event ev;
	ev.data.fd = fd;
	//edge triggle and half-close
	ev.events = EPOLLET | EPOLLRDHUP | mode;
	//触发一次事件通知后，该描述符将disable，无法收到事件通知，需要再一次修改ev.events
	//可避免竞态
	if (one_shot) 
		ev.events |= EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}
