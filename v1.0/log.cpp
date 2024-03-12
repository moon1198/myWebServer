#include "./log/log.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

//log format
//time : type info : content
//

Log::Log() : m_count(0), m_today(-1), m_fp(NULL) {

}

Log::~Log() {
	if (m_fp != NULL) {
		fclose(m_fp);
	}
}

bool Log::init(const char* file_name, bool close_log, int line_max, int max_queue_size, int buf_size) {
	if (max_queue_size > 0) {
		//m_is_async = true;
		//m_log_queue = new Block_queue<string>(max_queue_size);

		////由多个htpp连接线程，生产log队列， 由此线程处理log队列
		////为多生产者-单消费者模型
		//pthread_t tid;
		//pthread_create(&tid, NULL, flush_log_thread, NULL);
	}

	m_close = close_log;
	m_line_max = line_max;
	m_log_size = buf_size;

	const char* tmp = strrchr(file_name, '/');

	time_t today_t = time(NULL);
	struct tm* today = localtime(&today_t);
	
	if (tmp != NULL) {
		strcpy(m_name, tmp + 1);
		strncpy(m_dir, file_name, tmp - file_name + 1);
		snprintf(m_full_name, 255, "%s%04d_%02d_%02d_%s", m_dir, today->tm_year + 1900, today->tm_mon + 1, today->tm_mday, m_name);

	} else {
		strcpy(m_name, file_name);
		snprintf(m_full_name, 255, "%04d_%02d_%02d_%s", today->tm_year + 1900, today->tm_mon + 1, today->tm_mday, m_name);
	}
	m_today = today->tm_yday;
	m_fp = fopen(m_full_name, "a");
	if (m_fp == NULL) {
		return false;
	}
	return true;
}

void Log::write_log(int level,const char* format, ...) {

	char head[32];
	switch (level) {
		case 0:
			snprintf(head, 32, "[Debug]: ");
			break;
		case 1:
			snprintf(head, 32, "[Warn]: ");
			break;
		case 2:
			snprintf(head, 32, "[Info]: ");
			break;
		case 3:
			snprintf(head, 32, "[Error]: ");
			break;
		default:
			snprintf(head, 32, "[Undefine type]: ");
			break;
	}

	time_t today_t = time(NULL);
	struct tm* today = localtime(&today_t);

	m_locker.lock();
	++ m_count;
	if (m_today != today->tm_yday || m_count > m_line_max) {
		//time or out of lines
		//should new a new file

		char new_log[512] = {0};
		fflush(m_fp);
		fclose(m_fp);

		char time_head[32];
		snprintf(time_head, 32, "%04d_%02d_%02d", today->tm_year + 1900, today->tm_mon + 1, today->tm_mday);
		if (m_today != today->tm_yday) {
			//other day
			snprintf(new_log, 512, "%s%s%s", m_dir, time_head, m_name);
			m_today = today->tm_yday;
			m_count = 0;
		} else {
			snprintf(new_log, 512, "%s%s%s.%d", m_dir, time_head, m_name, m_count / m_line_max);
		}
		m_fp = fopen(new_log, "a");
	}
	m_locker.unlock();


	va_list va_args;
	va_start(va_args, format);
	char log_buf[m_log_size];
	int n = snprintf(log_buf, m_log_size - 1, "%04d_%02d_%02d %02d:%02d:%02d %s ", 
			today->tm_year + 1900, today->tm_mon + 1, today->tm_mday, today->tm_hour, 
			today->tm_min, today->tm_sec, head);
	int m = vsnprintf(log_buf + n, m_log_size - n - 1, format, va_args);

	log_buf[n + m] = '\n';
	log_buf[n + m + 1] = '\0';
	va_end(va_args);


	//async or not
	//m_queue is full or not
	//if (m_is_async || !m_log_queue->full) {
	if (m_is_async) {
		//m_log_queue->push(log_buf);
	} else {
		m_locker.lock();
		fputs(log_buf, m_fp);
		m_locker.unlock();
	}
}


void Log::flush() {
	m_locker.lock();
	fflush(m_fp);
	m_locker.unlock();
}
