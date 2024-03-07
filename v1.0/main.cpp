#include "webserver.h"

int main(int argc, char*argv[]) {
	WebServer Server;
	Server.init(9006, 1);
	Server.threadpool_init();
	Server.timer_lst_init();
	Server.event_listen();
	Server.event_loop();
	return 0;
}
