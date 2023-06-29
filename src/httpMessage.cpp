#include "httpMessage.hpp"

#include "profiler.hpp"
#include "utils.hpp"

#include <string.h>

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

httpMessage::httpMessage(std::string &message) {
	httpMessage(message.c_str());
}

httpMessage::httpMessage(const char *message) {

	PROFILE_FUNCTION();

	auto msgLen = strlen(message);

	rawMessage = new char[msgLen + 1];
	memcpy(rawMessage, message, msgLen + 1);

	// body and header are divided by two newlines
	auto bodyHead_meet = strstr(rawMessage, "\r\n\r\n");

	unsigned long bodyHead_diff = bodyHead_meet - rawMessage;

	stringRef header;
	stringRef body;

	// strstr return 0 if nothing is found or the given pointer if it is an empty string
	if (bodyHead_meet == 0 || bodyHead_meet == rawMessage) {
		header = {nullptr, 0};
		body   = {nullptr, 0};
	} else {
		header = {rawMessage, bodyHead_diff};
		body   = {rawMessage + bodyHead_diff, msgLen - bodyHead_diff};
	}

	// the +4 is to account for the length of the string searched

	http::decompileHeader(header, *this);

	// http::decompileMessage(header[http::RQ_Content_Type], parameters, body);
}

httpMessage::~httpMessage() {
	delete[] rawMessage;
}

/**
 * deconstruct the raw header in a map with key (option) -> value
 */
void http::decompileHeader(const stringRef &rawHeader, httpMessage &msg) {

	PROFILE_FUNCTION();

	// the first line should be "METHOD URL HTTP/Version"

	//  0 |1|    2
	// GET / HTTP/1.1\r\n
	size_t firstSpaceLen  = strstr(rawHeader.str, " ") - rawHeader.str;                                           // gets me until the space after the verb
	size_t secondSpaceLen = strstr(rawHeader.str + firstSpaceLen + 1, " ") - (rawHeader.str + firstSpaceLen + 1); // untile the space after the URL
	size_t endlineLen     = strstr(rawHeader.str, "\r\n") - (rawHeader.str + firstSpaceLen + secondSpaceLen + 2); // until the first newline

	stringRef methodRef  = {rawHeader.str, firstSpaceLen};
	stringRef versionRef = {rawHeader.str + secondSpaceLen + firstSpaceLen + 2, endlineLen};

	msg.method  = http::getMethodCode(methodRef);
	msg.url     = {rawHeader.str + firstSpaceLen + 1, secondSpaceLen}; // line[1];
	msg.version = http::getVersionCode(versionRef);

	// the position of the query parameter market (if present)
	// auto   qPos = msg.url.find("?");
	size_t qPos = strnchr(msg.url.str, '?', msg.url.len) - msg.url.str;

	// if '?' is in the url, it means that query parameters are used
	if (qPos != 0) {
		// get the entire query
		stringRef queryParams = {msg.url.str + qPos + 1, msg.url.len - qPos - 1};
		parseQueryParameters(queryParams, msg.parameters);

		// erase the '?' and everything after it
		// msg.url.erase(msg.url.begin() + qPos);
	}
	/*

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
	}*/
}

/**
 * decompile message in header and body
 */
void http::decompileMessage(const std::string &cType, std::unordered_map<std::string, std::string> &parameters, std::string &body) {

	PROFILE_FUNCTION();

	// the content type indicates how parameters are showed
	if (!cType.empty()) {

		// the fields are separated from each others with a "&" and key -> value are separated with "="
		// plus they are encoded as a URL
		if (cType == "application/x-www-form-urlencoded") {
			// http::parseQueryParameters(body, parameters);
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
std::string http::compileMessage(const std::unordered_map<int, std::string> &header, const std::string &body) {

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

int http::getMethodCode(const stringRef &requestMethod) {

	PROFILE_FUNCTION();

	auto rqms = requestMethod.str;
	auto rqml = requestMethod.len;

	if (rqml == 0) {
		return HTTP_INVALID;
	}

	// Yep that's it, such logic
	if (strncmp(rqms, "GET", rqml) == 0) {
		return HTTP_GET;
	} else if (strncmp(rqms, "HEAD", rqml) == 0) {
		return HTTP_HEAD;
	} else if (strncmp(rqms, "POST", rqml) == 0) {
		return HTTP_POST;
	} else if (strncmp(rqms, "PUT", rqml) == 0) {
		return HTTP_PUT;
	} else if (strncmp(rqms, "DELETE", rqml) == 0) {
		return HTTP_DELETE;
	} else if (strncmp(rqms, "OPTIONS", rqml) == 0) {
		return HTTP_OPTIONS;
	} else if (strncmp(rqms, "CONNECT", rqml) == 0) {
		return HTTP_CONNECT;
	} else if (strncmp(rqms, "TRACE", rqml) == 0) {
		return HTTP_TRACE;
	} else if (strncmp(rqms, "PATCH", rqml) == 0) {
		return HTTP_PATCH;
	}

	return HTTP_INVALID;
}

int http::getVersionCode(const stringRef &httpVersion) {

	// Yep that's it, such logic pt 2.0

	auto htvs = httpVersion.str;
	auto htvl = httpVersion.len;

	if (htvl == 0) {
		return HTTP_INVALID;
	}

	if (strncmp(htvs, "HTTP/0.9", htvl) == 0) {
		return HTTP_09;
	} else if (strncmp(htvs, "HTTP/1.0", htvl) == 0) {
		return HTTP_10;
	} else if (strncmp(htvs, "HTTP/1.1", htvl) == 0) {
		return HTTP_11;
	} else if (strncmp(htvs, "HTTP/2", htvl) == 0) {
		return HTTP_2;
	} else if (strncmp(htvs, "HTTP/3", htvl) == 0) {
		return HTTP_3;
	}

	return HTTP_INVALID;
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
void http::parseQueryParameters(const stringRef &query, std::unordered_map<stringRef, stringRef> &parameters) {

	PROFILE_FUNCTION();

	// each parameter is separeted by the '&'
	// auto datas = split(query, "&");

	for (size_t i = 0; i < query.len; ++i) {
		// temp string to store the decoded value
		/*
		const char *src = datas[i].c_str();
		char       *dst = new char[strlen(src) + 1];
		urlDecode(dst, src);

		//  0 |  1
		// key=value
		temp = split(dst, "=");

		delete[] dst;
		*/

		auto      eq    = strnchr(query.str, '=', query.len);
		size_t    pos   = eq - query.str;
		stringRef key   = {query.str, pos};
		stringRef value = {eq + 1, query.len - pos};

		if (eq != nullptr && strcmp(key.str, "") != 0) {
			parameters[key] = value;
		}
	}
}

/**
 * Parse the data not encoded in form data and then insert them in the parameters map in the httpMessage
 */
void http::parsePlainParameters(const std::string &params, std::unordered_map<std::string, std::string> &parameters) {

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
void http::parseFormData(const std::string &params, std::string &divisor, std::unordered_map<std::string, std::string> &parameters) {

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