#include "sqlpool/sqlpool.h"

#include <string>
using namespace std;

Sqlpool::Sqlpool() {
	m_curconn = 0;
	m_freeconn = 0;
}

Sqlpool::~Sqlpool() {
	destory_conns();
}

Sqlpool* Sqlpool::get_instance() {
	static Sqlpool instance;
	return &instance;
}

void Sqlpool::init(string dbname, string url, int port, string user, string passward,
					int max_conn, int close_log) {
	m_dbname = dbname;
	m_url = url;
	m_port = port;
	m_user = user;
	m_passward = passward;
	m_close_log = close_log;

	for (int i = 0; i < max_conn; ++ i) {
		MYSQL* tmp_conn = NULL;

		tmp_conn = mysql_init(tmp_conn);
		if (tmp_conn == NULL) {
			LOG_ERROR("MYSQL init connection failure");
			continue;
		}

		mysql_real_connect(tmp_conn, m_url.c_str(), m_user.c_str(), m_passward.c_str(),
					m_dbname.c_str(), m_port, NULL, 0);
		if (tmp_conn == NULL) {
			LOG_ERROR("MYSQL init connection failure");
			continue;
		}

		conn_lst.push_back(tmp_conn);
		++ m_freeconn;
	}
	m_maxconn = m_freeconn;
	sem = Sem(m_freeconn);
}

MYSQL* Sqlpool::get_conn() {
	MYSQL* ret_conn;
	sem.wait();
	locker.lock();

	ret_conn = conn_lst.front();
	conn_lst.pop_front();
	-- m_freeconn;
	++ m_curconn;

	locker.unlock();
	return ret_conn;
}

bool Sqlpool::release_conn(MYSQL* conn) {
	if (conn == NULL) {
		return false;
	}
	locker.lock();

	conn_lst.push_back(conn);
	++ m_freeconn;
	-- m_curconn;

	locker.unlock();
	sem.post();
	return true;
}


void Sqlpool::destory_conns() {
	locker.lock();
	if (conn_lst.size() > 0) {
		std::list<MYSQL*>::iterator it = conn_lst.begin();
		for (; it != conn_lst.end(); ++ it) {
			mysql_close(*it);
		}
		m_curconn = 0;
		m_freeconn = 0;
		conn_lst.clear();
	}
	locker.unlock();
}


ConnectionRALL::ConnectionRALL(MYSQL** conn, Sqlpool* pool) {
	*conn = pool->get_conn();
	m_pool = pool;
	m_conn = *conn;
}

ConnectionRALL::~ConnectionRALL() {
	m_pool->release_conn(m_conn);
}

