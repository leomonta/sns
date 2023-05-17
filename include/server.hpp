#pragma once

#include "HTTP_message.hpp"
#include "tcpConn.hpp"

void acceptRequests(tcpConn *tcpConnection, bool *threadStop);
void resolveRequest(Socket clientSocket, tcpConn *tcpConnection, bool *threadStop);

void        Head(HTTP_message &inbound, HTTP_message &outbound);
void        Get(HTTP_message &inbound, HTTP_message &outbound);
void        composeHeader(const std::string &filename, std::map<std::string, std::string> &result);
std::string getFile(const std::string &file);

void parseArgs(int argc, char *argv[]);
void setup();
void stop();
void restart();
void start();

void setupContentTypes();
void getContentType(const std::string &filetype, std::string &result);