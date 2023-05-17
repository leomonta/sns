#pragma once

/*
Thanks to -> https://stackoverflow.com/questions/7698488/turn-a-simple-socket-into-an-ssl-socket
und to -> https://gist.github.com/vedantroy/d2b99d774484cf4ea5165b200888e414
*/

#include "tcpConn.hpp"

#include <openssl/ssl.h>

namespace sslConn {

	void initializeServer();

	void terminateServer();

	SSL_CTX *createContext();

	void destroyContext(SSL_CTX *ctx);

	SSL *createConnection(SSL_CTX *ctx, Socket client);

	void destroyConnection(SSL *ssl);

	int receiveRecord(SSL *ssl, std::string &buff);

	int sendRecord(SSL *ssl, std::string &buff);

	int acceptClientConnection(SSL *ssl);
} // namespace sslConn