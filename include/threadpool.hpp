#pragma once

#include "methods.hpp"
#include "miniVector.hpp"

#include <cstddef>
#include <pthread.h>
#include <semaphore.h>
#include <sslConn.h>
#include <tcpConn.h>

/**
 * A thread pool created exclusively for managing the resolveRequest function
 */
namespace ThreadPool {

	// linked list + other thread related data
	typedef struct tpool {
		miniVector::miniVector<Methods::resolver_data> ring_buffer;     // the jobs arranged in a threadBuffer
		size_t                                         ring_data_count; // amount of stored data in the ring buffer
		size_t                                         ring_read_index; // index of the last job
		sem_t                                          sempahore;       // semaphore to decide how many can get in the critical section
		pthread_mutex_t                                mutex;           // mutex to enter the critical section
		size_t                                         thread_count;    // num of threads allocated in *threads
		pthread_t                                     *threads;         // array of the pthreads started, it will not change once the tpoll is created
		bool                                           stop;            // should the threads stop
	} tpool;

	/**
	 * Create a thread pool on the heap and returns it.
	 * It also start tCount threads with the proxy_resReq function
	 *
	 * @param[in] thread_count the number of concurrent threads to start
	 * @param[out] res the thread pool to initialize
	 * @return the modified thread pool
	 */
	tpool *initialize(const size_t thread_count, tpool *res);

	/**
	 * Free all the resource allocated in the given thrad pool
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
	void enqueue(tpool *tpool, const Methods::resolver_data *data);

} // namespace ThreadPool
