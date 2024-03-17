#ifndef __SQLPOLL_H_
#define __SQLPOLL_H_

#include <list>
#include <string>
#include <mysql/mysql.h>

#include "lock/locker.h"
#include "log/log.h"

class Sqlpool {

public:
	static Sqlpool* get_instance();

	void init(std::string dbname, std::string url, int port, std::string user,
		   	std::string password, int max_conn, int close_log);

	MYSQL* get_conn();
	bool release_conn(MYSQL* );


private:
	Sqlpool();
	~Sqlpool();

	void destory_conns();

	std::list<MYSQL*> conn_lst;

	std::string m_url;
	unsigned m_port;
	std::string m_user;
	std::string m_password;
	std::string m_dbname;
	int m_close_log;
	
	Sem sem;
	Locker locker;

	int m_curconn;
	int m_freeconn;
	int m_maxconn;

};

class ConnectionRALL{
public:
	ConnectionRALL(MYSQL**, Sqlpool*);
	~ConnectionRALL();
private:
	MYSQL* m_conn;
	Sqlpool* m_pool;
};


#endif
