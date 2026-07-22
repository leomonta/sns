#pragma once

#include "MiniMap_StringRef_MessageProcessor.h"
#include "ResolverData.h"
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
 * @param[in] `fun` the function to be called to manage the http reqeust,
 * 	nullptr to remove the mapping
 * @oaram[in] `SNSServer` the server where to apply the mapping
 *
 */
void register_route(const StringRef *route_name, const MessageProcessor *fun, SNSServer *server);

void SNSstart(SNSServer *server);

void SNSstop(SNSServer *server);

void SNSrestart(SNSServer *server);
