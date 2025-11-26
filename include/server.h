#pragma once

#include "StringRef.h"
#include "threadpool.h"

#include <sslConn.h>
#include <tcpConn.h>

// this shouldn't be here but it makes sense for preventing cyclic include
typedef struct {
	pthread_t  request_acceptor;
	Socket     server_socket;
	SSL_CTX   *ssl_context;
	time_t     start_time;
	ThreadPool thread_pool;
} RuntimeInfo;

typedef struct {
	unsigned short tcp_port;
	StringRef      base_dir;
} SNSSettings;

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
 * @param ssl_connection the ssl connection to communicate on
 * @param client_socket the socket of the client
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

/**
 * given the cli arguments set's the server options accordingly
 *
 * @param argc the count of arguments
 * @param argv the values of the arguments
 */
SNSSettings parse_args(const int argc, const char *argv[]);

/**
 * Setup the server, loads libraries and stuff
 *
 * to call one
 */
void setup(SNSSettings args, RuntimeInfo *res);

/**
 * stop the server and its threads
 */
void stop(RuntimeInfo *rti);

/**
 * stop and immediatly starts the server again
 */
void restart(SNSSettings ca, RuntimeInfo *rti);

/**
 * starts the server main thread
 */
void start(RuntimeInfo *rti);
