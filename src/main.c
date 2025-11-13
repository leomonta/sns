#include "main.h"

#include "logger.h"
#include "threadpool.h"

#include <errno.h>
#include <sslConn.h>
#include <tcpConn.h>

void setup(SNSSettings args, RuntimeInfo *res) {

	// initializing methods data
	setup_methods(&args.base_dir);

	errno = 0;

	// initializing the tcp Server
	res->server_socket = TCP_initialize_server(args.tcp_port, 4);

	if (res->server_socket == INVALID_SOCKET) {
		exit(1);
	}

	llog(LOG_INFO, "[SERVER] Listening on %*s:%d\n", args.base_dir.len, args.base_dir.str, args.tcp_port);

	// initializing the ssl connection data
	SSL_initialize();
	res->ssl_context = SSL_create_context("/usr/local/bin/server.crt", "/usr/local/bin/key.pem");

	if (res->ssl_context == nullptr) {
		SSL_terminate();
		exit(1);
	}

	llog(LOG_INFO, "[SSL] Context created\n");

	// finally creating the threadPool
	initialize_threadpool(20, &res->thread_pool);

	llog(LOG_INFO, "[THREAD POOL] Started the listenings threads\n");
}

void stop(RuntimeInfo *rti) {

	TCP_terminate(rti->server_socket);

	// tpool stop
	rti->thread_pool.stop = true;
	destroy_threadpool(&rti->thread_pool);
	llog(LOG_INFO, "[SERVER] Sent stop message to all threads\n");

	// thread stop
	pthread_join(rti->request_acceptor, NULL);
	llog(LOG_INFO, "[SERVER] Request acceptor stopped\n");

	SSL_destroy_context(rti->ssl_context);

	SSL_terminate();

	llog(LOG_INFO, "[SERVER] Server stopped\n");
}

void start(RuntimeInfo *rti) {

	rti->thread_pool.stop = false;

#ifdef NO_THREADING
	proxy_accReq(rti);
#else
	pthread_create(&rti->request_acceptor, NULL, proxy_acc_req, rti);
#endif

	llog(LOG_DEBUG, "[SERVER] Request acceptor thread Started\n");

	rti->start_time = time(nullptr);
}

void restart(SNSSettings ca, RuntimeInfo *rti) {

	stop(rti);
	setup_methods(&ca.base_dir);
	start(rti);
}
