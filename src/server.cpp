#include "server.hpp"

#include "methods.hpp"
#include "miniMap.hpp"
#include "profiler.hpp"
#include "stringRef.hpp"
#include "threadpool.hpp"
#include "utils.hpp"

#include <csignal>
#include <cstdlib>
#include <logger.h>
#include <poll.h>
#include <pthread.h>
#include <sys/types.h>

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

void Panico(int cos) {
	printf("Signal %d received %s \n", cos, strerror(errno));
	exit(1);
}

namespace Res {

	Socket             serverSocket;
	SSL_CTX           *sslContext = nullptr;
	ThreadPool::tpool *threadPool;
} // namespace Res

int main(const int argc, const char *argv[]) {

	Instrumentor::Get().BeginSession("Leonard server");

	signal(SIGPIPE, Panico);

	// Get port and directory, maybe
	auto args = parseArgs(argc, argv);

	// setup the tcp server socket and the ssl context
	auto runInfo = setup(args);

	// start the requests accpetor secure thread
	start(&runInfo);

	// line that will be read from stdin
	char line[256];

	// cli like interface
	while (true) {
		if (fgets(line, sizeof(line), stdin) == nullptr) {
			continue;
		}

		// line is a valid value

		trimwhitespace(line);

		// simple commands
		if (streq_str(line, "exit") || streq_str(line, "quit")) {
			stop(&runInfo);
			break;
		}

		if (streq_str(line, "restart")) {
			restart(args, &runInfo);
		}

		if (streq_str(line, "time")) {
			time_t now   = time(nullptr) - runInfo.startTime;
			long   days  = now / (60 * 60 * 24);
			long   hours = now / (60 * 60) % 24;
			long   mins  = now / 20 % 60;
			long   secs  = now % 60;

			printf("Time elapsed since the 'start()' method was called is %ld.%02ld:%02ld:%02ld\n", days, hours, mins, secs);
		}
	}

	Instrumentor::Get().EndSession();

	return 0;
}

runtimeInfo setup(cliArgs args) {

	PROFILE_FUNCTION();

	// initializing methods data
	Methods::setupContentTypes();
	Methods::setupBaseDir(args.baseDir);

	errno = 0;

	runtimeInfo res;

	// initializing the tcp Server
	res.serverSocket = TCPinitializeServer(args.tcpPort, 4);

	if (res.serverSocket == INVALID_SOCKET) {
		exit(1);
	}

	llog(LOG_INFO, "[SERVER] Listening on %*s:%d\n", args.baseDir.len, args.baseDir.str, args.tcpPort);

	// initializing the ssl connection data
	SSLinitializeServer();
	res.sslContext = SSLcreateContext("/usr/local/bin/server.crt", "/usr/local/bin/key.pem");

	if (res.sslContext == nullptr) {
		exit(1);
	}

	llog(LOG_INFO, "[SSL] Context created\n");

	// finally creating the threadPool
	res.threadPool = ThreadPool::create(std::thread::hardware_concurrency());

	llog(LOG_INFO, "[THREAD POOL] Started the listenings threads\n");

	return res;
}

void stop(runtimeInfo *rti) {

	PROFILE_FUNCTION();

	TCPterminate(rti->serverSocket);

	rti->threadPool->stop = true;
	ThreadPool::destroy(rti->threadPool);
	llog(LOG_INFO, "[SERVER] Sent stop message to all threads\n");

	pthread_join(rti->requestAcceptor, NULL);
	llog(LOG_INFO, "[SERVER] Request acceptor stopped\n");

	SSLdestroyContext(rti->sslContext);

	SSLterminateServer();

	llog(LOG_INFO, "[SERVER] Server stopped\n");
}

void start(runtimeInfo *rti) {

	PROFILE_FUNCTION();

	rti->threadPool->stop = false;

#ifdef NO_THREADING
	proxy_accReq(rti);
#else
	pthread_create(&rti->requestAcceptor, NULL, proxy_accReq, rti);
#endif

	llog(LOG_DEBUG, "[SERVER] ReqeustAcceptorSecure thread Started\n");

	rti->startTime = time(nullptr);
}

void restart(cliArgs ca, runtimeInfo *rti) {

	PROFILE_FUNCTION();
	stop(rti);
	setup(ca);
	start(rti);
}

cliArgs parseArgs(const int argc, const char *argv[]) {

	PROFILE_FUNCTION();
	// server directory port
	//    0       1      2

	cliArgs res = {443, {nullptr, 0}};

	switch (argc) {
	case 3:
		res.tcpPort = static_cast<unsigned short>(std::atoi(argv[2]));
		llog(LOG_DEBUG, "[SERVER] Read port %d from cli args\n", res.tcpPort);
		[[fallthrough]];

	case 2:
		res.baseDir =  {argv[1], strlen(argv[1])};
		llog(LOG_DEBUG, "[SERVER] Read directory %s from cli args\n", argv[1]);
		break;
	case 1:
		llog(LOG_FATAL, "[SERVER] No base directory given, exiting\n");
		exit(1);
	}

	return res;
}

void *proxy_accReq(void *ptr) {
	acceptRequestsSecure((runtimeInfo *)(ptr));
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

		resolveRequestSecure(data.ssl, data.clientSocket);
	}

#ifdef NO_THREADING
	return nullptr;
#else
	pthread_exit(NULL);
#endif
}

void acceptRequestsSecure(runtimeInfo *rti) {
	Socket client = -1;

	pollfd ss = {
	    .fd      = rti->serverSocket,
	    .events  = POLLIN,
	    .revents = 0,
	};

	// Receive until the peer shuts down the connection
	// ! and * are at the same precedence but they right-to-left associated so this works
	while (!rti->threadPool->stop) {

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
		ThreadPool::enque(rti->threadPool, &t_data);
#endif
	}
}

void resolveRequestSecure(SSL *sslConnection, const Socket clientSocket) {

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
