#include "timer/lst_timer.h"

Timer_lst::Timer_lst() {
	head = new Timer();
	tail = new Timer();
	head->next = tail;
	tail->prev = head;
}

Timer_lst::~Timer_lst() {
	delete head;
	delete tail;
}

void Timer_lst::tick() {
	//std::cout << "tick" << std::endl;
	time_t cur_time = time(NULL);
	Timer* tmp = head->next;
	while (tmp != tail) {
		if (tmp->expire <= cur_time) {
			tmp->cb_func(tmp->data);
			del_timer(tmp);
			tmp = tmp->next;
		} else {
			break;
		}
	}

}

void Timer_lst::add_timer(Timer* timer) {
	if (!timer) {
		return ;
	}
	Timer* tmp = head->next;
	while (tmp != tail) {
		if (timer->expire <= tmp->expire) {
			timer->prev = tmp->prev;
			timer->next = tmp;
			tmp->prev->next = timer;
			tmp->prev = timer;
			break;
		}
		tmp = tmp->next;
	}
	if (tmp == tail) {
		timer->prev = tmp->prev;
		timer->next = tmp;
		tmp->prev->next = timer;
		tmp->prev = timer;
	}
	return ;

	//Timer* tmp = head;

	//while (tmp->next != tail) {
	//	if (timer->expire < tmp->next->expire) {
	//		timer->prev = tmp;
	//		timer->next = tmp->next;
	//		tmp->next->prev = timer;
	//		tmp->next = timer;
	//	}
	//}
	//timer->prev = tmp;
	//timer->next = tmp->next;
	//tmp->next->prev = timer;
	//tmp->next = timer;

}

void Timer_lst::del_timer(Timer* timer) {
	if (!timer || timer == head || timer == tail) {
		return ;
	}
	timer->prev->next = timer->next;
	timer->next->prev = timer->prev;
	delete timer;
}

void Timer_lst::adjust_timer(Timer* timer) {
	if (!timer) {
		return ;
	}
	timer->prev->next = timer->next;
	timer->next->prev = timer->prev;
	add_timer(timer);
}


