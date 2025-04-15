#pragma once

#include "threadpool.hpp"

#include <sslConn.h>
#include <tcpConn.h>

// this shouldn't be here but it makes sense for preventing cyclic include
typedef struct runtimeInfo {
	pthread_t         requestAcceptor = 0;
	Socket            serverSocket    = 0;
	SSL_CTX          *sslContext      = nullptr;
	time_t            startTime       = 0;
	ThreadPool::tpool threadPool{};
} runtimeInfo;

void SIGPIPE_handler(int os);

/**
 * polls for activity on the server socket, ir receives a client start the resolveRequestSecure thread
 *
 * @param threadStop if true stops the infinite loop and exits
 */
void acceptRequests(runtimeInfo *rti);

/**
 * receive a client that wants to communicate and attempts to resolve it's request
 *
 * @param threadStop if true stops the infinite loop and exits
 * @param clientSock the socket of the client
 * @param sslConn the ssl connection to communicate on
 */
void resolveRequest(SSL *sslConn, const Socket clientSocket);

/**
 * Proxy function to be called by pthred that itself calls acceptRequestsSecure
 */
void *proxy_accReq(void *ptr);

/**
 * Proxy function to be called by pthred that itself calls resolveRequestSecure
 */
void *proxy_resReq(void *ptr);
