#include "server.hpp"

#include "logger.h"
#include "methods.hpp"
#include "miniMap.hpp"
#include "stringRef.hpp"
#include "threadpool.hpp"

#include <csignal>
#include <cstdlib>
#include <poll.h>
#include <pthread.h>
#include <sslConn.h>
#include <sys/types.h>
#include <tcpConn.h>

const char *methodStr[] = {
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
	llog(LOG_FATAL, "[SIGPIPE] Received a sigpipe %s\n", os);
}

void *proxy_accReq(void *ptr) {
	acceptRequests((runtimeInfo *)(ptr));
#ifdef NO_THREADING
	return nullptr;
#else
	pthread_exit(NULL);
#endif
}

void *proxy_resReq(void *ptr) {
	ThreadPool::tpool *pool = (ThreadPool::tpool *)(ptr);

	while (!pool->stop) {
		auto data = ThreadPool::dequeue(pool);

		// dequeue might return a null value for how it is implemented, i deal with that
		if (data.ssl == nullptr) {
			llog(LOG_DEBUG, "[THREAD] DEQUEUE return null. Probably the threadpool is commiting fake data to kill all threads\n");
			continue;
		}

		resolveRequest(data.ssl, data.clientSocket);
	}

#ifdef NO_THREADING
	return nullptr;
#else
	pthread_exit(NULL);
#endif
}

void acceptRequests(runtimeInfo *rti) {
	Socket client = INVALID_SOCKET;

	pollfd ss = {
	    .fd      = rti->serverSocket,
	    .events  = POLLIN,
	    .revents = 0,
	};

	// Receive until the peer shuts down the connection
	while (!rti->threadPool.stop) {

		// infinetly wait for the socket to become usable
		poll(&ss, 1, -1);

		client = TCP_accept_client(rti->serverSocket);

		if (client == -1) {
			// no client tried to connect
			continue;
		}

		auto sslConnection = SSL_create_connection(rti->sslContext, client);

		if (sslConnection == nullptr) {
			continue;
		}

		auto err = SSL_accept_client(sslConnection);

		if (err == -1) {
			continue;
		}

		llog(LOG_DEBUG, "[SERVER] Launched request resolver for socket %d\n", client);

		Methods::resolver_data t_data = {sslConnection, client};

#ifdef NO_THREADING
		proxy_resReq(t_data);
#else
		ThreadPool::enqueue(&rti->threadPool, &t_data);
#endif
	}
}

void resolveRequest(SSL *sslConnection, const Socket clientSocket) {

	// ---------------------------------------------------------------------- RECEIVE
	char *request       = nullptr;
	auto  bytesReceived = SSL_receive_record(sslConnection, &request);

	// received some bytes
	if (bytesReceived > 0) {

		http::inboundHttpMessage mex = http::makeInboundMessage(request);
		llog(LOG_INFO, "[SERVER] Received request <%s> \n", methodStr[mex.m_method]);

		http::outboundHttpMessage response{};
		response.m_headerOptions = miniMap::makeMiniMap<u_char, stringOwn>(16);

		switch (mex.m_method) {
		case http::HTTP_HEAD:
			Methods::Head(mex, response);
			break;

		case http::HTTP_GET:
			Methods::Get(mex, response);
			break;
		}

		// make the message a single formatted string
		auto res = http::compileMessage(response);
		// llog(LOG_DEBUG, "[SERVER] Message compiled -> \n%s\n", res;

		// ------------------------------------------------------------------ SEND
		// acknowledge the segment back to the sender
		SSL_send_record(sslConnection, res.str, res.len);

		http::destroyOutboundHttpMessage(&response);
		http::destroyInboundHttpMessage(&mex);
		free(request);
		free(res.str);
	}

	TCP_shutdown_socket(clientSocket);
	TCP_close_socket(clientSocket);
	SSL_destroy_connection(sslConnection);
}
