#pragma once
#include <map>
#include <string>
#include <vector>
#include <fstream>

// http method code
#define HTTP_METHOD_INVALID -1
#define HTTP_RESPONSE 0
#define HTTP_GET 1
#define HTTP_HEAD 2
// internally, all the methods below or equal to this number are implemented
#define HTTP_POST 3
#define HTTP_PUT 4
#define HTTP_DELETE 5
#define HTTP_OPTIONS 5
#define HTTP_CONNECT 6
#define HTTP_TRACE 7
#define HTTP_PATCH 8 

// message direction
#define HTTP_INBOUND 10
#define HTTP_OUTBOUND 11

class HTTP_message {
private:

	void decompileHeader();
	void decompileMessage();
	void compileHeader();
	void parseMethod(std::string& requestMethod);
	void parseQueryParameters(std::string& params);
	void parsePlainParameters(std::string& params);
	void parseFormData(std::string& params, std::string& divisor);

public:

	unsigned int direction; // 10 inboud, 11 outbound
	unsigned int method; // the appropriate http method
	std::map<std::string, std::string> headerOptions; // represent the header as the collection of the single options -> value
	std::map<std::string, std::string> parameters; // contain the data sent in the forms and query parameters
	std::string rawHeader; // represent the header as a single single string
	std::string rawBody; // represent the body as a single string
	std::string message; // represent the entire http message as header + \r\n + body
	std::string filename; // the resource asked from the client
	std::string HTTP_version; // the version of the http header (1.0, 1.1, 2.0, ...)

	void compileMessage();

	HTTP_message(unsigned int dir = HTTP_OUTBOUND) : direction(dir) {};
	HTTP_message(std::string& raw_message, unsigned int dir = HTTP_INBOUND);
};

