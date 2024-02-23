#include <string>
#include <iostream>
#include <unistd.h>
#include "threadpool.h"

class job {
public:
	job(std::string str) : m_str(str) {};
	void run() {
		std::cout << m_str;
	}
	~job() {};
private:
	std::string m_str;
};

int main (int argc, char *argv[]) {
	Threadpool<job> pool(8, 8);
	job job1("aaaa");
	job job2("bbbb");
	job job3("cccc");
	job job4("dddd");
	int i = 5;
	while(i --) {
		pool.push(&job1);
		pool.push(&job2);
		pool.push(&job3);
		pool.push(&job4);
	}
	sleep(5);

	return 0;
}
