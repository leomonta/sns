#include "sns.h"

#include "utils.h"

#include <errno.h>
#include <logger.h>
#include <poll.h>
#include <regex.h>

#ifndef NO_THREADING
#	define SNS_NO_RETURN     [[noreturn]]
#	define SNS_THREAD_RETURN pthread_exit(NULL)
#else
#	define SNS_NO_RETURN
#	define SNS_THREAD_RETURN return nullptr;
#endif

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

void register_route(const StringRef *route_name, const MessageProcessor *fun, SNSServer *server) {
	MiniMap_StringRef_MessageProcessor_set(&server->mapped_functions, route_name, fun);
}

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
			continue;
		}

		auto err = SSL_accept_client(ssl_connection);

		if (err == -1) {
			continue;
		}

		llog(LOG_DEBUG, "[SERVER] Launched request resolver for socket %d\n", client);

		ResolverData t_data = {ssl_connection, client, &server->mapped_functions};

#ifdef NO_THREADING
		proxy_res_req(t_data);
#else
		enqueue_threadpool(&server->info.thread_pool, &t_data);
#endif
	}
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

	auto const sz = mapped_function->keys.count;

	for (size_t i = 0; i < sz; ++i) {
		// take a key value pair
		StringRef  kk;
		auto const err = MiniMap_StringRef_MessageProcessor_get_by_index(mapped_function, i, &kk, &vv);
		if (err) {
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
		auto    errcode = regcomp(&regex, buff, REG_EXTENDED | REG_NOSUB);
		if (errcode != 0) {
			regerror(errcode, &regex, buff, INTERNAL_BUFF_SIZE + 1);
			llog(LOG_WARNING, "[FINDING RESOLVER] Regex compilation failed for pattern '%.*s' failed with error: %s\n", CLAMP_TO_POSITIVE_INT(kk.len), kk.str, buff);
		}

		errcode = regexec(&regex, url->str, 0, nullptr, 0);

		// I actually care only about if there was a match of not
		regfree(&regex);
		if (errcode != REG_NOMATCH) {
			return vv;
		}
	}

	llog(LOG_WARNING, "[FINDING RESOLVER] Could not find any matching processor '%.*s'\n", CLAMP_TO_POSITIVE_INT(url->len), url->str);

	res = MiniMap_StringRef_MessageProcessor_get(mapped_function, url, &vv);
	if (res) {
		llog(LOG_WARNING, "[FINDING RESOLVER] Going for the default MessageProcessor\n");
		return vv;
	}
	return nullptr;
}

void resolve_request(SSL *ssl_connection, const Socket client_socket, MiniMap_StringRef_MessageProcessor *mapped_function) {

	// ---------------------------------------------------------------------- RECEIVE
	char *request        = nullptr;
	auto  bytes_received = SSL_receive_record(ssl_connection, &request);

	// received some bytes
	if (bytes_received > 0) {

		InboundHttpMessage inbound_mex = parse_InboundMessage(request);
		free(request);
		llog(LOG_INFO, "[SERVER] Received request <%s> \n", method_str[inbound_mex.method]);

		OutboundHttpMessage outbound_mex = {};
		outbound_mex.header_options      = MiniMap_u_char_StringOwn_make(16, compare_u_char);
		outbound_mex.version             = HTTP_VER_10;

		MessageProcessor processor = findProcessor(&inbound_mex.url, mapped_function);
		if (processor != nullptr) {
			processor(&inbound_mex, &outbound_mex);

			// make the message a single formatted string
			auto res = compose_message(&outbound_mex);
			llog(LOG_DEBUG, "[SERVER] Message compiled -> \n%.*s\n", CLAMP_TO_POSITIVE_INT(res.len), res.str);

			// ---------------------------------------------------------------------- SEND
			// acknowledge the segment back to the sender
			SSL_send_record(ssl_connection, res.str, res.len);
			free(res.str);
		}

		destroy_OutboundHttpMessage(&outbound_mex);
		destroy_InboundHttpMessage(&inbound_mex);
	}

	TCP_shutdown_socket(client_socket);
	TCP_close_socket(client_socket);
	SSL_destroy_connection(ssl_connection);
}

// Yes, these two could be conbines into a single #ifdef but this is clearer

SNS_NO_RETURN void *proxy_acc_req(void *ptr) {
	accept_requests((SNSServer *)(ptr));
	SNS_THREAD_RETURN;
}

SNS_NO_RETURN void *proxy_res_req(void *ptr) {
	ThreadPool *pool = (ThreadPool *)(ptr);

	while (!pool->stop) {
		auto data = dequeue_threadpool(pool);

		// dequeue might return a null value for how it is implemented, i deal with that
		if (data.ssl == nullptr) {
			llog(LOG_DEBUG, "[THREAD] DEQUEUE return null. Probably the threadpool is commiting fake data to kill all threads\n");
			continue;
		}

		resolve_request(data.ssl, data.client_socket, data.mapped_function);
	}

	SNS_THREAD_RETURN;
}

void setup(SNSServer *server) {

	errno = 0;

	// initializing the tcp Server
	server->info.server_socket = TCP_initialize_server(server->settings.tcp_port, 4);

	if (server->info.server_socket == INVALID_SOCKET) {
		exit(1);
	}

	llog(LOG_INFO, "[SERVER] Listening on %.*s:%d\n", CLAMP_TO_POSITIVE_INT(server->settings.base_dir.len), server->settings.base_dir.str, server->settings.tcp_port);

	// initializing the ssl connection data
	SSL_initialize();
	server->info.ssl_context = SSL_create_context("/usr/local/bin/server.crt", "/usr/local/bin/key.pem");

	if (server->info.ssl_context == nullptr) {
		SSL_terminate();
		exit(1);
	}

	llog(LOG_INFO, "[SSL] Context created\n");

	// finally creating the threadPool
	initialize_threadpool(20, proxy_res_req, &server->info.thread_pool);

	llog(LOG_INFO, "[THREAD POOL] Started the listenings threads\n");
}

void SNSstart(SNSServer *server) {

	setup(server);

	server->info.thread_pool.stop = false;

#ifdef NO_THREADING
	proxy_acc_req(server);
#else
	pthread_create(&server->info.request_acceptor, NULL, proxy_acc_req, server);
#endif

	llog(LOG_DEBUG, "[SERVER] Request acceptor thread Started\n");

	server->info.start_time = time(nullptr);
}

void SNSrestart(SNSServer *server) {
	SNSstop(server);
	SNSstart(server);
}

void SNSstop(SNSServer *server) {

	TCP_terminate(server->info.server_socket);

	// tpool stop
	server->info.thread_pool.stop = true;
	destroy_threadpool(&server->info.thread_pool);
	llog(LOG_INFO, "[SERVER] Sent stop signal to all threads\n");

	// thread stop
	pthread_join(server->info.request_acceptor, NULL);
	llog(LOG_INFO, "[SERVER] Request acceptor stopped\n");

	SSL_destroy_context(server->info.ssl_context);

	SSL_terminate();

	llog(LOG_INFO, "[SERVER] Server stopped\n");
}
