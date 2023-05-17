#pragma once

/*
Thanks to -> https://stackoverflow.com/questions/7698488/turn-a-simple-socket-into-an-ssl-socket
und to -> https://gist.github.com/vedantroy/d2b99d774484cf4ea5165b200888e414
*/
#include <openssl/ssl.h>
// #include <openssl/err.h>

namespace sslConn {

	void initializeServer();

	void terminateServer();

	SSL_CTX *createContext();

	void destroyContext(SSL_CTX *ctx);

	SSL *createConnection(SSL_CTX *ctx);

	void destroyConnection(SSL *ssl);
} // namespace sslConn