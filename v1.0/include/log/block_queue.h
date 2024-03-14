#ifndef __BLOCK_QUEUE_H_
#define __BLOCK_QUEUE_H_

//#include "lock/locker.h"
#include "../lock/locker.h"
#include <queue>

template <typename T>
class Block_queue {
	
public:
	Block_queue(int queue_size);
	~Block_queue();

	void push(T str);
	T pop();

	T front();
	T back();

	bool full();
	bool empty();

private:
	Locker locker;
	Sem m_sem;

	std::queue<T> m_queue;
	int m_size;
	int cur_size;

};


template <typename T>
Block_queue<T>::Block_queue(int queue_size) : m_size(queue_size) {
	m_sem = Sem(0);
	locker = Locker();
}

template <typename T>
Block_queue<T>::~Block_queue() {

}

template <typename T>
void Block_queue<T>::push(T val) {
	locker.lock();
	m_queue.push(val);
	locker.unlock();
	m_sem.post();
}

template <typename T>
T Block_queue<T>::pop() {
	m_sem.wait();
	locker.lock();
	T tmp = m_queue.front();
	m_queue.pop();
	locker.unlock();
	return tmp;
}

template <typename T>
T Block_queue<T>::front() {
	locker.lock();
	T tmp = m_queue.front();
	locker.unlock();
	return tmp;
}

template <typename T>
T Block_queue<T>::back() {
	locker.lock();
	T tmp = m_queue.back();
	locker.unlock();
	return tmp;
}

template <typename T>
bool Block_queue<T>::empty() {
	locker.lock();
	bool ret = m_queue.empty();
	locker.unlock();
	return ret;
}

template <typename T>
bool Block_queue<T>::full() {
	locker.lock();
	int cur_size = m_queue.size();
	locker.unlock();
	if (m_size <= cur_size) {
		return true;
	}
	return false;
}



#endif
