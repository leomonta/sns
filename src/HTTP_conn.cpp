#include "HTTP_conn.hpp"
#include "utils.hpp"

#include <arpa/inet.h>
#include <string.h>

/**
 * Setup the TCP connection on the given port and ip, fails if the port isn't on the local machine and if the port is already used
 * Setup the server listening socket, the one that accept incoming client requests
 */
HTTP_conn::HTTP_conn(const std::string &port) {


	auto int_port = std::stoi(port);

	// create the server socket descriptor, return -1 on failure
	serverSocket = socket(
		AF_INET,					 // IPv4
		SOCK_STREAM | SOCK_NONBLOCK, // reliable conn, multiple communication per socket, non blocking accept
		IPPROTO_TCP);				 // Tcp protocol

	if (serverSocket == INVALID_SOCKET) {
		std::cout << "[Error]: Impossible to create server Socket. " << strerror(errno) << std::endl;
	}

	int enable = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));


	sockaddr_in serverAddr;
	/*
	type of address of this socket
	type of inbound socket
	port
	*/
	serverAddr.sin_family	   = AF_INET;		  // again IPv4
	serverAddr.sin_addr.s_addr = INADDR_ANY;	  // accept any type of ipv4 address
	serverAddr.sin_port		   = htons(int_port); // change to network byte order since needed internally,
												  // network byte order is Big Endian, this machine is Little Endian

	// bind the socket, "activate the socket"
	// return -1 on failure
	auto errorCode = bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr));

	if (errorCode == -1) {
		std::cout << "[Error]: Bind failed. " << strerror(errno) << std::endl;
	}

	// setup this socket to listen for connection, with the queue of SOMAXCONN -> 2^12
	errorCode = listen(serverSocket, SOMAXCONN);

	if (errorCode == -1) {
		std::cout << "[Error]: Listening failed. " << strerror(errno) << std::endl;
	}

	isConnValid = true;

}


HTTP_conn::~HTTP_conn() {
	shutDown(serverSocket);
	closeSocket(serverSocket);
}

/**
 * Shortcut to accept any client socket, to be used multiple time by thread
 * this functions blocks until it receive a socket
 */
Socket HTTP_conn::acceptClientSock() {

	sockaddr clientAddr;

	socklen_t clientSize = sizeof(clientAddr);

	Socket client = accept(serverSocket, &clientAddr, &clientSize);

	if (client == INVALID_SOCKET) {
		// std::cout << "[Error]: Could not accept client socket. " << strerror(errno) << std::endl;
		return -1;
	}

	// no clients for now
	if (client == EWOULDBLOCK || client == EAGAIN) {
		return -1;
	}

	sockaddr_in *temp = reinterpret_cast<sockaddr_in *>(&clientAddr);

	std::cout << "[Info]: Accepted client " << inet_ntoa(temp->sin_addr) << ":" << ntohs(temp->sin_port) << " with socket " << client << "\n";

	return client;
}

/**
 * Close the communication on the socket on both directions
 * the destroy the socket
 */
void HTTP_conn::closeSocket(Socket &clientSock) {

	auto res = close(clientSock);

	if (res < 0) {
		std::cout << "[Error]: Could not close Socket " << clientSock << ". " << strerror(errno) << std::endl;
	}
}

/**
 * close the communication from the server to the client and viceversa
 */
void HTTP_conn::shutDown(Socket &clientSock) {
	auto res = shutdown(clientSock, SHUT_RDWR);

	if (res < 0) {
		std::cout << "[Error]: Could not shutdown Socket " << clientSock << ". " << strerror(errno) << std::endl;
	}
}

/**
 * copy the incoming message requested to buff and return the bytes received
 *
 * if the bytes received are bigger than the buffer length, the remaining bytes
 */
int HTTP_conn::receiveRequest(Socket &clientSock, std::string &result) {
	char recvbuf[DEFAULT_BUFLEN];
	// result is the amount of bytes received

	auto bytesReceived = recv(clientSock, recvbuf, DEFAULT_BUFLEN, 0);
	if (bytesReceived > 0) {
		result = std::string(recvbuf, bytesReceived);

	} else {
		std::cout << "[Error]: failed to receive message. " << strerror(errno) << std::endl;

		result = "";
	}

	return bytesReceived;
}

/**
 * Send the buffer (buff) to the client, and return the bytes sent
 */
int HTTP_conn::sendResponse(Socket &clientSock, std::string &buff) {

	int result = send(clientSock, buff.c_str(), buff.size(), 0);
	if (result < 0) {

		std::cout << "[Error]: failed to send message. " << strerror(errno) << std::endl;
	}
	return result;
}
