#include "threadpool.hpp"

#include "logger.h"
#include "methods.hpp"
#include "miniVector.hpp"
#include "ringBuffer.hpp"
#include "server.hpp"

#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <tcpConn.h>

ThreadPool::tpool *ThreadPool::initialize(const size_t thread_count, tpool *res) {

	// init linked list
	res->ring_buffer       = ringBuffer::make<Methods::resolver_data>(thread_count);
	res->thread_count      = 0;
	res->stop              = false;

	// the semaphore is the one responsible for preventing race conditions
	auto errCode = sem_init(&res->sempahore, 0, 0);
	if (errCode != 0) {
		llog(LOG_ERROR, "[THREAD POOL] Could not initilize semphore -> %s\n", strerror(errno));
	}

	// Linux `man pthread_mutex_init` tells me this always returns 0
	// but pthread `man pthread_mutex_init` tells me this returns non zero on error
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

	res->threads = (pthread_t *)(malloc(thread_count * sizeof(pthread_t)));

	for (size_t i = 0; i < thread_count; ++i) {
		errCode = pthread_create(&res->threads[res->thread_count], NULL, proxy_resReq, (void *)(res));
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

void ThreadPool::destroy(tpool *tp) {

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
	ringBuffer::destroy(&tp->ring_buffer);

	pthread_mutex_destroy(&tp->mutex);
	sem_destroy(&tp->sempahore);
}

Methods::resolver_data ThreadPool::dequeue(tpool *tpool) {

	// this takes care of letting pass n threads for n datas
	sem_wait(&tpool->sempahore);
	pthread_mutex_lock(&tpool->mutex);
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ CRITICAL SECTION START

	Methods::resolver_data res;

	if (tpool->ring_buffer.stored == 0) {
		res = {nullptr, INVALID_SOCKET};
	} else {
		res = *ringBuffer::retrieve(&tpool->ring_buffer);
		/*
		* AddressSanitizer:DEADLYSIGNAL
		* =================================================================
		* ==24505==ERROR: AddressSanitizer: SEGV on unknown address 0x000000000008 (pc 0x57b0b83bd1af bp 0x78ef4f7fecd0 sp 0x78ef4f7fec40 T1)
		* ==24505==The signal is caused by a READ memory access.
		* ==24505==Hint: address points to the zero page.
		    * #0 0x57b0b83bd1af in ThreadPool::dequeue(ThreadPool::tpool*) src/threadpool.cpp:125
		    * #1 0x57b0b8338e21 in proxy_resReq(void*) src/server.cpp:47
		    * #2 0x7cef5345e29a in asan_thread_start /usr/src/debug/gcc/gcc/libsanitizer/asan/asan_interceptors.cpp:239
		    * #3 0x7cef528a57ea  (/usr/lib/libc.so.6+0x957ea) (BuildId: 468e3585c794491a48ea75fceb9e4d6b1464fc35)
		    * #4 0x7cef5292918b  (/usr/lib/libc.so.6+0x11918b) (BuildId: 468e3585c794491a48ea75fceb9e4d6b1464fc35)
		*
		* ==24505==Register values:
		* rax = 0x0000000000000000  rbx = 0x000078ef4c5419c0  rcx = 0x0000000000000000  rdx = 0x0000000000000000
		* rdi = 0x000078ef50900090  rsi = 0x0000000000000014  rbp = 0x000078ef4f7fecd0  rsp = 0x000078ef4f7fec40
		* r8 = 0x0000000000000000   r9 = 0x0000000000000000  r10 = 0x0000000000000000  r11 = 0x000078ef4f7ff6c0
		* r12 = 0x000078ef4c541a20  r13 = 0x00000f1de98a8338  r14 = 0x000078ef4f7fec50  r15 = 0x000078ef50900090
		* AddressSanitizer can not provide additional info.
		* SUMMARY: AddressSanitizer: SEGV src/threadpool.cpp:125 in ThreadPool::dequeue(ThreadPool::tpool*)
		* Thread T1 created by T0 here:
		    * #0 0x7cef535174ac in pthread_create /usr/src/debug/gcc/gcc/libsanitizer/asan/asan_interceptors.cpp:250
		    * #1 0x57b0b83bccf6 in ThreadPool::initialize(unsigned long, ThreadPool::tpool*) src/threadpool.cpp:60
		    * #2 0x57b0b83384f3 in setup(cliArgs) src/main.cpp:92
		    * #3 0x57b0b8337d45 in main src/main.cpp:21
		    * #4 0x7cef528376b4  (/usr/lib/libc.so.6+0x276b4) (BuildId: 468e3585c794491a48ea75fceb9e4d6b1464fc35)
		    * #5 0x7cef52837768 in __libc_start_main (/usr/lib/libc.so.6+0x27768) (BuildId: 468e3585c794491a48ea75fceb9e4d6b1464fc35)
		    * #6 0x57b0b8330b34 in _start (/usr/local/bin/sns+0x1bb34) (BuildId: ad7fc3f3fc7eff0a7b000fe5f3e1059499f87006)

		* ==24505==ABORTING

		 */
	}

	// let the next one in
	// ---------------------------------------------------------------------------------------------- CRITICAL SECTION END
	pthread_mutex_unlock(&tpool->mutex);

	return res;
}

void ThreadPool::enqueue(tpool *tpool, const Methods::resolver_data *data) {

	pthread_mutex_lock(&tpool->mutex);
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ CRITICAL SECTION START

	ringBuffer::append(&tpool->ring_buffer, data);

	// notify anywating thread that there is data
	sem_post(&tpool->sempahore);
	// ---------------------------------------------------------------------------------------------- CRITICAL SECTION END
	pthread_mutex_unlock(&tpool->mutex);
}
