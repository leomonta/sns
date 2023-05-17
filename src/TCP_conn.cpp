#include "TCP_conn.hpp"

#include "logger.hpp"
#include "utils.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * Setup the TCP connection on the given port and ip, fails if the port isn't on the local machine and if the port is already used
 * Setup the server listening socket, the one that accept incoming client requests
 */
TCP_conn::TCP_conn(const short port) {

	// create the server socket descriptor, return -1 on failure
	serverSocket = socket(
	    AF_INET,      // IPv4
	    SOCK_STREAM,  // reliable conn, multiple communication per socket, non blocking accept
	    IPPROTO_TCP); // Tcp protocol

	if (serverSocket == INVALID_SOCKET) {
		log(LOG_FATAL, "Impossible to create server Socket.\n	Reason: %d %s\n", errno, strerror(errno));
		return;
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
		return;
	}

	// setup this socket to listen for connection, with the queue of SOMAXCONN	Reason: 2^12
	// SOcket
	// MAXimum
	// CONNections
	errorCode = listen(serverSocket, SOMAXCONN);

	if (errorCode == -1) {
		log(LOG_FATAL, "Listening failed.\n	Reason: %d %s\n", errno, strerror(errno));
		return;
	}

	isConnValid = true;
}

TCP_conn::~TCP_conn() {
	shutDown(serverSocket);
	closeSocket(serverSocket);
}

/**
 * Shortcut to accept any client socket, to be used multiple time by thread
 * this functions blocks until it receive a socket
 */
Socket TCP_conn::acceptClientSock() {

	// the client socket address
	sockaddr clientAddr;

	// size of the client socket address
	socklen_t clientSize = sizeof(clientAddr);

	// get the socket (int) and the socket address fot the client
	Socket client = accept(serverSocket, &clientAddr, &clientSize);

	// received and invalid socket
	if (client == INVALID_SOCKET) {
		log(LOG_ERROR, "Could not accept client socket\n	Reason: %d %s\n", errno, strerror(errno));
		return -1;
	}

	sockaddr_in *temp = reinterpret_cast<sockaddr_in *>(&clientAddr);

	// everything fine, communicate on stdout
	log(LOG_INFO, "[Socket %d] Accepted client IP %s:%u\n", client, inet_ntoa(temp->sin_addr), ntohs(temp->sin_port));

	return client;
}

/**
 * Destroy the socket
 */
void TCP_conn::closeSocket(const Socket clientSock) {

	auto res = close(clientSock);

	if (res < 0) {
		log(LOG_ERROR, "[Socket %d] Could not close socket\n	Reason: %d %s\n", clientSock, errno, strerror(errno));
	}
}

/**
 * close the communication from the server to the client and viceversa
 */
void TCP_conn::shutDown(const Socket clientSock) {
	// shutdown for both ReaD and WRite
	auto res = shutdown(clientSock, SHUT_RDWR);

	if (res < 0) {
		log(LOG_ERROR, "[Socket %d] Could not shutdown socket\n	Reason: %d %s\n", clientSock, errno, strerror(errno));
	}
}

/**
 * copy the incoming message requested to buff and return the bytes received
 *
 * if the bytes received are bigger than the buffer length, the remaining bytes
 */
int TCP_conn::receiveRequest(const Socket clientSock, std::string &result) {
	char recvbuf[DEFAULT_BUFLEN];
	// result is the amount of bytes received

	auto bytesReceived = recv(clientSock, recvbuf, DEFAULT_BUFLEN, 0);

	if (bytesReceived > 0) {
		result = std::string(recvbuf, bytesReceived);
		log(LOG_INFO, "[Socket %d] Received %ldB from client\n", clientSock, bytesReceived);
	}

	if (bytesReceived == 0) {
		log(LOG_INFO, "[Socket %d] Client has shut down the communication\n", clientSock);
		result = "";
	}

	if (bytesReceived < 0) {
		log(LOG_ERROR, "[Socket %d] Failed to receive message\n	Reason: %d %s\n", clientSock, errno, strerror(errno));
		result = "";
	}

	return bytesReceived;
}

/**
 * Send the buffer (buff) to the client, and return the bytes sent
 */
int TCP_conn::sendResponse(const Socket clientSock, std::string &buff) {

	auto bytesSent = send(clientSock, buff.c_str(), buff.size(), 0);
	if (bytesSent < 0) {
		log(LOG_ERROR, "Failed to send message\n	Reason: %d %s\n", errno, strerror(errno));
	}

	if (bytesSent != static_cast<ssize_t>(buff.size())) {
		log(LOG_WARNING, "Mismatch between buffer size (%ldb) and bytes sent (%ldb)\n", buff.size(), bytesSent);
	}

	log(LOG_INFO, "[Socket %d] Sent %dB to client\n", clientSock, bytesSent);

	return bytesSent;
}
