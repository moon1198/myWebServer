#ifndef __PROCESSPOOL_H_
#define __PROCESSPOOL_H_

#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_EVENTS 10
#define BASE_ECHO_FD 5
#define MAX_ECHO_NUM BASE_ECHO_FD + 10

static void delfd(int epollfd, int fd);
static void addfd(int epollfd, int fd);
static int set_non_blocking(int fd);

class process
{
public:
	int pid;
	int pro_pipe[2];
	process() : pid(-1), pro_pipe{0, 0} {}
};

template <typename T>
class processpool
{
public:

	static processpool<T> *create(int sfd, int num_process = 8);


	void run ();
private:
	//构造函数设置为私有函数，用于构造单例
	processpool(int sfd, int num_process = 8);
	~processpool();

	static processpool<T> *instance;
	int m_sfd;	//listen fd
	int m_num_process;	//the number of process
	int idx;			//the id of process  main_process : -1; sub_process : 0-n;
	process* sub_processes;


	void run_parent();
	void run_child();

};


template <typename T>
processpool<T>* processpool<T>::instance = nullptr;

template <typename T>
processpool<T>* processpool<T>::create(int sfd, int num_process) {
	if (instance == nullptr) {
		instance = new processpool<T> (sfd, num_process);
	}
	return instance;
}

template <typename T>
processpool<T>::processpool(int sfd, int num_process) : 
	m_sfd(sfd), m_num_process(num_process), idx(-1), sub_processes(nullptr) {

	sub_processes = new process[m_num_process];

		//init sub_processes
	for (int i = 0; i < m_num_process; ++ i) {
		assert(pipe(sub_processes[i].pro_pipe) != -1);

		int cur_pid = fork();
		if (cur_pid == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		} else if (cur_pid == 0) {
			//child 
			close(sub_processes[i].pro_pipe[1]);
			idx = i;
			break;

		} else {
			//parent
			sub_processes[i].pid = cur_pid;
			close(sub_processes[i].pro_pipe[0]);
			continue;
		}

	}	

}

template <typename T>
processpool<T>::~processpool() {
	delete instance;
	delete [] sub_processes;
}

template <typename T>
void processpool<T>::run() {

	if (idx == -1) {
		//parent 
		//for (int i = 0; i < m_num_process; ++ i) {
		//	int wr_len = write(sub_processes[i].pro_pipe[1], "hello world\n", 12);
		//}
		run_parent();

	} else {
		//child
		run_child();
		//char buf[1024];

		//int len_rec = read(sub_processes[idx].pro_pipe[0], buf, 12);
		//buf[len_rec] = '\0';
		std::cout << "< " << idx << " >" << std::endl;
	}
}

template <typename T>
void processpool<T>::run_parent() {
	int epollfd = epoll_create1(0);
	assert(epollfd != -1);
	struct epoll_event events[MAX_EVENTS];
	addfd(epollfd, m_sfd);
	unsigned int count = 0;
	//init event
	do {
		std::cout << "flag" << std::endl;
		int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		assert(nfds != -1);
		for (int i = 0; i < nfds; ++ i) {
			//awaken child process
			write(sub_processes[count % m_num_process].pro_pipe[1], "accept", 6);
			++ count;
		}
	} while(1);

}

template <typename T>
void processpool<T>::run_child() {


	T echo_list[MAX_ECHO_NUM];
	

	int epollfd = epoll_create1(0);
	assert(epollfd != -1);
	struct epoll_event events[MAX_EVENTS];
	int in_fd = sub_processes[idx].pro_pipe[0];

	addfd(epollfd, in_fd);
	int count = 0;
	char buf[256];
	//init event
	do {
		++ count;
		std::cout << idx << " : flag child" << count % 10 << std::endl;
		int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		assert(nfds != -1);
		for (int i = 0; i < nfds; ++ i) {
			if (events[i].data.fd == in_fd) {
				//clear buffer of pipe
				read(in_fd, buf, 6);


				//accept link request
				struct sockaddr_in peer_addr;
				socklen_t peer_addr_len = sizeof(peer_addr);
				int cfd = accept(m_sfd, (struct sockaddr*) &peer_addr, &peer_addr_len);
				//std::cout << cfd << std::endl;
				addfd(epollfd, cfd);
				echo_list[cfd].set(epollfd, cfd);
				//accept();

				
			}else {
				//std::cout << "idx = " << idx << std::endl;
				echo_list[events[i].data.fd].run();

				//while (1) {
				//	int ret_len = recv(events[i].data.fd, buf, 255, 0);
				//	if (ret_len == 0) {
				//		//unlink
				//		delfd(epollfd, events[i].data.fd);
				//		break;
				//	} else if (ret_len > 0) {
				//		buf[ret_len] = '\0';
				//		printf("%s", buf);
				//	} else if (ret_len == -1) {
				//		if (errno == EAGAIN || errno == EWOULDBLOCK) break;
				//	}
				//}
			}
		}
	} while(1);

}

//static void delfd(int epollfd, int fd) {
//	assert(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) != -1);
//	close(fd);
//}

static void addfd(int epollfd, int fd) {
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = fd;
	assert(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) != -1);
	set_non_blocking(fd);
}

static int set_non_blocking(int fd) {
	int old_state = fcntl(fd, F_GETFL);
	int new_state = old_state | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_state);
	return old_state;
}

#endif
