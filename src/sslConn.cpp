#include "sslConn.hpp"

#include <logger.hpp>
#include <openssl/err.h>

const char *sslErrStr[]{

    "SSL_ERROR_NONE",
    "SSL_ERROR_SSL",
    "SSL_ERROR_WANT_READ",
    "SSL_ERROR_WANT_WRITE",
    "SSL_ERROR_WANT_X509_LOOKUP",
    "SSL_ERROR_SYSCALL",
    "SSL_ERROR_ZERO_RETURN",
    "SSL_ERROR_WANT_CONNECT",
    "SSL_ERROR_WANT_ACCEPT",
    "SSL_ERROR_WANT_ASYNC",
    "SSL_ERROR_WANT_ASYNC_JOB",
    "SSL_ERROR_WANT_CLIENT_HELLO_CB",
    "SSL_ERROR_WANT_RETRY_VERIFY",
};

void sslConn::initializeServer() {
	// make libssl and libcrypto errors readable, before library_init
	SSL_load_error_strings();
	log(LOG_DEBUG, "[SSL] Loaded error strings\n");

	// Should be called before everything else
	// Registers digests  and cyphers(whatever that means)
	SSL_library_init();
	log(LOG_DEBUG, "[SSL] Initialized library\n");

	// populate all digest and cypher algos to an internal table
	OpenSSL_add_ssl_algorithms();
	log(LOG_DEBUG, "[SSL] Loaded the cyphers and digest algorithms\n");
}

void sslConn::terminateServer() {
	ERR_free_strings();

	log(LOG_DEBUG, "[SSL] Error string fred\n");
}

SSL_CTX *sslConn::createContext() {
	// TLS is the newer version of ssl
	// use SSLv23_server_method() for sslv2, sslv3 and tslv1 compartibility
	// create a framework to create ssl struct for connections
	auto res = SSL_CTX_new(TLS_server_method());

	// maybe needed
	// SSL_CTX_set_options(res, SSL_OP_SINGLE_DH_USE);

	// the context could not be created
	if (res == nullptr) {
		ERR_print_errors_fp(stderr);
		// Should alos always use my logger
		log(LOG_FATAL, "[SSl] Could not create context\n");
		return nullptr;
	}

	log(LOG_DEBUG, "[SSL] Created context with the best protocol available\n");

	// Load the keys and cetificates

	auto cerFile = "/usr/local/bin/cert.pem";
	auto certUse = SSL_CTX_use_certificate_file(res, cerFile, SSL_FILETYPE_PEM);

	if (certUse != 1) {
		ERR_print_errors_fp(stderr);
		log(LOG_FATAL, "[SSL] Could not load certificate file '%s'\n", cerFile);
		return nullptr;
	}

	log(LOG_DEBUG, "[SSL] Loaded server certificate '%s'\n", cerFile);

	auto keyFile = "/usr/local/bin/key.pem";
	auto keyUse  = SSL_CTX_use_PrivateKey_file(res, keyFile, SSL_FILETYPE_PEM);

	if (keyUse != 1) {
		ERR_print_errors_fp(stderr);
		log(LOG_FATAL, "[SSL] Could not load private key file '%s'\n", keyFile);
		return nullptr;
	}

	log(LOG_DEBUG, "[SSL] Loaded server private key '%s'\n", keyFile);

	return res;
}

void sslConn::destroyContext(SSL_CTX *ctx) {
	// destroy and free the context
	SSL_CTX_free(ctx);

	log(LOG_DEBUG, "[SSL] Context destroyed\n");
}

SSL *sslConn::createConnection(SSL_CTX *ctx, Socket client) {
	// ssl is the actual struct that hold the connectiondata
	auto res = SSL_new(ctx);

	// the conn failed
	if (res == NULL) {
		ERR_print_errors_fp(stderr);
		log(LOG_ERROR, "[SSL] Could not create a connection\n");
		return nullptr;
	}

	log(LOG_DEBUG, "[SSL] Created new connection\n");

	auto err = SSL_set_fd(res, client);
	if (err != 1) {
		ERR_print_errors_fp(stderr);
		log(LOG_ERROR, "[SSL] Could not set the tcp client fd to the ssl connection\n");
		return nullptr;
	}

	log(LOG_DEBUG, "[SSL] Connection linked to the socket\n");

	log(LOG_INFO, "[SSL] Successfully created and linked a connection to the client sokcet %d\n", client);

	return res;
}

void sslConn::destroyConnection(SSL *ssl) {
	// close the connection and free the data in the struct

	SSL_shutdown(ssl);
	SSL_free(ssl);

	log(LOG_DEBUG, "[SSL] Connection destroyed\n");
}

int sslConn::receiveRecord(SSL *ssl, std::string &buff) {

	// FIXME: receiving ZERO_RETURN as error code lead to a SIGSEGV read smeowhere in or after this function

	char recvBuf[DEFAULT_BUFLEN];

	int bytesReceived = DEFAULT_BUFLEN;

	do {
		// if bytes received <= DEFAULT_BUFLEN, return the exact amount of byes received
		bytesReceived = SSL_read(ssl, recvBuf, DEFAULT_BUFLEN);

		if (bytesReceived > 0) {
			log(LOG_INFO, "[SSL] Received %dB from connection\n", bytesReceived);
			break;
		}

		if (bytesReceived < 0) {
			ERR_print_errors_fp(stderr);
			log(LOG_ERROR, "[SSL] Could not read from the ssl connection\n");
		}

		auto errcode = SSL_get_error(ssl, bytesReceived);
		if (errcode == SSL_ERROR_SSL) {
			log(LOG_DEBUG, "[SSL] Client shut down the communication");
			bytesReceived = 0;
			break;
		}
		if (errcode != SSL_ERROR_NONE) {
			log(LOG_DEBUG, "[SSL] Read failed with: %s\n", sslErrStr[errcode]);
		}
	} while (SSL_pending(ssl) > 0);

	buff.append(recvBuf);

	return bytesReceived;
}

int sslConn::sendRecord(SSL *ssl, std::string &buff) {

	int errcode   = SSL_ERROR_NONE;
	int bytesSent = 0;

	do {
		bytesSent = SSL_write(ssl, buff.c_str(), static_cast<int>(buff.size()));

		errcode = SSL_get_error(ssl, bytesSent);

		if (bytesSent < 0) {
			ERR_print_errors_fp(stderr);
			switch (errcode) {
			case SSL_ERROR_ZERO_RETURN:
				log(LOG_DEBUG, "[SSL] Client shutdown connection\n");
				break;
			default:
				log(LOG_DEBUG, "[SSL] Write failed with: %s\n", sslErrStr[errcode]);
				log(LOG_ERROR, "[SSL] Could not send Record to client\n");
			};

			return -1;
		}
	} while (errcode == SSL_ERROR_WANT_WRITE);

	if (bytesSent != static_cast<int>(buff.size())) {
		log(LOG_WARNING, "[SSL] Mismatch between buffer size (%ldb) and bytes sent (%ldb)\n", buff.size(), bytesSent);
	}

	log(LOG_INFO, "[SSL] Sent %ldB of data to client\n", bytesSent);

	return bytesSent;
}

int sslConn::acceptClientConnection(SSL *ssl) {

	auto res = SSL_accept(ssl);

	if (res != 1) {

		ERR_print_errors_fp(stderr);
		auto errcode = SSL_get_error(ssl, res);

		log(LOG_DEBUG, "[SSL] Read failed with: %s\n", sslErrStr[errcode]);
		log(LOG_ERROR, "[SSL] Could not accept ssl connection\n");
		return -1;
	}

	// Connection established, yuppy
	log(LOG_INFO, "[SSL] Accepted secure connection\n");
	return 0;
}