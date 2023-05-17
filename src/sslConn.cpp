#include <logger.hpp>
#include <openssl/err.h>
#include <sslConn.hpp>

void sslConn::initializeServer() {
	// make libssl and libcrypto errors readable, before library_init
	SSL_load_error_strings();

	// Should be called before everything else
	// Registers digests  and cyphers(whatever that means)
	SSL_library_init();

	// populate all digest and cypher algos to an internal table
	OpenSSL_add_ssl_algorithms();

	OPENSSL_config(nullptr);
}

void sslConn::terminateServer() {
	ERR_free_strings();
}

SSL_CTX *sslConn::createContext() {
	// TLS is the newer version of ssl
	// use SSLv23_server_method() for sslv2, sslv3 and tslv1 compartibility
	// create a framework to create ssl struct for connections
	auto res = SSL_CTX_new(TLSv1_server_method());

	// the context could not be created
	if (res == nullptr) {
		ERR_print_errors_fp(stderr);
		// Should alos always use my logger
		log(LOG_FATAL, "Could not create ssl context\n");
		return nullptr;
	}

	// maybe needed
	// SSL_CTX_set_options(res, SSL_OP_SINGLE_DH_USE)

	// Load the keys and cetificates

	auto certUse = SSL_CTX_use_certificate_file(res, "placegolder.pem", SSL_FILETYPE_PEM);

	if (certUse != 1) {
		ERR_print_errors_fp(stderr);
		log(LOG_FATAL, "Could not load certificate file %s\n", "placeholder.pem");
		return nullptr;
	}

	auto keyUse = SSL_CTX_use_PrivateKey_file(res, "placeholder.pem", SSL_FILETYPE_PEM);

	if (keyUse != 1) {
		ERR_print_errors_fp(stderr);
		log(LOG_FATAL, "Could not load private key file %s\n", "placeholder.pem");
		return nullptr;
	}

	return res;
}

void sslConn::destroyContext(SSL_CTX *ctx) {
	// destroy and free the context
	SSL_CTX_free(ctx);
}

SSL *sslConn::createConnection(SSL_CTX *ctx) {
	// ssl is the actual struct that hold the connectiondata
	auto res = SSL_new(ctx);

	// the conn failed
	if (res == nullptr) {
		fprintf(stderr, "SSL_new() failed\n");
		log(LOG_FATAL, "Could not create and ssl connction\n");
	}

	return res;
}

void sslConn::destroyConnection(SSL *ssl) {
	// close the connection and free the data in the struct

	SSL_shutdown(ssl);
	SSL_free(ssl);
}