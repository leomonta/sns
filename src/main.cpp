#include "main.hpp"

#include "logger.h"
#include "profiler.hpp"
#include "utils.hpp"

#include <csignal>
#include <cstdio>

int main(const int argc, const char *argv[]) {

#ifdef DEBUG
	Instrumentor::Get().BeginSession("Leonard server");
#endif

	signal(SIGPIPE, &SIGPIPE_handler);

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

#ifdef DEBUG
	Instrumentor::Get().EndSession();
#endif

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

	cliArgs res = {
	    443, {nullptr, 0}
    };

	switch (argc) {
	case 3:
		res.tcpPort = static_cast<unsigned short>(std::atoi(argv[2]));
		llog(LOG_DEBUG, "[SERVER] Read port %d from cli args\n", res.tcpPort);
		[[fallthrough]];

	case 2:
		res.baseDir = {argv[1], strlen(argv[1])};
		llog(LOG_DEBUG, "[SERVER] Read directory %s from cli args\n", argv[1]);
		break;
	case 1:
		llog(LOG_FATAL, "[SERVER] No base directory given, exiting\n");
		exit(1);
	}

	return res;
}
