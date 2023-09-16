#pragma once

#include "httpMessage.hpp"

#include <sslConn.hpp>
#include <tcpConn.hpp>

// requested file possible states
#define FILE_FOUND            0
#define FILE_NOT_FOUND        1
#define FILE_IS_DIR_FOUND     2
#define FILE_IS_DIR_NOT_FOUND 3

/**
 * polls for activity on the server socket, ir receives a client start the resolveRequestSecure thread
 *
 * @param threadStop if true stops the infinite loop and exits
 */
void acceptRequestsSecure(bool *threadStop);

/**
 * receive a client that wants to communicate and attempts to resolve it's request
 *
 * @param threadStop if true stops the infinite loop and exits
 * @param clientSock the socket of the client
 * @param sslConn the ssl connection to communicate on
 */
void resolveRequestSecure(SSL *sslConn, const Socket clientSocket, bool *threadStop);

/**
 * Fills the outbound httpMessage with the correct headers for the Head method
 *
 * @param request the request message received from the client
 * @param response what will be communicated to the client
 */
int Head(const httpMessage &request, httpMessage &response);

/**
 * Fill the outbounf httpMessage with the correct headers and body info for the Get method
 *
 * @param request the request message received from the client
 * @param response what will be communicated to the client
 */
void Get(const httpMessage &request, httpMessage &response);

/**
 * Given the name of a file, retrives all of the info to put in the http header
 *
 * @param filename the name of the file
 * @param msg httpMessage where to put the data
 * @param fileInfo additional fileinfo presence in the filesystem
 */
void composeHeader(const std::string &filename, httpMessage &msg, const int fileInfo);

/**
 * given a path returns the file as a string if it is present, a dir view if it's a directory
 *
 * @param path the path to read the file / directory
 * @param fileInfo info if the file is present or not
 *
 * @return the plain text string of the path
 */
std::string getContent(const stringRef &path, const int fileInfo);

/**
 * Given a path of a directory return its dirview in html
 *
 * @param path the path to the directory
 *
 * @return the html for the dirview
 */
std::string getDirView(const std::string &path);

/**
 * given the cli arguments set's the server options accordingly
 *
 * @param argc the count of arguments
 * @param argv the values of the arguments
 */
void parseArgs(const int argc, const char *argv[]);

/**
 * Setup the server, loads libraries and stuff
 *
 * to call one
 */
void setup();

/**
 * stop the server and its threads
 */
void stop();

/**
 * stop and immediatly starts the server again
 */
void restart();

/**
 * starts the server main thread
 */
void start();

/**
 * loads in the hash map for the mime types
 */
void setupContentTypes();

/**
 * sets result as the mime type correct for the give fyle extension
 *
 * @param filetype the file type / extension
 * @param result where to put the mimetype
 */
void getContentType(const std::string &filetype, std::string &result);