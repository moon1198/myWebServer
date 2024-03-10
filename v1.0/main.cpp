#include "webserver.h"

int main(int argc, char*argv[]) {
	int thread_num = 8;
	if (argc == 2) {
		thread_num = atoi(argv[1]);
	}

	WebServer Server;
	Server.init(9006, thread_num);
	Server.threadpool_init();
	Server.timer_lst_init();
	Server.event_listen();
	Server.event_loop();
	return 0;
}
