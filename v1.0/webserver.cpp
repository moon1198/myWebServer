#include "webserver.h"

#include <unistd.h>
#include <fcntl.h>
#include <socket.h>
#include <sys/epoll.h>

Webserver::Webserver() : m_threadpool(NULL), users(NULL) {

}

Webserver::~Webserver() {
	if (m_threadpool != NULL) {
		delete m_threadpool;
	}
	if (users != NULL) {
		delete users;
	}
}

void Webserver::init(int port = 9006, int thread_num = 8) {
	m_port = port;
	m_thread_num = thread_num;
}

void Webserver::threadpool_init() {
	m_threadpool = new Threadpool<http_client>(m_thread_num);
	assert(m_threadpool != NULL);
	users = new http_client[MAX_FD];
}

void Webserver::event_listen() {
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
	addfd(m_epollfd, m_lisfd, false);
}

void Webserver::event_loop() {

	bool stop = false;
	epoll_event events[MAX_EVENT_NUM];

	while (!stop) {
		int event_num = epoll_wait(m_epollfd, events, MAX_EVENT_NUM, -1);
		assert(event_num >= 0);

		for (int i = 0; i < event_num; ++ i) {
			int sockfd = events[i].data.fd;

			//new connection request
			if (sockfd == m_lisfd) {
				struct sockaddr peer_addr;
				socklen_t peer_addr_len = sizeof(peer_addr);
				int peer_fd = accept(m_lisfd, &peer_addr, &peer_addr_len);
				addfd(m_epollfd, peer_fd, false);

				users[peer_fd].init();
				continue;
			}

			if (event[i].events | EPOLLIN) {
				users[sockfd].read();
				continue;
			}

			if (event[i].events | EPOLLOUT) {
				users[sockfd].write();
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
	epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &ev);
	setnonblock(fd);
}
