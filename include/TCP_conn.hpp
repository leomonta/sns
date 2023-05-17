#pragma once

/*
Thanks to -> https://www.linuxhowtos.org/C_C++/socket.htm
*/

// system headers
#include <string>

#define INVALID_SOCKET -1

#define DEFAULT_BUFLEN 8000
typedef int Socket;

class TCP_conn {
private:
	// tcp socket listener
	Socket serverSocket;

public:
	// signal if the connection is valid
	bool isConnValid = false;

	TCP_conn(const short port);
	~TCP_conn();
	int	   receiveRequest(const Socket clientSock, std::string &result);
	int	   sendResponse(const Socket clientSock, std::string &buff);
	Socket acceptClientSock();
	void   closeSocket(const Socket clientSock);
	void   shutDown(const Socket clientSocket);
};