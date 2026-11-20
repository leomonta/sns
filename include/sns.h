#pragma once

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
	SNSRoutes   routes;
	SNSSettings settings;
	ServerInfo  info;
} SNSServer;

/**
 * creates the internal structure needed
 *
 * @param[in] `server` the server to create
 *
 * @return an error code describing if and which error occurred
 * - `FAILED_ALLOCATION`: the MiniVector could not be allocated, the structure was not constructed and should not be used nor destryed
 */
ERRFUNC create_SNSServer(SNSServer *server);

/**
 * destroys and free all the remaining data stored by the server
 *
 * @param[in] `server` the server to destroy
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC destroy_SNSServer(SNSServer *server);

/**
 * asks the `SNSserver` to call `fun` when any request on `route_name` is made
 * the server will not take ownership of any of the `route_name`s passed, these should be managed by the user or (more commonly) be compile time constants
 *
 * @param[in] `route_name` the plain name of the route to intercept,
 * 	passing `nullptr` specifies the fallback if no other route matched the url the client requested, in this case `regex` is ignored
 * 	the `route_name` string is limited to 255 characters
 * @param[in] `fun` the function to be called to manage the http reqeust,
 * @param[in] `regex` wheather the route specified has to be interpreted liretally or as a regex
 * @oaram[in] `SNSServer` the server where to apply the mapping
 *
 * @return an error code describing if and which error occurred
 *  - `INVALID_ARGUMENT`: fun cannot be null.
 *      No route registered
 *  - `FAILED_ALLOCATION`: Could not reserve space to store the given data
 *      No route registered
 *  - `INVALID_REGEX`: The regex passes could not be compiled by `regcomp(..., ..., REG_EXTENDED | REG_NOSUB)`
 *      No route registered
 */
ERRFUNC register_route(const StringView *route_name, const MessageProcessor proc, const bool regex, SNSServer *server);

/**
 * Removes the registered MessageProcessor for the specified route_name
 * If `route_name` is `nullptr` then it removes the fallback processor
 *
 * @param[in] `route_name` the route to unregistred, if `nullptr` then it removes the fallback processor
 * @param[in] `server` the server from which to unregister the route
 * 
 * @return an error code describing if and which error occurred
 *  - `NO_OP` the route specified does not exist
 */
ERRFUNC unregister_route(const StringView *route_name, SNSServer *server);

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
