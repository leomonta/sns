#include "threadpool.hpp"

#include <asm-generic/errno-base.h>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <logger.hpp>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

ThreadPool::tpool *ThreadPool::create(size_t tCount) {
	tpool *res = (tpool *)(malloc(sizeof(tpool)));

	res->head   = nullptr;
	res->tail   = nullptr;
	res->tCount = 0;

	// the semaphore is the one responsible for preventing race conditions
	auto errCode = sem_init(&res->sempahore, 0, 0);
	if (errCode != 0) {
		log(LOG_ERROR, "[THREAD POOL] Could not initilize semphore -> %s\n", strerror(errno));
	}

	errCode = pthread_mutex_init(&res->mutex, NULL);

	switch (errCode) {
	case EAGAIN:
		log(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, not enough resources\n");
		break;
	case ENOMEM:
		log(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, not enough memory\n");
		break;
	case EPERM:
		log(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, not enough priviledges\n");
		break;
	case EINVAL:
		log(LOG_ERROR, "[THREAD POOL] Could not initializer mutex, invalid settings in attr\n");
		break;
	case 0:
	default:
		break;
	}

	res->threads = (pthread_t *)(malloc(tCount * sizeof(pthread_t)));

	for (size_t i = 0; i < tCount; ++i) {
		errCode = pthread_create(&res->threads[res->tCount], NULL, proxy_resReq, (void *)(res));
		switch (errCode) {
		case EINVAL:
			// since I don't use attr for the pthread this should never happen
			log(LOG_ERROR, "[THREAD POOL] Could not initialize thread, invalid settings in attr\n");
			break;
		case EPERM:
			// since I don't use attr for the pthread this should never happen
			log(LOG_ERROR, "[THREAD POOL] Could not initialize thread, no permission to set schedule policies for attr\n");
			break;
		case EAGAIN:
			log(LOG_ERROR, "[THREAD POOL] Could not initialize thread, not enough resources\n");
			break;

		default:
		case 0:
			res->tCount++;
			break;
		}
	}

	return res;
}

void ThreadPool::destroy(tpool *tp) {

	// stops all threads
	tp->stop = true;

	// wait for the threads to finish
	for (size_t i = 0; i < tp->tCount; ++i) {
		// th thread may be wating for a job at the sempahore, by faking data (in case there is or there isn't)
		// I force the thread to check the stop value. Thus ensuring the thread exits gracefully
		// I might also start a timer of 0.5s to then call pthread_kill(SIG_KILL) to forcefully terminate the thread
		sem_post(&tp->sempahore);
		pthread_join(tp->threads[i], NULL);
	}

	// free the arrat used to store the thread ids
	free(tp->threads);

	// deleting the leftover data
	tjob *cursor = tp->head;

	while (cursor != nullptr) {
		auto t = cursor->next;
		free(cursor);
		cursor = t;
	}

	pthread_mutex_destroy(&tp->mutex);
	sem_destroy(&tp->sempahore);

	// lastly
	free(tp);
}

resolver_data ThreadPool::dequeue(tpool *tpool) {

	sem_wait(&tpool->sempahore);
	// this is not a problem, the request resolver will deal with n null return value
	pthread_mutex_lock(&tpool->mutex);

	// I'm sure there is a job for a thread

	resolver_data res;

	if (tpool->head == nullptr) {
		res = {nullptr, 0};
	} else {
		res         = tpool->head->data;
		if (tpool->head->next == nullptr) {
			tpool->tail = nullptr;
		}
		auto t      = tpool->head;
		tpool->head = tpool->head->next;
		free(t);
	}

	// let the next one in
	pthread_mutex_unlock(&tpool->mutex);

	return res;
}

void ThreadPool::enque(tpool *tpool, resolver_data *data) {

	pthread_mutex_lock(&tpool->mutex);
	tjob *ntail = (tjob *)(malloc(sizeof(tjob)));

	ntail->next = nullptr;
	ntail->data = *data;

	// headless / empty
	if (tpool->head == nullptr) {
		tpool->head = ntail;
		tpool->tail = ntail;
	} else {
		tpool->tail->next = ntail;
		tpool->tail       = ntail;
	}

	// notify anywating thread that there is data
	sem_post(&tpool->sempahore);
	pthread_mutex_unlock(&tpool->mutex);
}
