#include "webserver.h"

int main(int argc, char*argv[]) {
	int thread_num = 8;
	if (argc == 2) {
		thread_num = atoi(argv[1]);
	}
	int port = 9006;
	if (argc == 3) {
		thread_num = atoi(argv[1]);
		port = atoi(argv[2]);
	}

	WebServer Server;
	Server.init(port, thread_num);
	Server.threadpool_init();

	Server.sql_init();

	Server.log_write();

	Server.timer_lst_init();
	Server.event_listen();
	Server.event_loop();
	return 0;
}
