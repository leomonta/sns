#pragma once

#include "threadpool.h"

#include <sslConn.h>
#include <tcpConn.h>

// this shouldn't be here but it makes sense for preventing cyclic include
typedef struct {
	pthread_t  requestAcceptor;
	Socket     serverSocket;
	SSL_CTX   *sslContext;
	time_t     startTime;
	ThreadPool threadPool;
} RuntimeInfo;

void SIGPIPE_handler(int os);

/**
 * polls for activity on the server socket, ir receives a client start the resolveRequestSecure thread
 *
 * @param threadStop if true stops the infinite loop and exits
 */
void accept_requests(RuntimeInfo *rti);

/**
 * receive a client that wants to communicate and attempts to resolve it's request
 *
 * @param threadStop if true stops the infinite loop and exits
 * @param clientSock the socket of the client
 * @param sslConn the ssl connection to communicate on
 */
void resolve_request(SSL *ssl_connection, const Socket client_socket);

/**
 * Proxy function to be called by pthred that itself calls acceptRequestsSecure
 */
void *proxy_acc_req(void *ptr);

/**
 * Proxy function to be called by pthred that itself calls resolveRequestSecure
 */
void *proxy_res_req(void *ptr);
