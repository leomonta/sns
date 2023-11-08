#include "httpMessage.hpp"

#include "profiler.hpp"
#include "utils.hpp"

#include <logger.hpp>
#include <stdio.h>
#include <string.h>

void logMalformedParameter(const stringRef &strRef) {
	// malformed parameter

	// make a temporary buffer to print it with a classical printf
	char *temp = (char *)malloc(strRef.len + 1); // account for the null terminator
	memcpy(temp, strRef.str, strRef.len);        // copy the data
	temp[strRef.len] = '\0';                     // null terminator
	log(LOG_WARNING, "Malformed parameter -> '%s' \n", temp);
	free(temp); // and free
}

const stringRefConst headerRqStr[] = {
    TO_STRINGREF("A-IM"),
    TO_STRINGREF("Accept"),
    TO_STRINGREF("Accept-Charset"),
    TO_STRINGREF("Accept-Datetime"),
    TO_STRINGREF("Accept-Encoding"),
    TO_STRINGREF("Accept-Language"),
    TO_STRINGREF("Access-Control-Request-Method"),
    TO_STRINGREF("Access-Control-Request-Headers"),
    TO_STRINGREF("Authorization"),
    TO_STRINGREF("Cache-Control"),
    TO_STRINGREF("Connection"),
    TO_STRINGREF("Content-Encoding"),
    TO_STRINGREF("Content-Length"),
    TO_STRINGREF("Content-MD5"),
    TO_STRINGREF("Content-Type"),
    TO_STRINGREF("Cookie"),
    TO_STRINGREF("Date"),
    TO_STRINGREF("Expect"),
    TO_STRINGREF("Forwarded"),
    TO_STRINGREF("From"),
    TO_STRINGREF("Host"),
    TO_STRINGREF("HTTP2-Settings"),
    TO_STRINGREF("If-Match"),
    TO_STRINGREF("If-Modified-Since"),
    TO_STRINGREF("If-None-Match"),
    TO_STRINGREF("If-Range"),
    TO_STRINGREF("If-Unmodified-Since"),
    TO_STRINGREF("Max-Forwards"),
    TO_STRINGREF("Origin"),
    TO_STRINGREF("Pragma"),
    TO_STRINGREF("Prefer"),
    TO_STRINGREF("Proxy-Authorization"),
    TO_STRINGREF("Range"),
    TO_STRINGREF("Referer"),
    TO_STRINGREF("TE"),
    TO_STRINGREF("Trailer"),
    TO_STRINGREF("Transfer-Encoding"),
    TO_STRINGREF("User-Agent"),
    TO_STRINGREF("Upgrade"),
    TO_STRINGREF("Via"),
    TO_STRINGREF("Warning"),
};

const stringRefConst headerRpStr[] = {
    TO_STRINGREF("Accept-CH"),
    TO_STRINGREF("Access-Control-Allow-Origin"),
    TO_STRINGREF("Access-Control-Allow-Credentials"),
    TO_STRINGREF("Access-Control-Expose-Headers"),
    TO_STRINGREF("Access-Control-Max-Age"),
    TO_STRINGREF("Access-Control-Allow-Methods"),
    TO_STRINGREF("Access-Control-Allow-Headers"),
    TO_STRINGREF("Accept-Patch"),
    TO_STRINGREF("Accept-Ranges"),
    TO_STRINGREF("Age"),
    TO_STRINGREF("Allow"),
    TO_STRINGREF("Alt-Svc"),
    TO_STRINGREF("Cache-Control"),
    TO_STRINGREF("Connection"),
    TO_STRINGREF("Content-Disposition"),
    TO_STRINGREF("Content-Encoding"),
    TO_STRINGREF("Content-Language"),
    TO_STRINGREF("Content-Length"),
    TO_STRINGREF("Content-Location"),
    TO_STRINGREF("Content-MD5"),
    TO_STRINGREF("Content-Range"),
    TO_STRINGREF("Content-Type"),
    TO_STRINGREF("Date"),
    TO_STRINGREF("Delta-Base"),
    TO_STRINGREF("ETag"),
    TO_STRINGREF("Expires"),
    TO_STRINGREF("IM"),
    TO_STRINGREF("Last-Modified"),
    TO_STRINGREF("Link"),
    TO_STRINGREF("Location"),
    TO_STRINGREF("P3P"),
    TO_STRINGREF("Pragma"),
    TO_STRINGREF("Preference-Applied"),
    TO_STRINGREF("Proxy-Authenticate"),
    TO_STRINGREF("Public-Key-Pins"),
    TO_STRINGREF("Retry-After"),
    TO_STRINGREF("Server"),
    TO_STRINGREF("Set-Cookie"),
    TO_STRINGREF("Strict-Transport-Security"),
    TO_STRINGREF("Trailer"),
    TO_STRINGREF("Transfer-Encoding"),
    TO_STRINGREF("Tk"),
    TO_STRINGREF("Upgrade"),
    TO_STRINGREF("Vary"),
    TO_STRINGREF("Via"),
    TO_STRINGREF("Warning"),
    TO_STRINGREF("WWW-Authenticate"),
    TO_STRINGREF("X-Frame-Options"),
    TO_STRINGREF("Status"),
};

httpMessage::httpMessage(std::string &message) {
	httpMessage(message.c_str());
}

httpMessage::httpMessage(const char *message) {

	PROFILE_FUNCTION();

	// use strlen so i'm sure it's not counting the null terminator
	auto msgLen = strlen(message);

	// save the message in a local pointer so i don't rely on std::string allocation plus a null terminator
	rawMessage_a = static_cast<char *>(malloc(msgLen + 1));
	memcpy(rawMessage_a, message, msgLen + 1);

	// body and header are divided by two newlines
	auto msgSeparator = strstr(rawMessage_a, "\r\n\r\n");

	unsigned long hLen = msgSeparator - rawMessage_a;

	stringRef header;

	// strstr return 0 if nothing is found or the given pointer if it is an empty string
	if (msgSeparator == 0 || msgSeparator == rawMessage_a) {
		header = {nullptr, 0};
		body   = {nullptr, 0};
	} else {
		header = {rawMessage_a, hLen};
		body   = {rawMessage_a + hLen, msgLen - hLen};
	}

	// now i have the two stringRefs to the body and the header

	// extract metadata parameters and other stuff
	http::decompileHeader(header, *this);

	http::decompileMessage(headerOptions[http::RQ_Content_Type], this, body);
}

httpMessage::~httpMessage() {
	if (rawMessage_a != nullptr) {
		free(rawMessage_a);
	}

	if (dir == DIR_INBOUND) {
		// that's all folks
		return;
	}

	for (const auto &[key, val] : headerOptions) {
		free(val.str);
	}

	free(url.str);
	free(body.str);
}

void addToOptions(stringRef key, stringRef val, httpMessage *ctx) {
	ctx->headerOptions[http::getParameterCode(key)] = val;
}

void addToParams(stringRef key, stringRef val, httpMessage *ctx) {
	ctx->parameters[key] = val;
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
		parseOptions(queryParams, addToParams, "&", '=', &msg);

		// limit thw url to before the '?'
		msg.url.len -= msg.url.len - qPos;
	}

	parseOptions(rawHeader, addToOptions, "\r\n", ':', &msg);
}

/**
 * decompile message in header and body
 */
void http::decompileMessage(const stringRef &cType, httpMessage *msg, stringRef &body) {

	PROFILE_FUNCTION();

	// the content type indicates how parameters are showed
	if (isEmpty(cType)) {
		return;
	}

	// the fields are separated from each others with a "&" and key -> value are separated with "="
	// plus they are encoded as a URL
	if (!strncmp("application/x-www-form-urlencoded", cType.str, cType.len)) {
		parseOptions(body, addToParams, "&", '=', msg);
	} else if (strncmp("text/plain", cType.str, cType.len)) {
		parseOptions(body, addToParams, "\r\n", '=', msg);
	} else if (strnstr("multipart/form-data;", cType.str, cType.len) != nullptr) {
		// find the divisor
		//               0              |      1
		// multipart/form-data; boundary=----asdwadawd
		// auto divisor = split(cType, "=")[1];
		auto eq = strnchr(cType.str, '=', cType.len);
		// sometimes the boundary is encolsed in quotes, take care of this case
		if (eq == nullptr) {
			log(LOG_WARNING, "Malformed multipart form data, no '='\n");
		}

		size_t         temp    = cType.str + cType.len - eq;
		stringRefConst divisor = {eq, temp};

		if (*eq == '"') {
			++divisor.str;
			--divisor.len;
		}

		// http::parseFormData(body, divisor, parameters);
	}
}

/**
 * unite the header and the body in a single message
 */
stringRef http::compileMessage(const httpMessage &msg) {

	PROFILE_FUNCTION();

	const int STATUS_LINE_LEN = 15;

	// The value to modify are at       0123456789
	char statusLine[STATUS_LINE_LEN + 1] = "HTTP/1.0 XXX \r\n";

	// the 2 is for the \r\n separator
	auto msgLen = STATUS_LINE_LEN + msg.headerLen + 2 + msg.body.len;

	char *res = new char[msgLen];

	memcpy(res, statusLine, STATUS_LINE_LEN);

	res[11] = '0' + (char)((msg.statusCode) % 10);
	res[10] = '0' + (char)((msg.statusCode / 10) % 10);
	res[9]  = '0' + (char)((msg.statusCode / 100) % 10);

	char *writer = res + STATUS_LINE_LEN;

	// add all headers
	for (auto const &[key, val] : msg.headerOptions) {

		// the format is
		// name: value\r\n
		memcpy(writer, headerRpStr[key].str, headerRpStr[key].len);
		writer += headerRpStr[key].len;

		memcpy(writer, ": ", 2);
		writer += 2;

		memcpy(writer, val.str, val.len);
		writer += val.len;

		memcpy(writer, "\r\n", 2);
		writer += 2;
	}

	// headr body separator
	memcpy(writer, "\r\n", 2);
	writer += 2;

	memcpy(writer, msg.body.str, msg.body.len);
	writer += msg.body.len;

	return {res, msgLen};
}

u_char http::getMethodCode(const stringRef &requestMethod) {

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

u_char http::getVersionCode(const stringRef &httpVersion) {

	// Yep that's it, such logic pt 2.0

	auto htvs = httpVersion.str;
	auto htvl = httpVersion.len;

	if (htvl == 0) {
		return HTTP_INVALID;
	}

	if (!strncmp(htvs, "undefined", htvl)) {
		return HTTP_10;
	} else if (!strncmp(htvs, "HTTP/0.9", htvl)) {
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

u_char http::getParameterCode(const stringRef &parameter) {

	// thi is one hell of a elif chain
	// lickily i need to check only for the requests headers
	// I'm becoming yan dev

	for (u_char i = 0; i < RQ_ENUM_LEN; ++i) {
		if (strncmp(parameter.str, headerRqStr[i].str, parameter.len) == 0) {
			return i;
		}
	}

	return -1;
}

/**
 * Parse the options found in the stringRef given and call the function with them
 * @param head a stringRef pointing to the start of the options
 * @param headerOptions the map where to put the parsed strings
 */
void http::parseOptions(const stringRef &head, void (*fun)(stringRef a, stringRef b, httpMessage *ctx), const char *chunkSep, const char itemSep, httpMessage *ctx) {

	// parse the remaining header options

	const auto addLen = strlen(chunkSep); // the lenght to move after the string is found

	auto       startOptions      = strnchr(head.str, *chunkSep, head.len) + addLen; // the plus 1 is needed to start after the \r\n
	size_t     lenToStartOptions = startOptions - head.str;
	stringRef  chunk             = {startOptions, lenToStartOptions};
	const auto limit             = head.str + head.len;
	auto       limitLen          = head.len - lenToStartOptions;

	while (chunk.str < limit) {

		auto crlf = strnchr(chunk.str, *chunkSep, limitLen);

		if (crlf == nullptr) {
			chunk.len = limitLen;
		} else {
			chunk.len = crlf - chunk.str;
		}

		// printStringRef(chunk);

		auto sep = strnchr(chunk.str, itemSep, chunk.len);
		if (sep == nullptr) {
			logMalformedParameter(chunk);
		} else {

			// everything is fine here

			size_t    pos = sep - chunk.str;                // get the pointer difference from the start of the chunk and the position of the '='
			stringRef key = {chunk.str, pos};               // from the start of the chunk to before the '='
			stringRef val = {sep + 1, chunk.len - pos - 1}; // from after the '=' to the end of the chunk

			trim(key);
			trim(val);

			// printStringRef(key);
			// printStringRef(val);

			// check if we actually have a key, the value can be empty
			if (strcmp(key.str, "") != 0) {
				// finally put it in the map
				fun(key, val, ctx);
			}
		}

		// at the end of the chunk
		chunk.str = chunk.str + chunk.len + addLen; // move at the end of the string, over the ampersand
		limitLen -= chunk.len + addLen;             // recalculate the limitLen (limitLen = limitLen - len - 1) -> limitLen = limitLen - (len + 1)
		chunk.len = 0;                              // just to be sure
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

void http::addHeaderOption(const u_char option, const stringRefConst &value, httpMessage &msg) {
	auto opt = headerRpStr[option];

	stringRef cpy;
	cpy.str                   = makeCopyConst(value);
	cpy.len                   = value.len;
	msg.headerOptions[option] = cpy;

	// account the added data
	//                 name     ': ' value '\r\n'
	msg.headerLen += (opt.len + 2 + cpy.len + 2);
}

void http::setUrl(const stringRefConst &val, httpMessage &msg) {
	msg.url = {makeCopyConst(val), val.len};
}