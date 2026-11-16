#pragma once

#include "MiniMap_StringRef_MessageProcessor.h"
#include "threadpool.h"

// this shouldn't be here but it makes sense for preventing cyclic include
typedef struct {
	SSL_CTX   *ssl_context;
	ThreadPool thread_pool;
	pthread_t  request_acceptor;
	time_t     start_time;
	Socket     server_socket;
} ServerInfo;

typedef struct {
	StringRef      base_dir;
	unsigned short tcp_port;
} SNSSettings;

typedef struct {
	MiniMap_StringRef_MessageProcessor mapped_functions;
	SNSSettings                        settings;
	ServerInfo                         info;
} SNSServer;

/**
 * asks the `SNSserver` to call `fun` when any request on `route_name` is made
 *
 * @param[in] `route_name` the plain name of the route to intercept,
 * 	nullptr for any route that is not already matched by another mapping
 * 	the route_name string is limited to 255 characters
 * @param[in] `fun` the function to be called to manage the http reqeust,
 * 	nullptr to remove the mapping
 * @oaram[in] `SNSServer` the server where to apply the mapping
 *
 * @return an error code describing if and which error occurred
 *
 */
ERRFUNC register_route(const StringRef *route_name, const MessageProcessor *fun, SNSServer *server);

/**
 * sets up and starts the server and the worker threads
 * after this the server will accept requests and will respond
 *
 * @param[in] `server` the sns server to set up and start
 *
 * @return an error code describing if and which error occurred
 *
 * - `THREADPOOL__FAILED_INIT`. The threadpool initialization failed
 *     Nothing was constructed and nothing needs to be destroyed
 */
ERRFUNC SNSstart(SNSServer *server);

/**
 * stops and then restarts the server
 *
 * @param[in] `server` the sns server to restart
 *
 * @return an error code describing if and which error occurred
 *
 * - `THREADPOOL__FAILED_INIT`. The threadpool initialization failed
 *     Nothing was constructed and nothing needs to be destroyed
 *
 */
ERRFUNC SNSrestart(SNSServer *server);

/**
 * stops the seerver and kill all the threads 
 *
 * @param[in] `server` the server to kill
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC SNSstop(SNSServer *server);
