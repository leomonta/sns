#pragma once

#include "httpMessage.hpp"

#include <sslConn.hpp>
#include <tcpConn.hpp>

// requested file possible states
#define FILE_FOUND            0
#define FILE_NOT_FOUND        1
#define FILE_IS_DIR_FOUND     2
#define FILE_IS_DIR_NOT_FOUND 3

void acceptRequestsSecure(bool *threadStop);
void resolveRequestSecure(SSL *sslConn, Socket clientSocket, bool *threadStop);

int         Head(httpMessage &inbound, httpMessage &outbound);
void        Get(httpMessage &inbound, httpMessage &outbound);
void        composeHeader(const std::string &filename, httpMessage &msg, const int fileInfo);
std::string getFile(const std::string &file, const int fileInfo);
std::string getDirView(const std::string &dirname);

void parseArgs(const int argc, const char *argv[]);
void setup();
void stop();
void restart();
void start();

void setupContentTypes();
void getContentType(const std::string &filetype, std::string &result);