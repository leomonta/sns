#include "threadpool.h"

#include "RingBuffer_ResolverData.h"
#include "logger.h"
#include "methods.h"
#include "server.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <tcpConn.h>

ThreadPool *initialize_threadpool(const size_t thread_count, ThreadPool *res) {

	// init linked list
	res->ring_buffer  = RingBuffer_ResolverData_make(thread_count);
	res->thread_count = 0;
	res->stop         = false;

	// the semaphore is the one responsible for preventing race conditions
	auto errCode = sem_init(&res->sempahore, 0, 0);
	if (errCode != 0) {
		llog(LOG_ERROR, "[THREAD POOL] Could not initilize semphore -> %s\n", strerror(errno));
	}

	// Linux `man pthread_mutex_init` tells me this always returns 0
	// better safe than sorry?
	errCode = pthread_mutex_init(&res->mutex, NULL);

	switch (errCode) {
	case EAGAIN:
		llog(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, not enough resources\n");
		break;
	case ENOMEM:
		llog(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, not enough memory\n");
		break;
	case EPERM:
		llog(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, not enough priviledges\n");
		break;
	case EINVAL:
		llog(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, invalid settings in attr\n");
		break;
	case 0:
	default:
		break;
	}

	res->threads = malloc(thread_count * sizeof(pthread_t));

	for (size_t i = 0; i < thread_count; ++i) {
		errCode = pthread_create(&res->threads[res->thread_count], NULL, proxy_res_req, (void *)(res));
		switch (errCode) {
		case EINVAL:
			// since I don't use attr for the pthread this should never happen
			llog(LOG_ERROR, "[THREAD POOL] Could not initialize thread, invalid settings in attr\n");
			break;
		case EPERM:
			// since I don't use attr for the pthread this should never happen
			llog(LOG_ERROR, "[THREAD POOL] Could not initialize thread, no permission to set schedule policies for attr\n");
			break;
		case EAGAIN:
			llog(LOG_ERROR, "[THREAD POOL] Could not initialize thread, not enough resources\n");
			break;

		default:
		case 0:
			res->thread_count++;
			break;
		}
	}

	return res;
}

void destroy_threadpool(ThreadPool *tp) {

	// stops all threads
	tp->stop = true;

	// wait for the threads to finish
	for (size_t i = 0; i < tp->thread_count; ++i) {
		// the thread may be wating for a job at the sempahore, by faking data, even if there maybe actually be some,
		// I force the thread to check the stop value. Thus ensuring the thread exits gracefully
		sem_post(&tp->sempahore);
	}

	for (size_t i = 0; i < tp->thread_count; ++i) {
		// since i have no guarantee about the order of which the treads take the sempahore, I have to split the two processes
		pthread_join(tp->threads[i], NULL);
	}

	// free the arrat used to store the thread ids
	free(tp->threads);

	// freeing the leftover data (if any)
	RingBuffer_ResolverData_destroy(&tp->ring_buffer);

	pthread_mutex_destroy(&tp->mutex);
	sem_destroy(&tp->sempahore);
}

ResolverData dequeue_threadpool(ThreadPool *tpool) {

	// this takes care of letting pass n threads for n datas
	sem_wait(&tpool->sempahore);
	pthread_mutex_lock(&tpool->mutex);
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ CRITICAL SECTION START

	ResolverData res;

	if (tpool->ring_buffer.stored == 0) {
		res = (ResolverData){nullptr, INVALID_SOCKET};
	} else {
		RingBuffer_ResolverData_retrieve(&tpool->ring_buffer, &res);
	}

	// let the next one in
	// ---------------------------------------------------------------------------------------------- CRITICAL SECTION END
	pthread_mutex_unlock(&tpool->mutex);

	return res;
}

void enqueue_threadpool(ThreadPool *tpool, const ResolverData *data) {

	pthread_mutex_lock(&tpool->mutex);
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ CRITICAL SECTION START

	RingBuffer_ResolverData_append(&tpool->ring_buffer, data);

	// notify anywating thread that there is data
	sem_post(&tpool->sempahore);
	// ---------------------------------------------------------------------------------------------- CRITICAL SECTION END
	pthread_mutex_unlock(&tpool->mutex);
}
