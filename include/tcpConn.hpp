#pragma once

/*
Thanks to -> https://www.linuxhowtos.org/C_C++/socket.htm
*/

// system headers
#include <string>

#define INVALID_SOCKET -1

#define DEFAULT_BUFLEN 8000
typedef int Socket;

class tcpConn {
private:
	// tcp socket listener
	Socket serverSocket = INVALID_SOCKET;

public:
	// signal if the connection is valid
	bool isConnValid = false;

	tcpConn(){};
	void   terminate();
	void   initialize(const short port);
	int    receiveRequest(const Socket clientSock, std::string &result);
	int    sendResponse(const Socket clientSock, std::string &buff);
	Socket acceptClientSock();
	void   closeSocket(const Socket clientSock);
	void   shutDown(const Socket clientSocket);
};