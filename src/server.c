#include "server.h"

#include "StringRef.h"
#include "logger.h"
#include "methods.h"
#include "threadpool.h"

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <sslConn.h>
#include <stdlib.h>
#include <sys/types.h>
#include <tcpConn.h>

// only for logging purposes
const char *method_str[] = {
    "INVALID",
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "OPTIONS",
    "CONNECT",
    "TRACE",
    "PATCH",
};

void SIGPIPE_handler(int os) {
	llog(LOG_FATAL, "[SIGPIPE] Received a sigpipe %d\n", os);
}

#ifndef NO_THREADING
[[noreturn]]
#endif
void *proxy_acc_req(void *ptr) {
	accept_requests((RuntimeInfo *)(ptr));
#ifdef NO_THREADING
	return nullptr;
#else
	pthread_exit(NULL);
#endif
}

#ifndef NO_THREADING
[[noreturn]]
#endif
void *proxy_res_req(void *ptr) {
	ThreadPool *pool = (ThreadPool *)(ptr);

	while (!pool->stop) {
		auto data = dequeue_threadpool(pool);

		// dequeue might return a null value for how it is implemented, i deal with that
		if (data.ssl == nullptr) {
			llog(LOG_DEBUG, "[THREAD] DEQUEUE return null. Probably the threadpool is commiting fake data to kill all threads\n");
			continue;
		}

		resolve_request(data.ssl, data.client_socket);
	}

#ifdef NO_THREADING
	return nullptr;
#else
	pthread_exit(NULL);
#endif
}

void accept_requests(RuntimeInfo *rti) {
	Socket client = INVALID_SOCKET;

	struct pollfd ss = {
	    .fd      = rti->server_socket,
	    .events  = POLLIN,
	    .revents = 0,
	};

	// Receive until the peer shuts down the connection
	while (!rti->thread_pool.stop) {

		// infinetly wait for the socket to become usable
		poll(&ss, 1, -1);

		client = TCP_accept_client(rti->server_socket);

		if (client == -1) {
			// no client tried to connect
			continue;
		}

		auto ssl_connection = SSL_create_connection(rti->ssl_context, client);

		if (ssl_connection == nullptr) {
			continue;
		}

		auto err = SSL_accept_client(ssl_connection);

		if (err == -1) {
			continue;
		}

		llog(LOG_DEBUG, "[SERVER] Launched request resolver for socket %d\n", client);

		ResolverData t_data = {ssl_connection, client};

#ifdef NO_THREADING
		proxy_resReq(t_data);
#else
		enqueue_threadpool(&rti->thread_pool, &t_data);
#endif
	}
}

void resolve_request(SSL *ssl_connection, const Socket client_socket) {

	// ---------------------------------------------------------------------- RECEIVE
	char *request        = nullptr;
	auto  bytes_received = SSL_receive_record(ssl_connection, &request);

	// received some bytes
	if (bytes_received > 0) {

		InboundHttpMessage mex = parse_InboundMessage(request);
		llog(LOG_INFO, "[SERVER] Received request <%s> \n", method_str[mex.method]);

		OutboundHttpMessage response = {};
		response.header_options      = MiniMap_u_char_StringOwn_make(16);

		switch (mex.method) {
		case HTTP_HEAD:
			Head(&mex, &response);
			break;

		case HTTP_GET:
			Get(&mex, &response);
			break;
		}

		// make the message a single formatted string
		auto res = compose_message(&response);
		// llog(LOG_DEBUG, "[SERVER] Message compiled -> \n%*s\n", res.len, res.str);

		// ------------------------------------------------------------------ SEND
		// acknowledge the segment back to the sender
		SSL_send_record(ssl_connection, res.str, res.len);

		destroy_OutboundHttpMessage(&response);
		destroy_InboundHttpMessage(&mex);
		free(res.str);
	}

	TCP_shutdown_socket(client_socket);
	TCP_close_socket(client_socket);
	SSL_destroy_connection(ssl_connection);
}

void setup(SNSSettings settings, RuntimeInfo *res) {

	// initializing methods data
	setup_methods(&settings.base_dir);

	errno = 0;

	// initializing the tcp Server
	res->server_socket = TCP_initialize_server(settings.tcp_port, 4);

	if (res->server_socket == INVALID_SOCKET) {
		exit(1);
	}

	llog(LOG_INFO, "[SERVER] Listening on %*s:%d\n", (int)settings.base_dir.len, settings.base_dir.str, settings.tcp_port);

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
	llog(LOG_INFO, "[SERVER] Sent stop signal to all threads\n");

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

void restart(SNSSettings settings, RuntimeInfo *rti) {

	stop(rti);
	setup(settings, rti);
	start(rti);
}
