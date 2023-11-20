#pragma once
#include "server.hpp"

#include <cstddef>
#include <pthread.h>
#include <semaphore.h>
#include <sslConn.hpp>
#include <tcpConn.hpp>

/**
 * A thread pool created exclusively for managing the resolveRequest function
 */
namespace ThreadPool {

	typedef struct tjob {
		resolver_data data; // 12bytes
		tjob         *next; // 8 + 4 padding
	} tjob;

	typedef struct tpool {
		tjob           *head;
		tjob           *tail;
		sem_t           sempahore;
		pthread_mutex_t mutex;
		size_t          tCount;
		pthread_t      *threads;
		size_t          worksCount;
		bool            stop;
	} tpool;

	/**
	 * Create a thread pool on the heap and returns it.
	 * It also start tCount threads with the proxy_resReq function
	 *
	 * @param tCount the number of concurrent threads to start
	 * @return an allocated thread pool
	 */
	tpool *create(size_t tCount);

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
	resolver_data dequeue(tpool *tpool);

	/**
	 * Add the given data to the queue as a job for ay waiting thread
	 *
	 * @param tpool the thread pool where to put the job in
	 * @param data the data that will be copied into the thread job
	 */
	void enque(tpool *tpool, resolver_data *data);

} // namespace ThreadPool
