#pragma once

#include "methods.hpp"

#include <cstddef>
#include <pthread.h>
#include <semaphore.h>
#include <sslConn.hpp>
#include <tcpConn.hpp>

/**
 * A thread pool created exclusively for managing the resolveRequest function
 */
namespace ThreadPool {

	// linked list node
	typedef struct tjob {
		Methods::resolver_data data; // data of the node. I use a copy since the data is small (12B) and trivial int + ptr
		tjob                  *next; // next job
	} tjob;

	// linked list + other thread related data
	typedef struct tpool {
		tjob           *head;      // head of linked list
		tjob           *tail;      // tail of linked list
		sem_t           sempahore; // semaphore to decide how many can get in the critical section
		pthread_mutex_t mutex;     // mutex to enter the critical section
		size_t          tCount;    // num of threads allocated in *threads
		pthread_t      *threads;   // array of the pthreads started, it will not change once the tpoll is created
		bool            stop;      // should the threads stop
	} tpool;

	/**
	 * Create a thread pool on the heap and returns it.
	 * It also start tCount threads with the proxy_resReq function
	 *
	 * @param tCount the number of concurrent threads to start
	 * @return an allocated thread pool
	 */
	tpool *create(const size_t tCount);

	/**
	 * Free all the resource allocated in the given thrad pool and the thread pool itself
	 *
	 * @parma tp the thread pool the destroy
	 */
	void destroy(tpool *tp);

	/**
	 * Take a job form the given threadpool queue
	 *
	 * @parama tpool the thread pool where to take the job from
	 * @return a pointer to a resolver_data structure
	 */
	Methods::resolver_data dequeue(tpool *tpool);

	/**
	 * Add the given data to the queue as a job for ay waiting thread
	 *
	 * @param tpool the thread pool where to put the job in
	 * @param data the data that will be copied into the thread job
	 */
	void enque(tpool *tpool, const Methods::resolver_data *data);

} // namespace ThreadPool
