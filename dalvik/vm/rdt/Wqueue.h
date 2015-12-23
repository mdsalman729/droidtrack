#pragma once

#ifndef __wqueue_h__
#define __wqueue_h__
#include <pthread.h>
#include <list>
#include <string>

using namespace std;
template <typename T> class wqueue
{
	list<T>& m_queue;
	pthread_mutex_t m_mutex;
	pthread_cond_t m_condv;
	
public:
	
	 wqueue(list<T>& queue) : m_queue(queue) {
	   pthread_mutex_init(&m_mutex, NULL);
	   pthread_cond_init(&m_condv, NULL);

	}

	~wqueue() {
		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_condv);
	}

	void add(T item) {
		pthread_mutex_lock(&m_mutex);
		m_queue.push_back(item);
		pthread_cond_signal(&m_condv);
		pthread_mutex_unlock(&m_mutex);
	}

	T remove() {
		pthread_mutex_lock(&m_mutex);
		while (m_queue.size() == 0) {
		  pthread_cond_wait(&m_condv, &m_mutex);
		}
		T item = m_queue.front();
		m_queue.pop_front();
		pthread_mutex_unlock(&m_mutex);
		return item;
	}

	int size() {
		pthread_mutex_lock(&m_mutex);
		int size = m_queue.size();
		pthread_mutex_unlock(&m_mutex);
		return size;
	}
};

class WorkItem
{
public:
	int	    event_name;
	int	    thread_id;
	int	    child_thread_id;
	int	    dest_thread_id;
	int	    tsk_id;
	int	    lock_addr;
	int  	    delay;
	int  	    rwId;
	int	    obj;
	char	    *class_name;
	int	    field_name;

	WorkItem(int name, int tid, int ctid, int dest_tid, int tsk,int lck_addr, int dely, int rwid, int obj, char *class_name, int field_name)
		: event_name(name), thread_id(tid) , child_thread_id(ctid), dest_thread_id(dest_tid),tsk_id(tsk), lock_addr(lck_addr), delay(dely), rwId(rwid),obj(obj), class_name(class_name), field_name(field_name) {}

	~WorkItem() {}
	//const char* getMessage() { return m_message.c_str(); }
	//int getNumber() { return m_number; }
};

void wqueueInit();
#endif
