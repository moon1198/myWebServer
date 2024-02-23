#ifndef __WEBSERVER_H_
#define __WEBSERVER_H_

#include "threadpool/threadpool.h"
#include "lock/locker.h"

#define MAX_FD 65535				//最大文件描述符
#define MAX_EVENT_NUM  10000		//epoll最大队列长度


class Webserver 
{
public:
	Webserver();
	~Webserver();

	//attribute setting 
	void init();
	void threadpool_init();
	void event_listen();

public:
	Config m_config;
	int m_thread_num;
	Threadpool<http_client> *m_threadpool;

	//连接相关
	int m_port;					//端口
	int m_epollfd;				//
	http_client *users;			//记录连接用户, 长度可设置为最大文件描述符，并以此做索引；
	



private:

};






#endif
