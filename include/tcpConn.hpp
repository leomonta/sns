#pragma once

/*
Thanks to -> https://www.linuxhowtos.org/C_C++/socket.htm
*/

// system headers
#include <string>

#define INVALID_SOCKET -1

#define DEFAULT_BUFLEN 8000
typedef int Socket;

namespace tcpConn {

	/**
	 * setup a server listening on port
	 * @return the server socket
	 */
	Socket initializeServer(const short port);

	/**
	 * retup a client connected to server_name
	 * @return the client socket
	 */
	Socket initializeClient(const int port, const char *server_name);

	/**
	 * shorthand to close and shutdown a socket
	 */
	void terminate(const Socket sck);

	/**
	 * close the given socked, close the related fd
	 */
	void closeSocket(const Socket sck);

	/**
	 * send the tcp shutdown message through the socket
	 */
	void shutdownSocket(const Socket sck);

	/**
	 * receive a segment from the specified socket
	 * @return the amount of bytes sent
	 */
	int receiveSegment(const Socket sck, std::string &result);

	/**
	 * send a segment through specified socket
	 * @return the amount of bytes received
	 */
	int sendSegment(const Socket sck, std::string &buff);

	/**
	 * @return a client that wants to connect to this server
	 */
	Socket acceptClientSock(const Socket ssck);
} // namespace tcpConn