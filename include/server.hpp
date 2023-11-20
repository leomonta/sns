#pragma once

#include "httpMessage.hpp"
#include "stringRef.hpp"
#include "threadpool.hpp"

#include <sslConn.hpp>
#include <tcpConn.hpp>

struct cliArgs {
	unsigned short tcpPort;
	const char    *baseDir;
};

// this shouldn't be here but it makes sense for preventing cyclic include 
typedef struct runtimeInfo {
	pthread_t          requestAcceptor;
	Socket             serverSocket;
	SSL_CTX           *sslContext = nullptr;
	time_t             startTime;
	ThreadPool::tpool *threadPool;
} runtimeInfo;


/**
 * given the cli arguments set's the server options accordingly
 *
 * @param argc the count of arguments
 * @param argv the values of the arguments
 */
cliArgs parseArgs(const int argc, const char *argv[]);

/**
 * Setup the server, loads libraries and stuff
 *
 * to call one
 */
runtimeInfo setup(cliArgs args);

/**
 * stop the server and its threads
 */
void stop(runtimeInfo *rti);

/**
 * stop and immediatly starts the server again
 */
void restart(cliArgs ca, runtimeInfo *rti);

/**
 * starts the server main thread
 */
void start(runtimeInfo *rti);

/**
 * polls for activity on the server socket, ir receives a client start the resolveRequestSecure thread
 *
 * @param threadStop if true stops the infinite loop and exits
 */
void acceptRequestsSecure(runtimeInfo *rti);

/**
 * receive a client that wants to communicate and attempts to resolve it's request
 *
 * @param threadStop if true stops the infinite loop and exits
 * @param clientSock the socket of the client
 * @param sslConn the ssl connection to communicate on
 */
void resolveRequestSecure(SSL *sslConn, const Socket clientSocket);

/**
 * Proxy function to be called by pthred that itself calls acceptRequestsSecure
 */
void *proxy_accReq(void *ptr);

/**
 * Proxy function to be called by pthred that itself calls resolveRequestSecure
 */
void *proxy_resReq(void *ptr);
