#include "HTTP_message.hpp"
#include "utils.hpp"

#include <iostream>

HTTP_message::HTTP_message(std::string& raw_message, unsigned int dir) {
	message = raw_message;
	direction = dir;

	decompileMessage();
}

/**
* deconstruct the raw header in a map with key (option) -> value
*/
void HTTP_message::decompileHeader() {
	std::vector<std::string> options = split(rawHeader, "\r\n");
	std::vector<std::string> temp;

	for (size_t i = 0; i < options.size(); i++) {

		temp = split(options[i], ": ");
		// first header option "METHOD file HTTP/version"
		if (temp.size() <= 1 && temp[0] != "") {

			temp = split(options[i], " ");
			parseMethod(temp[0]);
			if (temp[1].find("?") != std::string::npos) {
				std::vector<std::string> query = split(temp[1], "?");
				filename = query[0];
				parseQueryParameters(query[1]);
			} else {
				filename = temp[1];
			}
			HTTP_version = temp[2];
		}

		// the last header option is \n
		if (temp.size() >= 2) {
			headerOptions[temp[0]] = temp[1];
		}
	}

}

/**
* decompile message in header and body
*/
void HTTP_message::decompileMessage() {
	// body and header are divided by two newlines
	size_t pos = message.find("\r\n\r\n");

	rawHeader = message.substr(0, pos);
	rawBody = message.substr(pos);

	decompileHeader();
	std::string divisor;

	divisor = headerOptions["Content-Type"];

	// the first four bytes are \r\n\r\n, i need to remove them
	rawBody.erase(rawBody.begin(), rawBody.begin() + 4);

	if (!divisor.empty()) {
		// the fields are separated from each others with a "&" and key -> value are separated with "="
		// plus they are encoded as a URL
		if (divisor == "application/x-www-form-urlencoded") {
			parseQueryParameters(rawBody);
		} else if (divisor == "text/plain") {
			parsePlainParameters(rawBody);
		} else if (divisor.find("multipart/form-data;") != std::string::npos) {

			// find the divisor
			divisor = "--" + split(divisor, "=")[1];
			// sometimes the boundary is encolsed in quotes, take care of this case
			if (divisor[0] == '"' && divisor.back() == '"') {
				divisor.pop_back();
				divisor.erase(divisor.begin());
			}

			parseFormData(rawBody, divisor);
		}

	}
}

/**
* format the header from the array to a string
*/
void HTTP_message::compileHeader() {
	// Always the response code fisrt
	rawHeader += "HTTP/1.1 " + headerOptions["HTTP/1.1"] + "\r\n";

	for (auto const& [key, val] : headerOptions) {
		// already wrote response code
		if (key != "HTTP/1.1") {
			// header option name :	header option value
			rawHeader += key + ": " + val + "\r\n";
		}
	}
}

/**
* unite the header and the body in a single message
*/
void HTTP_message::compileMessage() {
	compileHeader();
	message = rawHeader + "\r\n" + rawBody;
}

/**
* given the string containing the request method the the property method to it's correct value
*/
void HTTP_message::parseMethod(std::string& requestMethod) {
	// Yep that's it, such logic
	if (requestMethod == "GET") {
		method = HTTP_GET;
	} else if (requestMethod == "HEAD") {
		method = HTTP_HEAD;
	} else if (requestMethod == "POST") {
		method = HTTP_POST;
	} else if (requestMethod == "PUT") {
		method = HTTP_PUT;
	} else if (requestMethod == "DELETE") {
		method = HTTP_DELETE;
	} else if (requestMethod == "OPTIONS") {
		method = HTTP_OPTIONS;
	} else if (requestMethod == "CONNECT") {
		method = HTTP_CONNECT;
	} else if (requestMethod == "TRACE") {
		method = HTTP_TRACE;
	} else if (requestMethod == "PATCH") {
		method = HTTP_PATCH;
	}
}

/**
* Given a string of urlencoded parameters parse and decode them, then insert them in the parameters map in the HTTP_message
*/
void HTTP_message::parseQueryParameters(std::string& params) {
	std::vector<std::string> datas;
	std::vector<std::string> temp;

	// parameters may come from the body via POST or the url via GET
	datas = split(params, "&");

	for (size_t i = 0; i < datas.size(); i++) {
		// temp string to store the decoded value
		char* dst = (char*) datas[i].c_str();
		urlDecode(dst, datas[i].c_str());
		//  0 |  1
		// key=value
		temp = split(dst, "=");

		if (temp.size() >= 2 && temp[1] != "") {
			parameters[temp[0]] = temp[1];
		}

	}
}

/**
* Parse the data not encoded in form data and then insert them in the parameters map in the HTTP_message
*/
void HTTP_message::parsePlainParameters(std::string& params) {
	std::vector<std::string> datas;
	std::vector<std::string> temp;

	datas = split(params, "\r\n");

	// the last place always contains "--", no real data in here
	datas.pop_back();
	datas.erase(datas.begin());

	for (size_t i = 0; i < datas.size(); i++) {
		temp = split(datas[i], "=");

		if (temp.size() >= 2) {
			parameters[temp[0]] = temp[1];
		}

	}
}

/**
* Parse the data divided by special divisor in form data and then insert them in the parameters map in the HTTP_message
*/
void HTTP_message::parseFormData(std::string& params, std::string& divisor) {

	std::vector<std::string> datas;
	std::vector<std::string> temp;

	datas = split(params, divisor);

	// the last place always contains "--", no real data in here
	datas.pop_back();
	datas.erase(datas.begin());

	for (size_t i = 0; i < datas.size(); i++) {
		/* 0  |                    1                        |    2   |  4   | 5  |
		* \r\n|Content-Disposition: form-data; name="field1"|\r\n\r\n|value1|\r\n|
		*/
		std::vector<std::string> option_value = split(datas[i], "\r\n");
		option_value.pop_back(); // pos 5
		option_value.erase(option_value.begin() + 2); // pos 2
		option_value.erase(option_value.begin()); // pos 0


		//                0              ||      1      ||         2
		// Content-Disposition: form-data; name="field1"; filename="example.txt"
		std::vector<std::string> options = split(option_value[0], "; ");

		// jump directly to "name="
		std::string name = options[1].substr(5);
		// remove heading and trailing double quotes " 
		name.pop_back();
		name.erase(name.begin());

		parameters[name] = option_value[1];

	}
}
