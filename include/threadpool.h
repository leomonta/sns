#pragma once

#include "RingBuffer_ResolverData.h"

#include <pthread.h>
#include <semaphore.h>
#include <sslConn.h>
#include <stddef.h>

/**
 * A thread pool created exclusively for managing the resolveRequest function
 */

// linked list + other thread related data
typedef struct {
	RingBuffer_ResolverData ring_buffer;  // the jobs arranged in a threadBuffer
	sem_t                   sempahore;    // semaphore to decide how many can get in the critical section
	pthread_mutex_t         mutex;        // mutex to enter the critical section
	size_t                  thread_count; // num of threads allocated in *threads
	pthread_t              *threads;      // array of the pthreads started, it will not change once the tpoll is created
	bool                    stop;         // should the threads stop
} ThreadPool;

typedef void *(*GenericFunction)(void *);

/**
 * Create a thread pool on the heap and returns it.
 * It also start tCount threads with the proxy_resReq function
 *
 * @param[in] `thread_count` the number of concurrent threads to start
 * @param[in] `fun` the function to call for thread workers
 * @param[out] `thread_pool` the thread pool to initialize
 *
 * @return the modified thread pool
 */
ERRFUNC initialize_threadpool(const size_t thread_count, GenericFunction fun, ThreadPool *thread_pool);

/**
 * Free all the resource allocated in the given thrad pool
 *
 * @parma[in] `thread_pool` the thread pool the destroy
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC destroy_threadpool(ThreadPool *thread_pool);

/**
 * Take a job form the given threadpool queue
 *
 * @param[in] `thread_pool` the thread pool where to take the job from
 * @param[out] `data` where to place the dequeued data.
 * if NO_OP or another error returned this values should be ignored as there was no data to communicate back
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC dequeue_threadpool(ThreadPool *thread_pool, ResolverData *data);

/**
 * Add the given data to the queue as a job for ay waiting thread
 *
 * @param[in] `thread_pool` the thread pool where to put the job in
 * @param[in] `data` the data that will be copied into the thread job
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC enqueue_threadpool(ThreadPool *thread_pool, const ResolverData *data);
