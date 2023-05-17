#include "httpMessage.hpp"

#include "profiler.hpp"
#include "utils.hpp"

#include <cstring>

const char *headerRqStr[] = {
    "A-IM",
    "Accept",
    "Accept-Charset",
    "Accept-Datetime",
    "Accept-Encoding",
    "Accept-Language",
    "Access-Control-Request-Method",
    "Access-Control-Request-Headers",
    "Authorization",
    "Cache-Control",
    "Connection",
    "Content-Encoding",
    "Content-Length",
    "Content-MD5",
    "Content-Type",
    "Cookie",
    "Date",
    "Expect",
    "Forwarded",
    "From",
    "Host",
    "HTTP2-Settings",
    "If-Match",
    "If-Modified-Since",
    "If-None-Match",
    "If-Range",
    "If-Unmodified-Since",
    "Max-Forwards",
    "Origin",
    "Pragma",
    "Prefer",
    "Proxy-Authorization",
    "Range",
    "Referer",
    "TE",
    "Trailer",
    "Transfer-Encoding",
    "User-Agent",
    "Upgrade",
    "Via",
    "Warning",
};

const char *headerRpStr[] = {
    "Accept-CH",
    "Access-Control-Allow-Origin",
    "Access-Control-Allow-Credentials",
    "Access-Control-Expose-Headers",
    "Access-Control-Max-Age",
    "Access-Control-Allow-Methods",
    "Access-Control-Allow-Headers",
    "Accept-Patch",
    "Accept-Ranges",
    "Age",
    "Allow",
    "Alt-Svc",
    "Cache-Control",
    "Connection",
    "Content-Disposition",
    "Content-Encoding",
    "Content-Language",
    "Content-Length",
    "Content-Location",
    "Content-MD5",
    "Content-Range",
    "Content-Type",
    "Date",
    "Delta-Base",
    "ETag",
    "Expires",
    "IM",
    "Last-Modified",
    "Link",
    "Location",
    "P3P",
    "Pragma",
    "Preference-Applied",
    "Proxy-Authenticate",
    "Public-Key-Pins",
    "Retry-After",
    "Server",
    "Set-Cookie",
    "Strict-Transport-Security",
    "Trailer",
    "Transfer-Encoding",
    "Tk",
    "Upgrade",
    "Vary",
    "Via",
    "Warning",
    "WWW-Authenticate",
    "X-Frame-Options",
    "Status",
};

httpMessage::httpMessage(std::string &rawMessage) {

	PROFILE_FUNCTION();

	// body and header are divided by two newlines
	auto pos = rawMessage.find("\r\n\r\n");

	auto rawHeader = rawMessage.substr(0, pos);

	// the +4 is to account for the length of the string searched
	body = rawMessage.substr(pos + 4);

	http::decompileHeader(rawHeader, *this);

	http::decompileMessage(header[http::RQ_Content_Type], parameters, body);
}

/**
 * deconstruct the raw header in a map with key (option) -> value
 */
void http::decompileHeader(const std::string &rawHeader, httpMessage &msg) {

	PROFILE_FUNCTION();

	std::vector<std::string> options = split(rawHeader, "\r\n");
	std::vector<std::string> line;

	// the first line should be "METHOD URL HTTP/Version"

	//    0    1       2
	// METHOD|URL|HTTP/Version
	line = split(options[0], " ");

	msg.method  = http::getMethodCode(line[0]);
	msg.url     = line[1];
	msg.version = http::getVersionCode(line[2]);

	// the position of the query parameter market (if present)
	auto qPos = msg.url.find("?");

	// if '?' is in the url, it means that query parameters are used
	if (qPos != std::string::npos) {
		// get the entire query
		parseQueryParameters(msg.url.substr(qPos + 1), msg.parameters);

		// erase the '?' and everything after it
		msg.url.erase(msg.url.begin() + qPos);
	}

	// parse the remaining header options

	for (size_t i = 1; i < options.size(); ++i) {

		// each header option is composed as "option: value\n"
		//    0      1
		// option: value
		line = split(options[i], ": ");

		auto code = http::getParameterCode(line[0]);
		if (code == -1) {
			// log the warn
		} else {
			msg.header[code] = line[1];
		}
	}
}

/**
 * decompile message in header and body
 */
void http::decompileMessage(const std::string &cType, std::map<std::string, std::string> &parameters, std::string &body) {

	PROFILE_FUNCTION();

	// the content type indicates how parameters are showed
	if (!cType.empty()) {

		// the fields are separated from each others with a "&" and key -> value are separated with "="
		// plus they are encoded as a URL
		if (cType == "application/x-www-form-urlencoded") {
			http::parseQueryParameters(body, parameters);
			// } else if (cType == "text/plain") {
			// http::parsePlainParameters(body, parameters);
		} else if (cType.find("multipart/form-data;") != std::string::npos) {

			// find the divisor
			//               0              |      1
			// multipart/form-data; boundary=----asdwadawd
			auto divisor = split(cType, "=")[1];
			// sometimes the boundary is encolsed in quotes, take care of this case
			if (divisor[0] == '"' && divisor.back() == '"') {
				divisor.pop_back();
				divisor.erase(divisor.begin());
			}

			http::parseFormData(body, divisor, parameters);
		}
	}
}

/**
 * unite the header and the body in a single message
 */
std::string http::compileMessage(const std::map<int, std::string> &header, const std::string &body) {

	PROFILE_FUNCTION();

	// Always the response code fisrt
	// TODO: implement more methods
	std::string rawHeader = "HTTP/1.0 " + header.at(http::RP_Status);
	rawHeader += "\r\n";

	for (auto const &[key, val] : header) {
		// already wrote response code
		if (key != http::RP_Status) {
			// header option name :	header option value
			rawHeader += std::string(headerRpStr[key]) + ": " + val + "\r\n";
		}
	}

	return rawHeader + "\r\n" + body;
}

int http::getMethodCode(const std::string &requestMethod) {

	PROFILE_FUNCTION();

	// Yep that's it, such logic
	if (requestMethod == "GET") {
		return HTTP_GET;
	} else if (requestMethod == "HEAD") {
		return HTTP_HEAD;
	} else if (requestMethod == "POST") {
		return HTTP_POST;
	} else if (requestMethod == "PUT") {
		return HTTP_PUT;
	} else if (requestMethod == "DELETE") {
		return HTTP_DELETE;
	} else if (requestMethod == "OPTIONS") {
		return HTTP_OPTIONS;
	} else if (requestMethod == "CONNECT") {
		return HTTP_CONNECT;
	} else if (requestMethod == "TRACE") {
		return HTTP_TRACE;
	} else if (requestMethod == "PATCH") {
		return HTTP_PATCH;
	}

	return -1;
}

int http::getVersionCode(const std::string &httpVersion) {

	// Yep that's it, such logic pt 2.0

	if (httpVersion == "HTTP/0.9") {
		return HTTP_09;
	} else if (httpVersion == "HTTP/1.0") {
		return HTTP_10;
	} else if (httpVersion == "HTTP/1.1") {
		return HTTP_11;
	} else if (httpVersion == "HTTP/2") {
		return HTTP_2;
	} else if (httpVersion == "HTTP/3") {
		return HTTP_3;
	}

	return -1;
}

int http::getParameterCode(const std::string &parameter) {

	// thi is one hell of a elif chain
	// lickily i need to check only for the requests headers
	// I'm becoming yan dev

	for (int i = 0; i < REQUEST_HEADERS; ++i) {
		if (parameter == headerRqStr[i]) {
			return i;
		}
	}

	return -1;
}

/**
 * Given a string of urlencoded parameters parse and decode them, then insert them in the parameters map in the httpMessage
 */
void http::parseQueryParameters(const std::string &query, std::map<std::string, std::string> &parameters) {

	PROFILE_FUNCTION();

	std::vector<std::string> temp;

	// each parameter is separeted by the '&'
	auto datas = split(query, "&");

	for (size_t i = 0; i < datas.size(); i++) {
		// temp string to store the decoded value
		const char *src = datas[i].c_str();
		char       *dst = new char[strlen(src) + 1];
		urlDecode(dst, src);

		//  0 |  1
		// key=value
		temp = split(dst, "=");

		delete[] dst;

		if (temp.size() >= 2 && temp[1] != "") {
			parameters[temp[0]] = temp[1];
		}
	}
}

/**
 * Parse the data not encoded in form data and then insert them in the parameters map in the httpMessage
 */
void http::parsePlainParameters(const std::string &params, std::map<std::string, std::string> &parameters) {

	PROFILE_FUNCTION();

	auto datas = split(params, "\r\n");

	// the last place always contains "--", no real data in here
	datas.pop_back();
	datas.erase(datas.begin());

	std::vector<std::string> line;
	for (size_t i = 0; i < datas.size(); i++) {
		line = split(datas[i], "=");

		if (line.size() >= 2) {
			parameters[line[0]] = line[1];
		}
	}
}

/**
 * Parse the data divided by special divisor in form data and then insert them in the parameters map in the httpMessage
 */
void http::parseFormData(const std::string &params, std::string &divisor, std::map<std::string, std::string> &parameters) {

	PROFILE_FUNCTION();

	std::vector<std::string> temp;

	auto datas = split(params, divisor);

	// the last place always contains "--", no real data in here
	datas.pop_back();
	datas.erase(datas.begin());

	for (size_t i = 0; i < datas.size(); i++) {
		// 0 |||||                    1                        ||||||2|||||  3   | 4  |
		//   \r\n|Content-Disposition: form-data; name="field1"|\r\n| \r\n|value1|\r\n|
		//                            0                                      1
		std::vector<std::string> option_value = split(datas[i], "\r\n");
		option_value.pop_back();                      // pos 4
		option_value.erase(option_value.begin() + 2); // pos 2
		option_value.erase(option_value.begin());     // pos 0

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