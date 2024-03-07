#ifndef __WEBSERVER_H_
#define __WEBSERVER_H_

#include "threadpool/threadpool.h"
#include "lock/locker.h"
#include "timer/lst_timer.h"
#include "http_client.h"


#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_FD 65535				//最大文件描述符
#define MAX_EVENT_NUM  10000		//epoll最大队列长度
#define TIMESLOT  5					//tick时间间隔


class WebServer 
{
public:
	WebServer();
	~WebServer();

	//attribute setting 
	void init(int, int);
	void threadpool_init();
	void timer_lst_init();
	void event_listen();
	void event_loop();

	static void sig_handler(int);
	void deal_sig(bool*, bool*);
	void timer_handler();
	void new_timer(int sockfd, struct sockaddr_in* addr);
	static void cb_func(client_data* data);


public:
	//Config m_config;
	int m_thread_num;
	Threadpool<Http_client> *m_threadpool;

	//连接相关
	int m_port;					//端口
	int m_lisfd;
	int m_pipefd[2];
	static int m_pipeout;
	int m_epollfd;				//
	Http_client *users;			//记录连接用户, 长度可设置为最大文件描述符，并以此做索引；
	//定时器里也需要连接数据
	Timer_lst* m_timer_lst;

private:

};

//int WebServer::m_pipeout = -1;



#endif
