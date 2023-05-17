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

	// Should be called before everything else
	// Registers digests  and cyphers(whatever that means)
	SSL_library_init();

	// populate all digest and cypher algos to an internal table
	OpenSSL_add_ssl_algorithms();
}

void sslConn::terminateServer() {
	ERR_free_strings();
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
		log(LOG_FATAL, "Could not create ssl context\n");
		return nullptr;
	}

	// Load the keys and cetificates

	auto certUse = SSL_CTX_use_certificate_file(res, "cert.pem", SSL_FILETYPE_PEM);

	if (certUse != 1) {
		ERR_print_errors_fp(stderr);
		log(LOG_FATAL, "Could not load certificate file %s\n", "cert.pem");
		return nullptr;
	}

	auto keyUse = SSL_CTX_use_PrivateKey_file(res, "key.pem", SSL_FILETYPE_PEM);

	if (keyUse != 1) {
		ERR_print_errors_fp(stderr);
		log(LOG_FATAL, "Could not load private key file %s\n", "key.pem");
		return nullptr;
	}

	return res;
}

void sslConn::destroyContext(SSL_CTX *ctx) {
	// destroy and free the context
	SSL_CTX_free(ctx);
}

SSL *sslConn::createConnection(SSL_CTX *ctx, Socket client) {
	// ssl is the actual struct that hold the connectiondata
	auto res = SSL_new(ctx);

	// the conn failed
	if (res == NULL) {
		ERR_print_errors_fp(stderr);
		log(LOG_FATAL, "Could not create and ssl connction\n");
		return nullptr;
	} else {
		log(LOG_DEBUG, "[Socket %d] Created ssl Connection\n", client);
	}

	auto err = SSL_set_fd(res, client);
	if (err != 1) {
		ERR_print_errors_fp(stderr);
		log(LOG_FATAL, "Could not set the tcp client fd to the ssl connection\n");
		return nullptr;
	} else {
		log(LOG_DEBUG, "[Socket %d] ssl connection linked to the socket\n", client);
	}

	return res;
}

void sslConn::destroyConnection(SSL *ssl) {
	// close the connection and free the data in the struct

	SSL_shutdown(ssl);
	SSL_free(ssl);
}

int sslConn::receiveRecord(SSL *ssl, std::string &buff) {

	char recvBuf[DEFAULT_BUFLEN];

	int bytesReceived = DEFAULT_BUFLEN;

	while (true) {
		// if bytes received <= DEFAULT_BUFLEN, return the exact amount of byes received
		bytesReceived = SSL_read(ssl, recvBuf, DEFAULT_BUFLEN);

		if (bytesReceived > 0) {
			log(LOG_INFO, "Received %dB", bytesReceived);
			break;
		}

		if (bytesReceived < 0) {
			ERR_print_errors_fp(stderr);
			log(LOG_FATAL, "Could not read from the ssl connection\n");
		}

		auto errcode = SSL_get_error(ssl, bytesReceived);
		log(LOG_DEBUG, "Read failed with: %s\n", sslErrStr[errcode]);
	}

	buff.append(recvBuf);

	return bytesReceived;
}

int sslConn::sendRecord(SSL *ssl, std::string &buff) {

	auto bytesSent = SSL_write(ssl, buff.c_str(), static_cast<int>(buff.size()));

	if (bytesSent > 0) {
		if (bytesSent != static_cast<int>(buff.size())) {
			log(LOG_WARNING, "Mismatch between buffer size (%ldb) and bytes sent (%ldb)\n", buff.size(), bytesSent);
		}

		log(LOG_INFO, "Sent %ldB to client\n", bytesSent);

		return bytesSent;
	}

	ERR_print_errors_fp(stderr);
	auto errcode = SSL_get_error(ssl, bytesSent);
	switch (errcode) {
	case SSL_ERROR_ZERO_RETURN:
		log(LOG_DEBUG, "Client shutdown connection\n");
		break;
	default:
		log(LOG_DEBUG, "Write failed with: %s\n", sslErrStr[errcode]);
		log(LOG_ERROR, "Could not send Record to client\n");
	};

	return -1;
}

int sslConn::acceptClientConnection(SSL *ssl) {

	while (true) {
		auto res = SSL_accept(ssl);

		if (res == 1) {
			break;
		}

		ERR_print_errors_fp(stderr);
		auto errcode = SSL_get_error(ssl, res);

		switch (errcode) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			continue;

		default:
			log(LOG_DEBUG, "Read failed with: %s\n", sslErrStr[errcode]);
			log(LOG_FATAL, "Could not accept ssl connection\n");
			return 0;
		}
	}

	// Connection established, yuppy
	log(LOG_INFO, "Accepted secure connection\n");
	return 1;
}