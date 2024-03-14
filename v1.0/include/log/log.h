#ifndef _LOG_H__
#define _LOG_H__

#include "lock/locker.h"
#include "log/block_queue.h"
#include <cstdio>
#include <string>

class Log
{

	public:
		static Log* get_instance() {
			static Log instance;
			return &instance;
		}
		static void* flush_log_thread(void* );
		bool init(const char*, bool, int, int max_queue_size = 0, int buf_size = 512);
		void write_log(int level, const char* format, ...);
		void async_write();
		void flush();

	private:
		Log();
		~Log();

		bool m_is_async;
		bool m_close;
		Block_queue<std::string>* block_queue;


		int m_count;
		int m_line_max;
		int m_today;

		char m_dir[128]; //
		char m_name[128]; //log file name
		char m_full_name[512];
		FILE* m_fp;

		Locker m_locker;
		int m_log_size;
};


#define LOG_DEBUG(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}

#endif
