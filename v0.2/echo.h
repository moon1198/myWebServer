#ifndef __ECHO_H_
#define __ECHO_H_

class echo{
public: 
	echo() = default;
	echo(int epollfd, int fd);
	void run ();
	void set (int epollfd, int fd);

private:
	int m_epollfd;
	int m_fd;
	char buf[256];

};

echo::echo(int epollfd, int fd) : m_epollfd(epollfd), m_fd(fd){

}

void echo::set (int epollfd, int fd) {
	m_epollfd = epollfd;
	m_fd = fd;
}

void echo::run () {
	while (1) {                                                                  
		int ret_len = recv(m_fd, buf, 255, 0);                      
		if (ret_len == 0) {                                                      
			//unlink                                                             
			delfd(m_epollfd, m_fd);                                   
			break;                                                               
		}else if (ret_len > 0) {                                                
			buf[ret_len] = '\0';                                                 
			printf("%s", buf);                                                   
		}else if (ret_len == -1) {                                              
			if (errno == EAGAIN || errno == EWOULDBLOCK) break;                  
		}
	}

}

static void delfd(int epollfd, int fd) {                                                     
	assert(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) != -1);                               
	close(fd); 
}



#endif
