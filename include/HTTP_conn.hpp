#pragma once
#undef UNICODE

// system headers
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <asio.hpp>

#define DEFAULT_BUFLEN 8000

class HTTP_conn {
private:

	WSADATA wsaData;
	int iResult;

	// server listen socket
	SOCKET ListenSocket = INVALID_SOCKET;

	struct addrinfo* result = NULL;
	struct addrinfo hints;

	int iSendResult;

	// variables for initialization
	// http
	std::string HTTP_Basedir = "/";
	std::string HTTP_IP = "127.0.0.1";
	std::string HTTP_Port = "80";

	/**
	* just print the error Winsocket error with a little formatting
	*/
	void checkError(std::string type) {

		if (iResult != 0) {
			std::cout << type << " failed with error: " << iResult << " " << WSAGetLastError() << std::endl;
			return;
		}
	}

public:

	HTTP_conn(const char* basedir, const char* ip, const char* port);
	int receiveRequest(SOCKET* clientSock, std::string& result);
	int sendResponse(SOCKET* clientSock, std::string* buff);
	SOCKET acceptClientSock();
	void closeClientSock(SOCKET* clientSock);
	void shutDown(SOCKET* clientSock);

};