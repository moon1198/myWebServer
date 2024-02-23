#ifndef __LOCKER_H_
#define __LOCKER_H_

#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

class Locker 
{
public:
	Locker() {
		if (pthread_mutex_init(&m_mutex, NULL) != 0) {
			assert(0);
		}
	}

	bool lock() {
		return pthread_mutex_lock(&m_mutex) == 0;
	}

	bool unlock() {
		return pthread_mutex_unlock(&m_mutex) == 0;
	}

	pthread_mutex_t *get() {
		return &m_mutex;
	}

	~Locker() {
		pthread_mutex_destroy(&m_mutex);
	}

private:
	pthread_mutex_t m_mutex;

};

class Cond {
public:
	Cond() {
		if (pthread_cond_init(&m_cond, NULL) != 0) {
			assert(0);
		}
	}

	void wait(Locker *mutex) {
		pthread_cond_wait(&m_cond, mutex->get());
	}

	void signal() {
		pthread_cond_signal(&m_cond);
	}

	void broadcast() {
		pthread_cond_broadcast(&m_cond);
	}

	~Cond() {
		pthread_cond_destroy(&m_cond);
	}
private:
	pthread_cond_t m_cond;
	Locker m_mutex;

};

class Sem {
public:
	Sem() {
		if (sem_init(&m_sem, 0, 0) != 0) {
			assert(0);
		}
	}

	Sem(int size) {
		if (sem_init(&m_sem, 0, size) != 0) {
			assert(0);
		}
	}

	bool wait() {
		return sem_wait(&m_sem) == 0;
	}

	bool post() {
		return sem_post(&m_sem) == 0;
	}
	
	~Sem() {
		sem_destroy(&m_sem);
	}

private:
	sem_t m_sem;

};


#endif
