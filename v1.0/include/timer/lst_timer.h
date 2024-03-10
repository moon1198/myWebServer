#ifndef __LST_TIMER_H_
#define __LST_TIMER_H_

#include <time.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>  // 提供网络相关的函数和数据结构
#include <arpa/inet.h>   // 提供 IP 地址转换函数


class Timer_lst;
class Timer;

class client_data
{
public:
	struct sockaddr_in addr;
	int m_sockfd;
	int m_epollfd;
	Timer* timer;
};

class Timer
{
public:
	Timer() : prev(NULL), next(NULL) {};
	~Timer() {};

	//void init();
	//
	//close connection 
	void (* cb_func)(client_data*); 
	
public:
	client_data* data;
	time_t expire;
	Timer* prev;
	Timer* next;
};

//void Timer::init(, ) {
//	expire = ;
//	data = new client_data(, );
//	cb_func = ;
//}

//Timer::~Timer() {
//	delete data;
//}

class Timer_lst 
{
public:
	Timer_lst();
	~Timer_lst();

	void add_timer(Timer* );
	void del_timer(Timer* );
	void adjust_timer(Timer* );
	void tick();


private:
	Timer* head;
	Timer* tail;

};


#endif

