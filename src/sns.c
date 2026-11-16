#include "sns.h"

#include "utils.h"

#include <errno.h>
#include <logger.h>
#include <poll.h>
#include <regex.h>
#include <signal.h>

#ifndef NO_THREADING
#	define SNS_NO_RETURN     [[noreturn]]
#	define SNS_THREAD_RETURN pthread_exit(NULL)
#else
#	define SNS_NO_RETURN
#	define SNS_THREAD_RETURN return nullptr;
#endif

static struct sigaction old_sigaction = {};

void SIGPIPE_handler(int os) {
	llog(LOG_INFO, "[SIGPIPE] Received a sigpipe. (%d) Probably just a client disconnect\n", os);
}

MessageProcessor findProcessor(const StringRef *url, const MiniMap_StringRef_MessageProcessor *mapped_function) {

#define INTERNAL_BUFF_SIZE 255

	// static / simple route case
	MessageProcessor vv;
	auto             res = MiniMap_StringRef_MessageProcessor_get(mapped_function, url, &vv);

	if (res) {
		return vv;
	}

	char buff[INTERNAL_BUFF_SIZE + 1] = {0};

	auto sz = mapped_function->keys.count;

	for (size_t i = 0; i < sz; ++i) {
		// take a key value pair
		StringRef kk;
		auto      err = MiniMap_StringRef_MessageProcessor_get_by_index(mapped_function, i, &kk, &vv);
		if (err > NO_OP) {
			continue;
		}

		// regcomp wants a null terminated string
		// But I don't want to allocate an arbitrary str.len + 1 array to ensure that the key is null terminated
		// so I'll limit the length to 255 and use the aforedeclarated `buff`
		size_t safe_size = kk.len > INTERNAL_BUFF_SIZE ? INTERNAL_BUFF_SIZE : kk.len;
		memcpy(buff, kk.str, safe_size);
		buff[safe_size] = '\0'; // since safe_size is always at max the size of the buff, this is always safe

		// build the regex
		regex_t regex;
		auto    code = regcomp(&regex, buff, REG_EXTENDED | REG_NOSUB);
		if (code != 0) {
			regerror(code, &regex, buff, INTERNAL_BUFF_SIZE + 1);
			llog(LOG_WARNING, "[FINDING RESOLVER] Regex compilation failed for pattern '%.*s' failed with error: %s\n", CLAMP_TO_POSITIVE_INT(kk.len), kk.str, buff);
		}

		code = regexec(&regex, url->str, 0, nullptr, 0);

		// I actually care only about if there was a match of not
		regfree(&regex);
		if (code != REG_NOMATCH) {
			return vv;
		}
	}

	llog(LOG_WARNING, "[FINDING RESOLVER] Could not find any matching processor '%.*s'\n", CLAMP_TO_POSITIVE_INT(url->len), url->str);

	res = MiniMap_StringRef_MessageProcessor_get(mapped_function, url, &vv);
	if (res != NO_ERROR) {
		llog(LOG_WARNING, "[FINDING RESOLVER] Going for the default MessageProcessor\n");
		return vv;
	}
	return nullptr;
}

/**
 * Will receive the data from the connection, parse the request and give it to the message processor
 *
 */
void resolve_request(SSL *ssl_connection, const Socket client_socket, MiniMap_StringRef_MessageProcessor *mapped_function) {

	// ---------------------------------------------------------------------- RECEIVE
	char *request        = nullptr;
	auto  bytes_received = SSL_receive_record(ssl_connection, &request);

	// received some bytes
	if (bytes_received <= 0) {
		free(request);
		goto cleanup; // https://thephd.dev/assets/img/2025/06/raw%20goto.jpg
	}

	InboundHttpMessage inbound_mex;
	auto               err = create_InboundMessage(request, &inbound_mex);
	if (err > NO_OP) {
		llog(LOG_ERROR, "[SERVER] Discarding invalid request from client %d\n", client_socket);
		if (err == PARSING_HEADER__MALFORMED_REQUEST || err == FAILED_ALLOCATION) {
			free(request);
		} else {
			err = destroy_InboundHttpMessage(&inbound_mex);
			if (err > NO_OP) {
				llog(LOG_ERROR, "[SERVER] Destroying the inbound message failed, potentially leaking memory (client %d)\n", client_socket);
			}
		}
		goto cleanup;
	}

	const StringView met = methods_str[inbound_mex.method];
	llog(LOG_INFO, "[SERVER] Received request <%.*s> from %d\n", CLAMP_TO_POSITIVE_INT(met.len), met.str, client_socket);

	OutboundHttpMessage outbound_mex = {};
	err                              = MiniMap_HTTPResponseOption_StringOwn_make(16, compare_HTTPResponseOption, &outbound_mex.header_options);
	if (err > NO_OP) {
		err = destroy_InboundHttpMessage(&inbound_mex);
		if (err > NO_OP) {
			llog(LOG_ERROR, "[SERVER] Destroying the inbound message failed, potentially leaking memory (client %d)\n", client_socket);
		}
		goto cleanup;
	}

	outbound_mex.version = HTTP_VER_11;

	MessageProcessor processor = findProcessor(&inbound_mex.url, mapped_function);
	if (processor != nullptr) { // TODO: check after message processor re-work
		processor(&inbound_mex, &outbound_mex);

		// make the message a single formatted string
		StringOwn res = {};
		err           = compose_message(&outbound_mex, &res);
		if (err <= NO_OP) {
			llog(LOG_DEBUG, "[SERVER] Message compiled -> \n%.*s\n", CLAMP_TO_POSITIVE_INT(res.len), res.str);

			// ---------------------------------------------------------------------- SEND
			// acknowledge the segment back to the sender
			SSL_send_record(ssl_connection, res.str, res.len);
			free(res.str);
		}
	}

	err = destroy_OutboundHttpMessage(&outbound_mex);
	if (err > NO_OP) {
		llog(LOG_ERROR, "[SERVER] Destroying the outbound message failed, potentially leaking memory (client %d)\n", client_socket);
	}
	err = destroy_InboundHttpMessage(&inbound_mex);
	if (err > NO_OP) {
		llog(LOG_ERROR, "[SERVER] Destroying the inbound message failed, potentially leaking memory (client %d)\n", client_socket);
	}
cleanup:
	SSL_destroy_connection(ssl_connection);
	TCP_shutdown_socket(client_socket);
	TCP_close_socket(client_socket);
}

SNS_NO_RETURN void *proxy_res_req(void *ptr) {
	ThreadPool *pool = (ThreadPool *)(ptr);

	while (!pool->stop) {
		ResolverData data = {};
		auto         err  = dequeue_threadpool(pool, &data);
		if (err == NO_OP) {
			continue;
		}

		if (data.ssl == nullptr) {
			llog(LOG_DEBUG, "[THREAD] DEQUEUE returned null data. Probably the threadpool is commiting fake data to kill all threads\n");
			continue;
		}

		resolve_request(data.ssl, data.client_socket, data.mapped_function);
	}

	SNS_THREAD_RETURN;
}

/**
 * Listens on the socket for TLS connections
 * if manages to establish one pushes a new job in the thread pool with the details
 */
void accept_requests(SNSServer *server) {
	Socket client = INVALID_SOCKET;

	struct pollfd ss = {
	    .fd      = server->info.server_socket,
	    .events  = POLLIN,
	    .revents = 0,
	};

	// Receive until the peer shuts down the connection
	while (!server->info.thread_pool.stop) {

		// infinetly wait for the socket to become usable
		poll(&ss, 1, -1);

		client = TCP_accept_client(server->info.server_socket);

		if (client == -1) {
			// no client tried to connect
			continue;
		}

		auto ssl_connection = SSL_create_connection(server->info.ssl_context, client);

		if (ssl_connection == nullptr) {
			TCP_shutdown_socket(client);
			TCP_close_socket(client);
			continue;
		}

		auto err = SSL_accept_client(ssl_connection);

		if (err == -1) {
			SSL_destroy_connection(ssl_connection);
			TCP_shutdown_socket(client);
			TCP_close_socket(client);
			continue;
		}

		llog(LOG_DEBUG, "[SERVER] Queued new request for socket %d\n", client);

		ResolverData t_data = {ssl_connection, client, &server->mapped_functions};

#ifdef NO_THREADING
		proxy_res_req((void *)&t_data);
#else
		auto res = enqueue_threadpool(&server->info.thread_pool, &t_data);
		if (res > NO_OP) {
			return;
		}
#endif
	}
}

SNS_NO_RETURN void *proxy_acc_req(void *ptr) {
	accept_requests((SNSServer *)(ptr));
	SNS_THREAD_RETURN;
}

/**
 * Sets up the TCP server and the threadpool
 *
 * @param[in] `server` the sns server to set up
 *
 * - `THREADPOOL__FAILED_INIT`. The threadpool initialization failed
 *     Nothing was constructed and nothing needs to be destroyed
 */
ERRFUNC setup(SNSServer *server) {

	errno = 0;

	struct sigaction action = {
	    .sa_handler = SIGPIPE_handler,
	    .sa_mask    = {},
	    .sa_flags   = 0,
	};

	sigaction(SIGPIPE, &action, &old_sigaction);

	// initializing the tcp Server
	server->info.server_socket = TCP_initialize_server(server->settings.tcp_port, 4);

	if (server->info.server_socket == INVALID_SOCKET) {
		exit(1);
	}

	llog(LOG_INFO, "[SERVER] Listening on %.*s:%d\n", CLAMP_TO_POSITIVE_INT(server->settings.base_dir.len), server->settings.base_dir.str, server->settings.tcp_port);

	// initializing the ssl connection data
	SSL_initialize();
	// TODO: take this cli or config file
	server->info.ssl_context = SSL_create_context("/usr/local/bin/server.crt", "/usr/local/bin/key.pem");

	if (server->info.ssl_context == nullptr) {
		SSL_terminate();
		exit(1);
	}

	llog(LOG_INFO, "[SSL] Context created\n");

	// finally creating the threadPool
	auto err = initialize_threadpool(20, proxy_res_req, &server->info.thread_pool);
	if (err > NO_OP) {
		TCP_terminate(server->info.server_socket);
		SSL_destroy_context(server->info.ssl_context);
		SSL_terminate();
		sigaction(SIGPIPE, &old_sigaction, NULL);

		llog(LOG_INFO, "[SERVER] Stopping server due to thread pool issues\n");

		return THREADPOOL__FAILED_INIT;
	}

	return NO_ERROR;
}

ERRFUNC SNSstart(SNSServer *server) {

	auto err = setup(server);
	RET_IF_ERR(err)

	server->info.thread_pool.stop = false;

#ifdef NO_THREADING
	proxy_acc_req(server);
#else
	pthread_create(&server->info.request_acceptor, NULL, proxy_acc_req, server);
#endif

	llog(LOG_DEBUG, "[SERVER] Request acceptor thread started\n");

	server->info.start_time = time(nullptr);

	return NO_ERROR;
}

ERRFUNC SNSrestart(SNSServer *server) {
	auto err = SNSstop(server);
	RET_IF_ERR(err)
	return SNSstart(server);
}

ERRFUNC SNSstop(SNSServer *server) {

	TCP_terminate(server->info.server_socket);

	// tpool stop
	auto err = destroy_threadpool(&server->info.thread_pool);
	RET_IF_ERR(err) // Does not return an error (yet)
	llog(LOG_INFO, "[SERVER] Sent stop signal to all threads\n");

	// thread stop
	pthread_join(server->info.request_acceptor, NULL);
	llog(LOG_INFO, "[SERVER] Request acceptor stopped\n");

	SSL_destroy_context(server->info.ssl_context);

	SSL_terminate();

	sigaction(SIGPIPE, &old_sigaction, NULL);

	llog(LOG_INFO, "[SERVER] Server stopped\n");

	return NO_ERROR;
}

ERRFUNC register_route(const StringRef *route_name, const MessageProcessor *fun, SNSServer *server) {
	// TODO: still very broken
	return MiniMap_StringRef_MessageProcessor_set(&server->mapped_functions, route_name, fun);
}
