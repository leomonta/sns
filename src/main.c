#include "main.h"

#include "logger.h"
#include "threadpool.h"
#include "utils.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sslConn.h>
#include <tcpConn.h>


int main(const int argc, const char *argv[]) {

	signal(SIGPIPE, &SIGPIPE_handler);

	// Get port and directory, maybe
	auto args = parse_args(argc, argv);

	RuntimeInfo run_info;

	// setup the tcp server socket and the ssl context
	setup(args, &run_info);

	// start the requests accpetor secure thread
	start(&run_info);

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
		if (strncmp(line, "exit", 4) == 0 || strncmp(line, "quit", 4) == 0) {
			stop(&run_info);
			break;
		}

		if (strncmp(line, "restart", 7) == 0) {
			restart(args, &run_info);
		}

		if (strncmp(line, "time", 4) == 0) {
			time_t now   = time(nullptr) - run_info.startTime;
			long   days  = now / (60 * 60 * 24);
			long   hours = now / (60 * 60) % 24;
			long   mins  = now / 20 % 60;
			long   secs  = now % 60;

			printf("Time elapsed since the 'start()' method was called is %ld.%02ld:%02ld:%02ld\n", days, hours, mins, secs);
		}
	}

	return 0;
}

void setup(CliArgs args, RuntimeInfo *res) {

	// initializing methods data
	setup_methods(&args.baseDir);

	errno = 0;

	// initializing the tcp Server
	res->serverSocket = TCP_initialize_server(args.tcpPort, 4);

	if (res->serverSocket == INVALID_SOCKET) {
		exit(1);
	}

	llog(LOG_INFO, "[SERVER] Listening on %*s:%d\n", args.baseDir.len, args.baseDir.str, args.tcpPort);

	// initializing the ssl connection data
	SSL_initialize();
	res->sslContext = SSL_create_context("/usr/local/bin/server.crt", "/usr/local/bin/key.pem");

	if (res->sslContext == nullptr) {
		exit(1);
	}

	llog(LOG_INFO, "[SSL] Context created\n");

	// finally creating the threadPool
	initialize_threadpool(20, &res->thread_pool);

	llog(LOG_INFO, "[THREAD POOL] Started the listenings threads\n");
}

void stop(RuntimeInfo *rti) {

	TCP_terminate(rti->serverSocket);

	// tpool stop
	rti->thread_pool.stop = true;
	destroy_threadpool(&rti->thread_pool);
	llog(LOG_INFO, "[SERVER] Sent stop message to all threads\n");

	// thread stop
	pthread_join(rti->requestAcceptor, NULL);
	llog(LOG_INFO, "[SERVER] Request acceptor stopped\n");

	SSL_destroy_context(rti->sslContext);

	SSL_terminate();

	llog(LOG_INFO, "[SERVER] Server stopped\n");
}

void start(RuntimeInfo *rti) {

	rti->thread_pool.stop = false;

#ifdef NO_THREADING
	proxy_accReq(rti);
#else
	pthread_create(&rti->requestAcceptor, NULL, proxy_acc_req, rti);
#endif

	llog(LOG_DEBUG, "[SERVER] Request acceptor thread Started\n");

	rti->startTime = time(nullptr);
}

void restart(CliArgs ca, RuntimeInfo *rti) {

	stop(rti);
	setup_methods(&ca.baseDir);
	start(rti);
}

CliArgs parse_args(const int argc, const char *argv[]) {

	// server directory port
	//    0       1      2

	CliArgs args = {
	    443,
	    {nullptr, 0}
    };

	switch (argc) {
	case 3:
		args.tcpPort = (unsigned short)(atoi(argv[2]));
		llog(LOG_DEBUG, "[CLI] Read port %d from cli args\n", args.tcpPort);
		[[fallthrough]];

	case 2:
		args.baseDir = (StringRef) {argv[1], strlen(argv[1])};
		llog(LOG_DEBUG, "[CLI] Read directory %s from cli args\n", argv[1]);
		break;
	case 1:
		llog(LOG_FATAL, "[CLI] No base directory given, exiting\n");
		exit(1);
	}

	llog(LOG_DEBUG, "[CLI] Successfully parsed cli args\n");

	return args;
}
