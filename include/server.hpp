#pragma once

#include "HTTP_message.hpp"
#include "TCP_conn.hpp"

void acceptRequests(TCP_conn *tcpConnection, bool *threadStop);
void resolveRequest(Socket clientSocket, TCP_conn *tcpConnection, bool *threadStop);

void        Head(HTTP_message &inbound, HTTP_message &outbound);
void        Get(HTTP_message &inbound, HTTP_message &outbound);
void        composeHeader(const std::string &filename, std::map<std::string, std::string> &result);
std::string getFile(const std::string &file);

void setupContentTypes();
void parseArgs(int argc, char *argv[]);

void getContentType(const std::string &filetype, std::string &result);