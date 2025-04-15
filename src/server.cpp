#include "server.hpp"

#include "methods.hpp"
#include "miniMap.hpp"
#include "stringRef.hpp"
#include "threadpool.hpp"

#include <csignal>
#include <cstdlib>
#include <logger.h>
#include <poll.h>
#include <pthread.h>
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

		client = TCPacceptClientSock(rti->serverSocket);

		if (client == -1) {
			// no client tried to connect
			continue;
		}

		auto sslConnection = SSLcreateConnection(rti->sslContext, client);

		if (sslConnection == nullptr) {
			continue;
		}

		auto err = SSLacceptClientConnection(sslConnection);

		if (err == -1) {
			continue;
		}

		llog(LOG_DEBUG, "[SERVER] Launched request resolver for socket %d\n", client);

		Methods::resolver_data t_data = {sslConnection, client};

#ifdef NO_THREADING
		proxy_resReq(t_data);
#else
		ThreadPool::enque(&rti->threadPool, &t_data);
#endif
	}
}

void resolveRequest(SSL *sslConnection, const Socket clientSocket) {

	// ---------------------------------------------------------------------- RECEIVE
	char *request       = nullptr;
	auto  bytesReceived = SSLreceiveRecord(sslConnection, &request);

	// received some bytes
	if (bytesReceived > 0) {

		http::inboundHttpMessage mex = http::makeInboundMessage(request);
		llog(LOG_INFO, "[SERVER] Received request <%s> \n", methodStr[mex.m_method]);

		http::outboundHttpMessage response{};
		response.m_headerOptions = miniMap::makeMiniMap<u_char, stringRef>(16);

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
		SSLsendRecord(sslConnection, res.str, res.len);

		http::destroyOutboundHttpMessage(&response);
		http::destroyInboundHttpMessage(&mex);
		free(request);
		free(res.str);
	}

	TCPshutdownSocket(clientSocket);
	TCPcloseSocket(clientSocket);
	SSLdestroyConnection(sslConnection);
}
