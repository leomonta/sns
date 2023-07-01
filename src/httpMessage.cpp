#include "httpMessage.hpp"

#include "logger.hpp"
#include "profiler.hpp"
#include "utils.hpp"

#include <stdio.h>
#include <string.h>

/**
 * Quick method to print a stringRef
 */
void printStringRef(const stringRef &strRef) {

	const char *str = strRef.str;

	for (size_t i = 0; i < strRef.len; ++i) {
		// there is no print function that print a portion of a string, so i print each character one by one
		putchar(*str);
		++str;
	}
	// newline is essential for flushing
	putchar('\n');
}

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

	// use strlen so i'm sure it's not counting the null terminator
	auto msgLen = strlen(message);

	// save the message in a local pointer so i don't rely on std::string allocation plus a null terminator
	rawMessage_a = new char[msgLen + 1];
	memcpy(rawMessage_a, message, msgLen + 1);

	// body and header are divided by two newlines
	auto msgSeparator = strstr(rawMessage_a, "\r\n\r\n");

	unsigned long headerLen = msgSeparator - rawMessage_a;

	stringRef header;
	stringRef body;

	// strstr return 0 if nothing is found or the given pointer if it is an empty string
	if (msgSeparator == 0 || msgSeparator == rawMessage_a) {
		header = {nullptr, 0};
		body   = {nullptr, 0};
	} else {
		header = {rawMessage_a, headerLen};
		body   = {rawMessage_a + headerLen, msgLen - headerLen};
	}

	// now i have the two stringRefs to the body and the header

	// extract metadata parameters and other stuff
	http::decompileHeader(header, *this);

	// http::decompileMessage(header[http::RQ_Content_Type], parameters, body);
}

httpMessage::~httpMessage() {
	delete[] rawMessage_a;
}

/**
 * deconstruct the raw header in a map with key (option) -> value
 * and fetch some Metadata such as verb, version, url, a parameters
 * @param rawHeader the stringRef containing the header as a char* string
 * @param msg the httpMessage to store the information extracted
 */
void http::decompileHeader(const stringRef &rawHeader, httpMessage &msg) {

	PROFILE_FUNCTION();

	// the first line should be "METHOD URL HTTP/Version"

	//  0 |1|    2
	// GET / HTTP/1.1\r\n
	size_t firstSpaceLen  = strnstr(rawHeader.str, " ", rawHeader.len) - rawHeader.str;                                           // gets me until the space after the method verb
	size_t secondSpaceLen = strnstr(rawHeader.str + firstSpaceLen + 1, " ", rawHeader.len) - (rawHeader.str + firstSpaceLen + 1); // until the space after the URL
	size_t endlineLen     = strnstr(rawHeader.str, "\r\n", rawHeader.len) - (rawHeader.str + firstSpaceLen + secondSpaceLen + 2); // until the first newline

	// temporary stringRefs
	stringRef methodRef  = {rawHeader.str, firstSpaceLen};
	stringRef versionRef = {rawHeader.str + secondSpaceLen + firstSpaceLen + 2, endlineLen};

	// actually storing the values in the object
	msg.method  = http::getMethodCode(methodRef);
	msg.url     = {rawHeader.str + firstSpaceLen + 1, secondSpaceLen}; // line[1];
	msg.version = http::getVersionCode(versionRef);

	// Now checks if there are query parameters

	// the position of the query parameter marker (if present)
	auto qPos = strnchr(msg.url.str, '?', msg.url.len) - msg.url.str;

	// if strnchr returns nullptr the result is negative, else is positive
	if (qPos > 0) {
		// confine the parameters in a single stringREf excluding the '?'
		stringRef queryParams = {msg.url.str + qPos + 1, msg.url.len - qPos - 1};
		parseQueryParameters(queryParams, msg.parameters);
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
	if (!strncmp(rqms, "GET", rqml)) {
		return HTTP_GET;
	} else if (!strncmp(rqms, "HEAD", rqml)) {
		return HTTP_HEAD;
	} else if (!strncmp(rqms, "POST", rqml)) {
		return HTTP_POST;
	} else if (!strncmp(rqms, "PUT", rqml)) {
		return HTTP_PUT;
	} else if (!strncmp(rqms, "DELETE", rqml)) {
		return HTTP_DELETE;
	} else if (!strncmp(rqms, "OPTIONS", rqml)) {
		return HTTP_OPTIONS;
	} else if (!strncmp(rqms, "CONNECT", rqml)) {
		return HTTP_CONNECT;
	} else if (!strncmp(rqms, "TRACE", rqml)) {
		return HTTP_TRACE;
	} else if (!strncmp(rqms, "PATCH", rqml)) {
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

	if (!strncmp(htvs, "HTTP/0.9", htvl)) {
		return HTTP_09;
	} else if (!strncmp(htvs, "HTTP/1.0", htvl)) {
		return HTTP_10;
	} else if (!strncmp(htvs, "HTTP/1.1", htvl)) {
		return HTTP_11;
	} else if (!strncmp(htvs, "HTTP/2", htvl)) {
		return HTTP_2;
	} else if (!strncmp(htvs, "HTTP/3", htvl)) {
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
 * Given a string of url encoded parameters parse and decode them, then insert them in the parameters map in the httpMessage
 * @param query the query parameters in the format key=value&key2=value2... as a stringRef
 * @param parameters the std::map where to put the key, value pairs as stringRefs
 */
void http::parseQueryParameters(const stringRef &query, std::unordered_map<stringRef, stringRef> &parameters) {

	PROFILE_FUNCTION();

	// If i'm being called it means that an ? has been found in the url
	// Still there might be nothing after it -> GET /index.html? HTTP/1.1

	stringRef  chunk    = {query.str, query.len}; // a key=value pair ignoring the '&'
	const auto limit    = query.str + query.len;  // the pshysical memory address pointing to the end of the query, to not overshoot
	auto       limitLen = query.len;              // from the start of the chunk to the end of the query params, to not overshoot
	                                              // as the chunks porgess this gets smaller

	// as long as we are in a safe space
	while (chunk.str < limit) {

		// find the chunk delimiter
		auto ampersand = strnchr(chunk.str, '&', limitLen);

		// and set the chunk length according to it
		if (ampersand == nullptr) {
			chunk.len = limitLen;
		} else {
			chunk.len = ampersand - chunk.str;
		}

		// search for the the equal sign
		auto eq = strnchr(chunk.str, '=', chunk.len);
		if (eq == nullptr) {
			// malformed parameter

			// make a temporary buffer to print it with a classical printf
			char *temp = (char *)malloc(chunk.len + 1); // account for the null terminator
			memcpy(temp, chunk.str, chunk.len);         // copy the data
			temp[chunk.len] = '\0';                     // null terminator
			log(LOG_WARNING, "Query Parameter malformed -> '%s' \n", temp);
			free(temp); // and free

		} else {

			// everything is fine here

			size_t    pos = eq - chunk.str;                // get the pointer difference from the start of the chunk and the position of the '='
			stringRef key = {chunk.str, pos};              // from the start of the chunk to before the '='
			stringRef val = {eq + 1, chunk.len - pos - 1}; // from after the '=' to the end of the chunk

			// check if we actually have a key, the value can be empty
			if (strcmp(key.str, "") != 0) {
				// finally put it in the map
				parameters[key] = val;
			}
		}

		// at the end of the chunk
		chunk.str = chunk.str + chunk.len + 1; // move at the end of the string, over the ampersand
		limitLen -= chunk.len + 1;             // recalculate the limitLen (limitLen = limitLen - len - 1) -> limitLen = limitLen - (len + 1)
		chunk.len = 0;                         // just to be sure
	}
}

/**
 * Parse the data not encoded in form data and then insert them in the parameters map in the httpMessage
 * @param params the parameters encoded in plain form
 * @param parameters the map where to put the key, value pairs found as stringRefs
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
 * 
 * @param params the parameters encodedn in form data 
 * @param divisor the unique divisor string used in the data
 * @param parameters the map where to put the parameters found
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