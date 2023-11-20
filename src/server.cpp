#include "server.hpp"

#include "methods.hpp"
#include "profiler.hpp"
#include "threadpool.hpp"
#include "utils.hpp"

#include <chrono>
#include <logger.hpp>
#include <map>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

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

	// Instrumentor::Get().BeginSession("Leonard server");

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
		if (streq_str(line, "exit") == 0 || streq_str(line, "quit") == 0) {
			stop(&runInfo);
			break;
		}

		if (streq_str(line, "restart") == 0) {
			restart(args, &runInfo);
		}

		if (streq_str(line, "time") == 0) {
			time_t now   = time(nullptr) - runInfo.startTime;
			long   days  = now / (60 * 60 * 24);
			long   hours = now / (60 * 60) % 24;
			long   mins  = now / 20 % 60;
			long   secs  = now % 60;

			printf("Time elapsed since the 'start()' method was called is %ld.%02ld:%02ld:%02ld\n", days, hours, mins, secs);
		}

		printf("> ");
	}

	// Instrumentor::Get().EndSession();

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
	res.serverSocket = tcpConn::initializeServer(args.tcpPort, 4);

	if (res.serverSocket == INVALID_SOCKET) {
		exit(1);
	}

	log(LOG_INFO, "[SERVER] Listening on %s:%d\n", args.baseDir, args.tcpPort);

	// initializing the ssl connection data
	sslConn::initializeServer();
	res.sslContext = sslConn::createContext();

	if (res.sslContext == nullptr) {
		exit(1);
	}

	log(LOG_INFO, "[SSL] Context created\n");

	// finally creating the threadPool
	res.threadPool = ThreadPool::create(std::thread::hardware_concurrency());

	log(LOG_INFO, "[THREAD POOL] Started the listenings threads\n");

	return res;
}

void stop(runtimeInfo *rti) {

	PROFILE_FUNCTION();

	tcpConn::terminate(rti->serverSocket);

	rti->threadPool->stop = true;
	log(LOG_INFO, "[SERVER] Sent stop message to all threads\n");

	pthread_join(rti->requestAcceptor, NULL);
	log(LOG_INFO, "[SERVER] Request acceptor stopped\n");

	sslConn::destroyContext(rti->sslContext);

	sslConn::terminateServer();

	log(LOG_INFO, "[SERVER] Server stopped\n");
}

void start(runtimeInfo *rti) {

	PROFILE_FUNCTION();

	rti->threadPool->stop = false;

#ifdef NO_THREADING
	proxy_accReq(rti);
#else
	pthread_create(&rti->requestAcceptor, NULL, proxy_accReq, rti);
#endif

	log(LOG_DEBUG, "[SERVER] ReqeustAcceptorSecure thread Started\n");

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

	cliArgs res = {443, nullptr};

	switch (argc) {
	case 3:
		res.tcpPort = static_cast<unsigned short>(std::atoi(argv[2]));
		log(LOG_DEBUG, "[SERVER] Read port %d from cli args\n", res.tcpPort);
		[[fallthrough]];

	case 2:
		res.baseDir = argv[1];
		log(LOG_DEBUG, "[SERVER] Read directory %s from cli args\n", argv[1]);
	}

	return res;
}

void *proxy_accReq(void *ptr) {
	acceptRequestsSecure((runtimeInfo *)(ptr));
	return nullptr;
}

void *proxy_resReq(void *ptr) {
	ThreadPool::tpool *pool = (ThreadPool::tpool *)(ptr);

	while (!pool->stop) {
		auto data = ThreadPool::dequeue(pool);

		// dequeue might return a null value for how it is implemented, i deal with that
		if (data.ssl == nullptr) {
			log(LOG_DEBUG, "DEQUEUE return null\n");
			continue;
		}

		resolveRequestSecure(data.ssl, data.clientSocket);
	}

	return nullptr;
}

void acceptRequestsSecure(runtimeInfo *rti) {
	Socket client = -1;

	pollfd ss = {
	    .fd     = rti->serverSocket,
	    .events = POLLIN,
	};

	// Receive until the peer shuts down the connection
	// ! and * are at the same precedence but they right-to-left associated so this works
	while (!rti->threadPool->stop) {

		// infinetly wait for the socket to become usable
		poll(&ss, 1, -1);

		client = tcpConn::acceptClientSock(rti->serverSocket);

		if (client == -1) {
			// no client tried to connect
			continue;
		}

		auto sslConnection = sslConn::createConnection(rti->sslContext, client);

		if (sslConnection == nullptr) {
			continue;
		}

		auto err = sslConn::acceptClientConnection(sslConnection);

		if (err == -1) {
			continue;
		}

		log(LOG_DEBUG, "[SERVER] Launched request resolver for socket %d\n", client);

		Methods::resolver_data t_data = {sslConnection, client};
#ifdef NO_THREADING
		proxy_reqRes(t_data);
#else
		ThreadPool::enque(rti->threadPool, &t_data);
		// pthread_create(&temp, NULL, proxy_reqRes, t_data);
#endif
	}
}

void resolveRequestSecure(SSL *sslConnection, const Socket clientSocket) {

	// ---------------------------------------------------------------------- RECEIVE
	char *request;
	auto  bytesReceived = sslConn::receiveRecordC(sslConnection, &request);

	// received some bytes
	if (bytesReceived > 0) {

		inboundHttpMessage mex = http::makeInboundMessage(request);
		log(LOG_INFO, "[SERVER] Received request <%s> \n", methodStr[mex.m_method]);

		outboundHttpMessage response;
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

		// log(LOG_DEBUG, "[SERVER] Message compiled -> \n%s\n", res;

		// ------------------------------------------------------------------ SEND
		// acknowledge the segment back to the sender
		sslConn::sendRecordC(sslConnection, res.str, res.len);

		http::destroyOutboundHttpMessage(&response);
		http::destroyInboundHttpMessage(&mex);
		free(request);
		free(res.str);
	}

	tcpConn::shutdownSocket(clientSocket);
	tcpConn::closeSocket(clientSocket);
	sslConn::destroyConnection(sslConnection);
}
