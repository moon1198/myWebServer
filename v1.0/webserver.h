#ifndef __WEBSERVER_H_
#define __WEBSERVER_H_

#include "threadpool/threadpool.h"
#include "lock/locker.h"
#include "timer/lst_timer.h"
#include "log/log.h"
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
	//void sql_init(std::string dbname, std::string url, int port, std::string user, std::string password, int max_conn, int close_log);
	void sql_init();
	void timer_lst_init();
	void event_listen();
	void event_loop();

	static void sig_handler(int);
	void deal_sig(bool*, bool*);
	void timer_handler();
	void new_timer(int sockfd, struct sockaddr_in* addr);
	static void cb_func(client_data* data);
	void prolong_timer(Timer*);
	void timer_deal_err(Timer*);
	void log_write();

public:
	//Config m_config;
	int m_thread_num;
	Threadpool<Http_client> *m_threadpool;

	//连接相关
	int m_port;					//端口
	int m_lisfd;
	int m_pipefd[2];
	static int* m_pipeio;
	int m_epollfd;				//
	Http_client *users;			//记录连接用户, 长度可设置为最大文件描述符，并以此做索引；
	//定时器里也需要连接数据
	Timer_lst* m_timer_lst;
	client_data* m_timer_data;

	//log
	int m_close_log;	//close or not
	int m_async;		//async or not

private:
	std::string m_dbname;
	std::string m_url;
	int m_sqlport;
	std::string m_user;
	std::string m_password;
	int m_max_sqlconn;

};

//int WebServer::m_pipeout = -1;



#endif
