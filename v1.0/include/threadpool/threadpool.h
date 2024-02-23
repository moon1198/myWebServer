#ifndef __THREADPOOL_H_
#define __THREADPOOL_H_

#include "../lock/locker.h"
#include <list>
#include <exception>
#include <pthread.h>



template <typename T>
class Threadpool
{
public:
	Threadpool(int thread_num = 8, int max_list = 10000);
	~Threadpool();
	bool push(T *);
	T* pop();

private:
	int m_thread_num;			//the thread_num of threadpool
	pthread_t *m_threads;		//the array of threads
	int m_max_list;				//the max size of requests
	std::list<T *> m_queue;		//the queue of requests
	Locker m_locker;				//locker
	Sem m_sem;					

private:
	//传入pthread_create的函数，需要使用静态成员函数，非静态成员函数会默认传入this指针，与void *
	//冲突，编译失败
	//可以使用静态成员函数，并附加this指针作为参数，以指明线程所在线程池；
	static void* worker(void *arg);
	//void run();
};

template <typename T>
Threadpool<T>::Threadpool(int thread_num, int max_list) : m_thread_num(thread_num), m_max_list(max_list), m_threads(NULL) {
	if (m_thread_num <= 0 || m_max_list <= 0) 
		throw std::exception();

	m_threads = new pthread_t[m_thread_num];
	assert(m_threads != NULL);
	m_sem = Sem(0);
	m_locker = Locker();
	for (int i = 0; i < m_thread_num; ++ i) {
		//create each thread for threadpool
		if (pthread_create(&m_threads[i], NULL, worker, (void *)this) != 0) {
			delete[] m_threads;
			assert(0);
		}
		if (pthread_detach(m_threads[i]) != 0) {
			delete[] m_threads;
			assert(0);
		}
	}
	std::cout << "finish init pool now." << std::endl;
}

template <typename T>
Threadpool<T>::~Threadpool() {
	delete[] m_threads;
}


template <typename T>
void *Threadpool<T>::worker(void *arg) {
	Threadpool *pool = (Threadpool *) arg;

	while (1) {
		T *task = pool->pop();
		task->run();
	}
}

template <typename T>
bool Threadpool<T>::push(T *task) {
	m_locker.lock();
	//若超出队列长度，舍弃任务，
	if (m_queue.size() >= m_max_list) {
		m_locker.unlock();
		return false;
   	}
	m_queue.push_back(task);
	m_locker.unlock();
	m_sem.post();
	return true;
}

template <typename T>
T *Threadpool<T>::pop() {
	m_sem.wait();
	m_locker.lock();
	T *task = m_queue.front();
	m_queue.pop_front();
	m_locker.unlock();
	return task;
}

#endif
