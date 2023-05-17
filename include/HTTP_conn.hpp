#pragma once

/*
Thanks to -> https://www.linuxhowtos.org/C_C++/socket.htm
*/

// system headers
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <vector>

#define INVALID_SOCKET -1

#define DEFAULT_BUFLEN 8000
typedef int Socket;

class HTTP_conn {
private:
	// tcp socket listener

	Socket serverSocket;

	// describe the internet socket address
	struct sockaddr_in serverAddr;

	// http variables
	std::string HTTP_Basedir = "/";
	std::string HTTP_IP		 = "127.0.0.1";
	std::string HTTP_Port	 = "80";

public:
	// Asio variables

	HTTP_conn(const std::string &basedir, const std::string &ip, const std::string &port);
	~HTTP_conn();
	int	   receiveRequest(Socket &clientSock, std::string &result);
	int	   sendResponse(Socket &clientSock, std::string &buff);
	Socket acceptClientSock();
	void   closeSocket(Socket &clientSock);
	void   shutDown(Socket &clientSocket);
};