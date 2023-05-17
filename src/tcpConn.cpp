#include "tcpConn.hpp"

#include "logger.hpp"
#include "profiler.hpp"
#include "utils.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

Socket tcpConn::initializeServer(const short port) {

	PROFILE_FUNCTION();

	// create the server socket descriptor, return -1 on failure
	auto serverSocket = socket(
	    AF_INET,                     // IPv4
	    SOCK_STREAM | SOCK_NONBLOCK, // reliable conn, multiple communication per socket, non blocking accept
	    IPPROTO_TCP);                // Tcp protocol

	if (serverSocket == INVALID_SOCKET) {
		log(LOG_FATAL, "Impossible to create server Socket.\n	Reason: %d %s\n", errno, strerror(errno));
		return INVALID_SOCKET;
	}

	int enable = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	// input socket of the server
	sockaddr_in serverAddr;
	/*
	type of address of this socket
	type of inbound socket
	port
	*/
	serverAddr.sin_family      = AF_INET;     // again IPv4
	serverAddr.sin_addr.s_addr = INADDR_ANY;  // accept any type of ipv4 address
	serverAddr.sin_port        = htons(port); // change to network byte order since its needed internally,
	                                          // network byte order is Big Endian, this machine is Little Endian

	// bind the socket	Reason: "activate the socket"
	// return -1 on failure
	auto errorCode = bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr));

	if (errorCode == -1) {
		log(LOG_FATAL, "Bind failed.\n	Reason: %d %s\n", errno, strerror(errno));
		return INVALID_SOCKET;
	}

	// setup this socket to listen for connection, with the queue of SOMAXCONN	Reason: 2^12
	// SOcket
	// MAXimum
	// CONNections
	errorCode = listen(serverSocket, SOMAXCONN);

	if (errorCode == -1) {
		log(LOG_FATAL, "Listening failed.\n	Reason: %d %s\n", errno, strerror(errno));
		return INVALID_SOCKET;
	}

	return serverSocket;
}

Socket tcpConn::initializeClient(const int port, const char *server_name) {

	Socket clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (clientSock == INVALID_SOCKET) {
		log(LOG_FATAL, "Impossible to create server Socket.\n	Reason: %d %s\n", errno, strerror(errno));
		return INVALID_SOCKET;
	}

	hostent *server_hn = gethostbyname(server_name);

	if (server_hn == nullptr) {
		log(LOG_FATAL, "Hostname requested is unrechable.\n	Reason. %d %s\n", errno, strerror(errno));
		return INVALID_SOCKET;
	}

	sockaddr_in serv_addr;

	// set the entire server address struct to 0
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

	// copy th server ip from the server hostname to the server socket internet address
	bcopy((char *)server_hn->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server_hn->h_length);
	serv_addr.sin_port = htons(port);

	if (connect(clientSock, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr)) == -1) {
		log(LOG_FATAL, "Connectionn to server failed.\n	Reason. %d %s\n", errno, strerror(errno));
		return INVALID_SOCKET;
	}

	return clientSock;
}

void tcpConn::terminate(const Socket sck) {

	PROFILE_FUNCTION();

	shutdownSocket(sck);
	closeSocket(sck);
}

void tcpConn::closeSocket(const Socket sck) {

	PROFILE_FUNCTION();

	auto res = close(sck);

	if (res < 0) {
		log(LOG_ERROR, "[Socket %d] Could not close socket\n	Reason: %d %s\n", sck, errno, strerror(errno));
	}
}

void tcpConn::shutdownSocket(const Socket sck) {

	PROFILE_FUNCTION();

	// shutdown for both ReaD and WRite
	auto res = shutdown(sck, SHUT_RDWR);

	if (res < 0) {
		log(LOG_ERROR, "[Socket %d] Could not shutdown socket\n	Reason: %d %s\n", sck, errno, strerror(errno));
	}
}

int tcpConn::receiveSegment(const Socket sck, std::string &result) {

	PROFILE_FUNCTION();

	char recvbuf[DEFAULT_BUFLEN];
	// result is the amount of bytes received
	ssize_t bytesReceived    = DEFAULT_BUFLEN;
	int     totBytesReceived = 0;

	while (bytesReceived == DEFAULT_BUFLEN) {
		bytesReceived = recv(sck, recvbuf, DEFAULT_BUFLEN, 0);
		totBytesReceived += bytesReceived;
		result.append(recvbuf, totBytesReceived);
	}

	if (totBytesReceived > 0) {
		log(LOG_INFO, "[Socket %d] Received %ldB from client\n", sck, totBytesReceived);
	}

	if (totBytesReceived == 0) {
		log(LOG_INFO, "[Socket %d] Client has shut down the communication\n", sck);
	}

	if (totBytesReceived < 0) {
		log(LOG_ERROR, "[Socket %d] Failed to receive message\n	Reason: %d %s\n", sck, errno, strerror(errno));
	}

	return totBytesReceived;
}

int tcpConn::sendSegment(const Socket sck, std::string &buff) {

	PROFILE_FUNCTION();

	auto bytesSent = send(sck, buff.c_str(), buff.size(), 0);
	if (bytesSent < 0) {
		log(LOG_ERROR, "Failed to send message\n	Reason: %d %s\n", errno, strerror(errno));
	}

	if (bytesSent != static_cast<ssize_t>(buff.size())) {
		log(LOG_WARNING, "Mismatch between buffer size (%ldb) and bytes sent (%ldb)\n", buff.size(), bytesSent);
	}

	log(LOG_INFO, "[Socket %d] Sent %dB to client\n", sck, bytesSent);

	return bytesSent;
}

Socket tcpConn::acceptClientSock(const Socket ssck) {

	PROFILE_FUNCTION();

	// the client socket address
	sockaddr clientAddr;

	// size of the client socket address
	socklen_t clientSize = sizeof(clientAddr);

	// get the socket (int) and the socket address fot the client
	Socket client = accept(ssck, &clientAddr, &clientSize);

	// received and invalid socket
	if (client == INVALID_SOCKET) {
		// log(LOG_ERROR, "Could not accept client socket\n	Reason: %d %s\n", errno, strerror(errno));
		return -1;
	}

	sockaddr_in *temp = reinterpret_cast<sockaddr_in *>(&clientAddr);

	// everything fine, communicate on stdout
	log(LOG_INFO, "[Socket %d] Accepted client IP %s:%u\n", client, inet_ntoa(temp->sin_addr), ntohs(temp->sin_port));

	return client;
}
