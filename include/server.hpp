#pragma once

#include "httpMessage.hpp"
#include "sslConn.hpp"
#include "tcpConn.hpp"

void acceptRequests(bool *threadStop);
void resolveRequest(Socket clientSocket, bool *threadStop);

void acceptRequestsSecure(bool *threadStop);
void resolveRequestSecure(SSL *sslConn, Socket clientSocket, bool *threadStop);

void        Head(httpMessage &inbound, httpMessage &outbound);
void        Get(httpMessage &inbound, httpMessage &outbound);
void        composeHeader(const std::string &filename, std::map<int, std::string> &result);
std::string getFile(const std::string &file);

void parseArgs(const int argc, const char *argv[]);
void setup();
void stop();
void restart();
void start();

void setupContentTypes();
void getContentType(const std::string &filetype, std::string &result);