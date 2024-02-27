#include "http_client.h"
int Http_client::m_epollfd = -1;
int Http_client::m_user_num = 0;

#include <sys/epoll.h>
#include <iostream>

void modfd(int epollfd, int m_fd, int ev, bool one_shot);
void setnonblock(int fd);
void addfd(int epollfd, int fd, bool one_shot);

void Http_client::init(int fd, const struct sockaddr_in *peer_addr) {
	m_fd = fd;
	m_peer_addr = peer_addr;
	m_read_idx = 0;
	m_write_idx = 0;
}

void Http_client::new_user(int fd, const struct sockaddr_in *peer_addr) {
	init(fd, peer_addr);
	addfd(m_epollfd, fd, true);
}

void Http_client::run() {
	strncpy(m_writebuf, m_readbuf, m_read_idx);
	//epoll_event ev;
	//ev.data.fd = m_fd;
	//ev.events = EPOLLET | EPOLLONESHOT | EPOLLRDHUP | EPOLLOUT;
	//epoll_ctl(m_epollfd, EPOLL_CTL_MOD, m_fd, &ev); 
	//std::cout << "ev.events = " << ev.events << std::endl;
	modfd(m_epollfd, m_fd, EPOLLOUT, true);
}


bool Http_client::Read() {
	while(1) {
		int len = read(m_fd, m_readbuf + m_read_idx, READ_BUF_LEN - m_read_idx);
		if (len == -1) {
			//all have been read
			if (errno & (EAGAIN | EWOULDBLOCK)) {
				break;
			}
			//error disappear
			return false;
			break;
			//disconnect
		} else if(len == 0){
			return false;
		}
		m_read_idx += len;
	}

	m_readbuf[m_read_idx] = '\0';

	return true;
}

bool Http_client::Write() {
	while(1) {
		int len = write(m_fd, m_writebuf + m_write_idx, READ_BUF_LEN);
		if (len == -1) {
			//all have been write
			if (errno & (EAGAIN | EWOULDBLOCK)) {
				break;
			}
			//error disappear
			return false;
			break;
		} else {
			m_read_idx += len;
		}
	}
	return true;
}
