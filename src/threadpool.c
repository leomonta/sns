#include "threadpool.h"

#include "RingBuffer_ResolverData.h"
#include "logger.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <tcpConn.h>

struct ThreadPool {
	RingBuffer_ResolverData ring_buffer;  // the jobs arranged in a threadBuffer
	sem_t                   sempahore;    // semaphore to decide how many can get in the critical section
	pthread_mutex_t         mutex;        // mutex to enter the critical section
	size_t                  thread_count; // num of threads allocated in *threads
	pthread_t              *threads;      // array of the pthreads started, it will not change once the tpoll is created
	bool                    stop;         // should the threads stop
};

ERRFUNC initialize_threadpool(const size_t thread_count, GenericFunction fun, ThreadPool *thread_pool) {

	// init linked list
	auto err = RingBuffer_ResolverData_make(thread_count, &thread_pool->ring_buffer);
	RET_IF_ERR(err)
	thread_pool->thread_count = 0;
	thread_pool->stop         = false;

	// the semaphore is the one responsible for preventing race conditions
	auto errCode = sem_init(&thread_pool->sempahore, 0, 0);
	if (errCode != 0) {
		llog(LOG_ERROR, "[THREAD POOL] Could not initilize semphore -> %s\n", strerror(errno));
		return THREADPOOL__FAILED_SEMAPHORE_INIT;
	}

	// Linux `man pthread_mutex_init` tells me this always returns 0
	// better safe than sorry?
	errCode = pthread_mutex_init(&thread_pool->mutex, NULL);

	switch (errCode) {
	case EAGAIN:
		llog(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, not enough resources\n");
		return THREADPOOL__FAILED_MUTEX_INIT;
	case ENOMEM:
		llog(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, not enough memory\n");
		return THREADPOOL__FAILED_MUTEX_INIT;
	case EPERM:
		llog(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, not enough priviledges\n");
		return THREADPOOL__FAILED_MUTEX_INIT;
	case EINVAL:
		llog(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, invalid settings in attr\n");
		return THREADPOOL__FAILED_MUTEX_INIT;
	case 0:
	default:
		break;
	}

	thread_pool->threads = malloc(thread_count * sizeof(pthread_t));
	TEST_ALLOC(thread_pool->threads)

	for (size_t i = 0; i < thread_count; ++i) {
		errCode = pthread_create(&thread_pool->threads[thread_pool->thread_count], NULL, fun, (void *)(thread_pool));
		switch (errCode) {
		case EINVAL:
			// since I don't use attr for the pthread this should never happen
			llog(LOG_ERROR, "[THREAD POOL] Could not initialize thread, invalid settings in attr\n");
			return THREADPOOL__FAILED_THREAD_CREATION;
		case EPERM:
			// since I don't use attr for the pthread this should never happen
			llog(LOG_ERROR, "[THREAD POOL] Could not initialize thread, no permission to set schedule policies for attr\n");
			return THREADPOOL__FAILED_THREAD_CREATION;
		case EAGAIN:
			llog(LOG_ERROR, "[THREAD POOL] Could not initialize thread, not enough resources\n");
			return THREADPOOL__FAILED_THREAD_CREATION;

		default:
		case 0:
			thread_pool->thread_count++;
			break;
		}
	}

	return NO_ERROR;
}

ERRFUNC destroy_threadpool(ThreadPool *thread_pool) {

	// stops all threads
	thread_pool->stop = true;

	// wait for the threads to finish
	for (size_t i = 0; i < thread_pool->thread_count; ++i) {
		// the thread may be wating for a job at the sempahore, by faking data, even if there maybe actually be some,
		// I force the thread to check the stop value. Thus ensuring the thread exits gracefully
		sem_post(&thread_pool->sempahore);
	}

	for (size_t i = 0; i < thread_pool->thread_count; ++i) {
		// since i have no guarantee about the order of which the treads take the sempahore, I have to split the two processes
		pthread_join(thread_pool->threads[i], NULL);
	}

	// free the arrat used to store the thread ids
	free(thread_pool->threads);

	// freeing the leftover data (if any)
	auto err = RingBuffer_ResolverData_destroy(&thread_pool->ring_buffer);
	RET_IF_ERR(err)

	pthread_mutex_destroy(&thread_pool->mutex);
	sem_destroy(&thread_pool->sempahore);

	return NO_ERROR;
}

ERRFUNC dequeue_threadpool(ThreadPool *thread_pool, ResolverData *data) {

	// this takes care of letting pass n threads for n datas
	sem_wait(&thread_pool->sempahore);
	pthread_mutex_lock(&thread_pool->mutex);
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ CRITICAL SECTION START

	if (thread_pool->ring_buffer.stored == 0) {
		*data = (ResolverData){nullptr, INVALID_SOCKET, nullptr};
	} else {
		auto err = RingBuffer_ResolverData_retrieve(&thread_pool->ring_buffer, data);
		if (err > NO_OP) {
			// ------------------------------------------------------------------------------ EARLY EXIT
			pthread_mutex_unlock(&thread_pool->mutex);
			return err;
		}
	}

	// let the next one in
	// ---------------------------------------------------------------------------------------------- CRITICAL SECTION END
	pthread_mutex_unlock(&thread_pool->mutex);

	return NO_ERROR;
}

ERRFUNC enqueue_threadpool(ThreadPool *thread_pool, const ResolverData *data) {

	pthread_mutex_lock(&thread_pool->mutex);
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ CRITICAL SECTION START

	auto err = RingBuffer_ResolverData_append(&thread_pool->ring_buffer, data);
	if (err > NO_OP) {
		// -------------------------------------------------------------------------------------- EARLY EXIT
		pthread_mutex_unlock(&thread_pool->mutex);
		return err;
	}

	// notify anywating thread that there is data
	sem_post(&thread_pool->sempahore);
	// ---------------------------------------------------------------------------------------------- CRITICAL SECTION END
	pthread_mutex_unlock(&thread_pool->mutex);

	return NO_ERROR;
}
