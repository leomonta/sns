#pragma once
#include <fstream>
#include <map>
#include <string>
#include <vector>

class httpMessage {
private:
	void decompileHeader();
	void decompileMessage();
	void compileHeader();
	void parseMethod(std::string &requestMethod);
	void parseQueryParameters(std::string &params);
	void parsePlainParameters(std::string &params);
	void parseFormData(std::string &params, std::string &divisor);

public:
	// http method code
	enum methods {
		HTTP_INVALID,
		HTTP_GET,
		HTTP_HEAD,
		HTTP_POST,
		HTTP_PUT,
		HTTP_DELETE,
		HTTP_OPTIONS,
		HTTP_CONNECT,
		HTTP_TRACE,
		HTTP_PATCH
	};

	enum version {
		HTTP_09,
		HTTP_10,
		HTTP_11,
		HTTP_2,
		HTTP_3
	};

	unsigned int                       method;        // the appropriate http method
	unsigned int                       version;       // the version of the http header (1.0, 1.1, 2.0, ...)
	std::map<std::string, std::string> headerOptions; // represent the header as the collection of the single options -> value
	std::map<std::string, std::string> parameters;    // contain the data sent in the forms and query parameters
	std::string                        rawHeader;     // represent the header as a single single string
	std::string                        rawBody;       // represent the body as a single string
	std::string                        message;       // represent the entire http message as header + \r\n + body
	std::string                        filename;      // the resource asked from the client

	void compileMessage();

	httpMessage(){};
	httpMessage(std::string &raw_message);
};
