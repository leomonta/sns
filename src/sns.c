#include "sns.h"

#include "responses.h"
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

//                                                                111 1 1111112222222222 3 33333333344444444445555555555666666 6 666777777777777777 7 7
//                                                      0123456789012 3 4567890123456789 0 12345678901234567890123456789012345 6 789012345678901234 5 6
static char   RESPONSE_GENERIC_INTERNAL_SERVER_ERROR[]   = "HTTP/1.1 xxx \r\nServer: sns/5.0\r\nDate: Thu, 01 Jan 1997 00:00:00 GMT\r\nContent-Length: 0\r\n\r\n";
static size_t RESPONSE_GENERIC_INTERNAL_SERVER_ERROR_LEN = sizeof(RESPONSE_GENERIC_INTERNAL_SERVER_ERROR) - 1;

static void SIGPIPE_handler(int os) {
	llog(LOG_INFO, "[SIGPIPE] Received a sigpipe. (%d) Probably just a client disconnect\n", os);
}

static MessageProcessor find_processor(const StringView *url, const SNSRoutes *routes_info) {

	regex_t empty_reg = {};

	// plain routes

	for (size_t i = 0; i < routes_info->names.count; ++i) {
		regex_t regex = {};
		auto    ign   = MiniVector_regex_t_get(&routes_info->regexes, i, &regex);

		// if the route is regex, skip this
		if (memcmp(&regex, &empty_reg, sizeof(regex_t)) != 0) {
			continue;
		}
		StringView name = {};
		ign             = MiniVector_StringView_get(&routes_info->names, i, &name);

		// just return the first that is found
		if (equal_StringView(url, &name)) {
			MessageProcessor proc = {};
			ign                   = MiniVector_MessageProcessor_get(&routes_info->processors, i, &proc);
			return proc;
		}
	}

	// regexes

	for (size_t i = 0; i < routes_info->names.count; ++i) {
		regex_t regex = {};
		auto    ign   = MiniVector_regex_t_get(&routes_info->regexes, i, &regex);

		// if the route is not regex, skip this
		if (memcmp(&regex, &empty_reg, sizeof(regex_t)) == 0) {
			continue;
		}

		regmatch_t match_conf = {
		    .rm_so = 0,
		    .rm_eo = clamp_to_positive_int(url->len), // technically truncating the legnth here but if someone is going to send me a request with a url 2^32 char long (4GiB) we have other problems
		};
		auto code = regexec(&regex, url->str, 0, &match_conf, REG_STARTEND);

		if (code != REG_NOMATCH) {
			MessageProcessor proc = {};
			ign                   = MiniVector_MessageProcessor_get(&routes_info->processors, i, &proc);
			return proc;
		}
	}

	llog(LOG_WARNING, "[FINDING RESOLVER] Could not find any matching processor for url '%.*s', using the default one\n", clamp_to_positive_int(url->len), url->str);
	return routes_info->default_processor;
}

static void send500(SSL *ssl_connection) {

	auto res = RESPONSE_GENERIC_INTERNAL_SERVER_ERROR;
	res[9]   = '5';
	res[10]  = '0';
	res[11]  = '0';

	auto utc = getUTC();

	memcpy(&res[38], utc.str, utc.len);

	SSL_send_record(ssl_connection, res, RESPONSE_GENERIC_INTERNAL_SERVER_ERROR_LEN);
}

/**
 * Will receive the data from the connection, parse the request and give it to the message processor
 *
 */
static void resolve_request(SSL *ssl_connection, const Socket client_socket, const SNSRoutes *routes_info) {

	// ---------------------------------------------------------------------- RECEIVE
	char              *request        = nullptr;
	auto               bytes_received = SSL_receive_record(ssl_connection, &request);
	MessageProcessor   processor      = nullptr; // this is kept here to so an erlier error can set it to a default known value (like 400 Bad Request)
	InboundHttpMessage inbound_mex    = {};
	// received some bytes
	if (bytes_received <= 0) {
		free(request);
		goto cleanup; // https://thephd.dev/assets/img/2025/06/raw%20goto.jpg
	}

	auto err = create_InboundHttpMessage(request, &inbound_mex);
	if (err > NO_OP) {

		if (err == FAILED_ALLOCATION) {
			send500(ssl_connection);
			goto cleanup;
		}

		llog(LOG_WARNING, "[SERVER] Client made an invalid request, responding with <400 Bad Request> \n");

		processor = BadRequest400;
	}

	if (processor == nullptr) { // nothing else did set a responder already
		const StringView met = methods_str[inbound_mex.method];
		llog(LOG_INFO, "[SERVER] Received request <%.*s> from %d\n", clamp_to_positive_int(met.len), met.str, client_socket);

		StringView url_v = {inbound_mex.url.str, inbound_mex.url.len};
		processor        = find_processor(&url_v, routes_info);
	}
	if (processor != nullptr) { // TODO: check after message processor re-work

		OutboundHttpMessage outbound_mex = {};
		err                              = create_OutboundHttpMessage(&outbound_mex);
		if (err > NO_OP) {
			send500(ssl_connection);
			goto cleanup;
		}

		outbound_mex.version     = HTTP_VER_11;
		outbound_mex.status_code = 204;
		err                      = add_header_option(RP_DATE, getUTC(), &outbound_mex);

		if (err > NO_OP) {
			err = destroy_OutboundHttpMessage(&outbound_mex);
			// ignoring error, since it does not return one yet
			send500(ssl_connection);
			goto cleanup;
		}

		err = add_header_option(RP_SERVER, TO_STRINGVIEW(SNS_VERSION_STRING), &outbound_mex);

		if (err > NO_OP) {
			err = destroy_OutboundHttpMessage(&outbound_mex);
			// ignoring error, since it does not return one yet
			send500(ssl_connection);
			goto cleanup;
		}

		err = add_header_option(RP_CONTENT_LENGTH, TO_STRINGVIEW("0"), &outbound_mex);

		if (err > NO_OP) {
			err = destroy_OutboundHttpMessage(&outbound_mex);
			// ignoring error, since it does not return one yet
			send500(ssl_connection);
			goto cleanup;
		}

		// if the `create_InboundHttpMessage` faile inbound_mex is not properly constructed, but this is not a problem because
		// in that case `processor` is a known one that does not use the in mex,
		err = processor(&inbound_mex, &outbound_mex);

		if (err > NO_OP) {
			err = destroy_OutboundHttpMessage(&outbound_mex);
			// ignoring error, since it does not return one yet
			send500(ssl_connection);
			goto cleanup;
		}

		// make the message a single formatted string
		StringOwn res = {};
		err           = compose_message(&outbound_mex, &res);
		if (err <= NO_OP) {
			llog(LOG_DEBUG, "[SERVER] Message compiled -> \n%.*s\n", clamp_to_positive_int(res.len), res.str);

			// ---------------------------------------------------------------------- SEND
			// acknowledge the segment back to the sender
			SSL_send_record(ssl_connection, res.str, res.len);
			free(res.str);
		} else {
			send500(ssl_connection);
		}

		err = destroy_OutboundHttpMessage(&outbound_mex);
		// ignoring error, since it does not return one yet
	}
cleanup:
	err = destroy_InboundHttpMessage(&inbound_mex);
	// ignoring error, since it does not return one yet
	SSL_destroy_connection(ssl_connection);
	TCP_shutdown_socket(client_socket);
	TCP_close_socket(client_socket);
}

SNS_NO_RETURN static void *proxy_res_req(void *ptr) {

#ifdef NO_THREADING
	ResolverData *data = (ResolverData *)(ptr);
	resolve_request(data->ssl, data->client_socket, data->routes_info);
#else
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

		resolve_request(data.ssl, data.client_socket, data.routes_info);
	}

#endif
	SNS_THREAD_RETURN;
}

/**
 * Listens on the socket for TLS connections
 * if manages to establish one pushes a new job in the thread pool with the details
 */
static void accept_requests(SNSServer *server) {
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

		if (ss.revents == POLLNVAL) {
			// we closed the server socket
			break;
		}

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

		ResolverData t_data = {ssl_connection, client, &server->routes};

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

SNS_NO_RETURN static void *proxy_acc_req(void *ptr) {
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
[[nodiscard]] static ErrCode setup(SNSServer *server) {

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

	llog(LOG_INFO, "[SERVER] Listening on %.*s:%d\n", clamp_to_positive_int(server->settings.base_dir.len), server->settings.base_dir.str, server->settings.tcp_port);

	// initializing the ssl connection data
	SSL_initialize();
	// TODO: take this cli or config file
	server->info.ssl_context = SSL_create_context("/usr/local/bin/server.crt", "/usr/local/bin/key.pem");

	if (server->info.ssl_context == nullptr) {
		SSL_terminate();
		return SSL_INIT_FAILED;
	}

	llog(LOG_INFO, "[SSL] Context created\n");

#ifndef NO_THREADING
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
#endif

	return NO_ERROR;
}

ERRFUNC SNSstart(SNSServer *server) {

	auto err = setup(server);
	SNS_RET_IF_ERR(err)

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
	SNS_RET_IF_ERR(err)
	return SNSstart(server);
}

ERRFUNC SNSstop(SNSServer *server) {

	TCP_terminate(server->info.server_socket);

#ifndef NO_THREADING

	server->info.thread_pool.stop = true;

	// thread stop
	pthread_join(server->info.request_acceptor, NULL);
	llog(LOG_INFO, "[SERVER] Request acceptor stopped\n");

	// tpool stop
	auto err = destroy_threadpool(&server->info.thread_pool);
	SNS_RET_IF_ERR(err) // Does not return an error (yet)
	llog(LOG_INFO, "[SERVER] Sent stop signal to all threads\n");
#endif

	SSL_destroy_context(server->info.ssl_context);

	SSL_terminate();

	sigaction(SIGPIPE, &old_sigaction, NULL);

	llog(LOG_INFO, "[SERVER] Server stopped\n");

	return NO_ERROR;
}

ERRFUNC create_SNSServer(SNSServer *server) {

	*server = (SNSServer){};

	// there is no need yet for comparing regex
	auto err = MiniVector_regex_t_make(10, &server->routes.regexes);
	SNS_RET_IF_ERR(err)
	err = MiniVector_StringView_make(10, &server->routes.names);
	if (err > NO_OP) {
		auto err2 = MiniVector_regex_t_destroy(&server->routes.regexes);
		SNS_RET_IF_ERR(err2) // Destroy does not return an error yet
		return err;
	}
	err = MiniVector_MessageProcessor_make(10, &server->routes.processors);
	if (err > NO_OP) {
		auto err2 = MiniVector_regex_t_destroy(&server->routes.regexes);
		SNS_RET_IF_ERR(err2) // Destroy does not return an error yet
		err2 = MiniVector_StringView_destroy(&server->routes.names);
		SNS_RET_IF_ERR(err2) // Destroy does not return an error yet
		return err;
	}

	server->routes.default_processor = NotFound404;

	return NO_ERROR;
}

ERRFUNC destroy_SNSServer(SNSServer *server) {

	regex_t empty = {};

	for (size_t i = 0; i < server->routes.regexes.count; ++i) {
		regex_t regex = {};
		auto    err   = MiniVector_regex_t_get(&server->routes.regexes, i, &regex);
		SNS_RET_IF_ERR(err) // get can'r return anything now since `i` is bounded
		if (memcmp(&empty, &regex, sizeof(regex_t)) != 0) {
			regfree(&regex);
		}
	}
	auto err = MiniVector_regex_t_destroy(&server->routes.regexes);
	SNS_RET_IF_ERR(err) // MiniMap destroy does not return an error yet
	err = MiniVector_StringView_destroy(&server->routes.names);
	SNS_RET_IF_ERR(err) // MiniMap destroy does not return an error yet
	err = MiniVector_MessageProcessor_destroy(&server->routes.processors);
	SNS_RET_IF_ERR(err) // MiniMap destroy does not return an error yet

	return NO_ERROR;
}

ERRFUNC register_route(const StringView *route_name, const MessageProcessor proc, const bool regex, SNSServer *server) {

	if (proc == nullptr) {
		return INVALID_ARGUMENT;
	}

	if (route_name == nullptr) {
		server->routes.default_processor = proc;
		return NO_ERROR;
	}

	regex_t reg = {};

	if (regex) {
		char    *buff = malloc(route_name->len + 1);
		SNS_TEST_ALLOC(buff);
		// regcomp wants a null terminated string
		memcpy(buff, route_name->str, route_name->len);
		buff[route_name->len] = '\0';

		auto code = regcomp(&reg, buff, REG_EXTENDED | REG_NOSUB);
		if (code != 0) {
			regerror(code, &reg, buff, route_name->len + 1);
			llog(LOG_WARNING, "[ROUTING] Regex compilation failed for pattern '%.*s' failed with error: %s\n", clamp_to_positive_int(route_name->len), route_name->str, buff);
			free(buff);
			return INVALID_REGEX;
		}
		free(buff);
		auto err = MiniVector_regex_t_append(&server->routes.regexes, &reg);
		if (err > NO_OP) {
			regfree(&reg);
			return err;
		} // only FAILED_ALLOCATION
	} else {
		regex_t empty = {}; // this is a bit of an hack, It's impossible for a valid regex to be all 0 and that's probably never gonna change
		                    // else I'd have to create a bitfield
		auto    err   = MiniVector_regex_t_append(&server->routes.regexes, &empty);
		SNS_RET_IF_ERR(err);
	}

	auto err = MiniVector_StringView_append(&server->routes.names, route_name);
	if (err > NO_OP) {
		if (regex) {
			regfree(&reg);
		}
		auto err2 = MiniVector_regex_t_remove(&server->routes.regexes, server->routes.regexes.count - 1);
		SNS_RET_IF_ERR(err2)
		return err;
	}

	err = MiniVector_MessageProcessor_append(&server->routes.processors, &proc);
	if (err > NO_OP) {
		if (regex) {
			regfree(&reg);
		}
		auto err2 = MiniVector_regex_t_remove(&server->routes.regexes, server->routes.regexes.count - 1);
		SNS_RET_IF_ERR(err2)
		err2 = MiniVector_StringView_remove(&server->routes.names, server->routes.names.count - 1);
		SNS_RET_IF_ERR(err2)
		return err;
	}

	return NO_ERROR;
}

ERRFUNC unregister_route(const StringView *route_name, SNSServer *server) {
	if (route_name == nullptr) {
		server->routes.default_processor = NotFound404;
		return NO_ERROR;
	}

	for (size_t i = 0; i < server->routes.names.count; ++i) {
		StringView name = {};
		// this can't fail because the index is bounded
		auto ign        = MiniVector_StringView_get(&server->routes.names, i, &name);
		if (equal_sized_strings(route_name->str, route_name->len, name.str, name.len)) {

			regex_t regex = {};
			regex_t empty = {};
			// these can't fail because the index is bounded
			ign           = MiniVector_regex_t_get(&server->routes.regexes, i, &regex);
			if (memcmp(&regex, &empty, sizeof(regex_t)) != 0) {
				regfree(&regex);
			}
			ign = MiniVector_regex_t_remove(&server->routes.regexes, i);
			ign = MiniVector_StringView_remove(&server->routes.names, i);
			ign = MiniVector_MessageProcessor_remove(&server->routes.processors, i);

			return NO_ERROR;
		}
	}

	return NO_OP;
}
