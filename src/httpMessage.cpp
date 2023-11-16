#include "httpMessage.hpp"

#include "profiler.hpp"
#include "stringRef.hpp"
#include "utils.hpp"

#include <logger.hpp>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

void logMalformedParameter(const stringRefConst &strRef) {
	// malformed parameter

	// make a temporary buffer to print it with a classical printf
	char *temp = (char *)malloc(strRef.len + 1); // account for the null terminator
	memcpy(temp, strRef.str, strRef.len);        // copy the data
	temp[strRef.len] = '\0';                     // null terminator
	log(LOG_WARNING, "Malformed parameter -> '%s' \n", temp);
	free(temp); // and free
}

// Beautifully compile-time evaluated lookup tables for header options
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

inboundHttpMessage http::makeInboundMessage(const char *str) {

	PROFILE_FUNCTION();

	inboundHttpMessage res;

	// use strlen so i'm sure it's not counting the null terminator
	auto msgLen = strlen(str);

	// save the message in a local pointer so i don't rely on std::string allocation plus a null terminator
	auto temp = static_cast<char *>(malloc(msgLen + 1));
	memcpy(temp, str, msgLen + 1);

	res.m_rawMessage_a = temp;

	// body and header are divided by two newlines
	auto msgSeparator = strstr(res.m_rawMessage_a, "\r\n\r\n");

	unsigned long hLen = msgSeparator - res.m_rawMessage_a;

	stringRefConst header;

	// strstr return 0 if nothing is found or the given pointer if it is an empty string
	if (msgSeparator == 0 || msgSeparator == res.m_rawMessage_a) {
		header     = {nullptr, 0};
		res.m_body = {nullptr, 0};
	} else {
		header     = {res.m_rawMessage_a, hLen};
		res.m_body = {res.m_rawMessage_a + hLen, msgLen - hLen};
	}

	// now i have the two stringRefs to the body and the header

	// extract metadata parameters and other stuff
	http::decompileHeader(header, res);

	http::decompileMessage(res);

	return res;
}

void http::destroyInboundHttpMessage(inboundHttpMessage *mex) {

	if (mex->m_rawMessage_a != nullptr) {
		free(const_cast<char *>(mex->m_rawMessage_a));
	}
}

void http::destroyOutboundHttpMessage(outboundHttpMessage *mex) {

	for (const auto &[key, val] : mex->m_headerOptions) {
		free(val.str);
	}

	free(mex->m_body.str);
	free(mex->m_filename.str);
}

void addToOptions(stringRefConst key, stringRefConst val, inboundHttpMessage *ctx) {
	ctx->m_headerOptions[http::getParameterCode(key)] = val;
}

void addToParams(stringRefConst key, stringRefConst val, inboundHttpMessage *ctx) {
	ctx->m_parameters[key] = val;
}

void http::decompileHeader(const stringRefConst &rawHeader, inboundHttpMessage &msg) {

	PROFILE_FUNCTION();

	// the first line should be "METHOD URL HTTP/Version"

	//  0 |1|    2
	// GET / HTTP/1.1\r\n
	size_t firstSpaceLen  = strnstr(rawHeader.str, " ", rawHeader.len) - rawHeader.str;                                           // gets me until the space after the method verb
	size_t secondSpaceLen = strnstr(rawHeader.str + firstSpaceLen + 1, " ", rawHeader.len) - (rawHeader.str + firstSpaceLen + 1); // until the space after the URL
	size_t endlineLen     = strnstr(rawHeader.str, "\r\n", rawHeader.len) - (rawHeader.str + firstSpaceLen + secondSpaceLen + 2); // until the first newline

	// temporary stringRefs
	stringRefConst methodRef  = {rawHeader.str, firstSpaceLen};
	stringRefConst versionRef = {rawHeader.str + secondSpaceLen + firstSpaceLen + 2, endlineLen};

	// actually storing the values in the object
	msg.m_method  = http::getMethodCode(methodRef);
	msg.m_url     = {rawHeader.str + firstSpaceLen + 1, secondSpaceLen}; // line[1];
	msg.m_version = http::getVersionCode(versionRef);

	// Now checks if there are query parameters

	// the position of the query parameter marker (if present)
	auto qPos = strnchr(msg.m_url.str, '?', msg.m_url.len) - msg.m_url.str;

	// if strnchr returns nullptr the result is negative, else is positive
	if (qPos > 0) {
		// confine the parameters in a single stringREf excluding the '?'
		stringRefConst queryParams = {msg.m_url.str + qPos + 1, msg.m_url.len - qPos - 1};
		parseOptions(queryParams, addToParams, "&", '=', &msg);

		// limit thw url to before the '?'
		msg.m_url.len -= msg.m_url.len - qPos;
	}

	parseOptions(rawHeader, addToOptions, "\r\n", ':', &msg);
}

void http::decompileMessage(inboundHttpMessage &msg) {

	PROFILE_FUNCTION();

	auto cType = msg.m_headerOptions[http::RQ_Content_Type];

	// the content type indicates how parameters are showed
	if (isEmpty(cType)) {
		return;
	}

	// type one, url encoded
	if (strncmp("application/x-www-form-urlencoded", cType.str, cType.len) == 0) {
		// the fields are separated from each others with a "&" and key -> value are separated with "="
		// plus they are encoded as a URL
		parseOptions(msg.m_body, addToParams, "&", '=', &msg);

		// type two plain text
	} else if (strncmp("text/plain", cType.str, cType.len) == 0) {
		parseOptions(msg.m_body, addToParams, "\r\n", '=', &msg);

		// part three multipart form-data
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

stringRef http::compileMessage(const outboundHttpMessage &msg) {

	PROFILE_FUNCTION();

	// I preconstruct the status line so i don't have to do multiple allocations and string concatenations
	// The value to modify are at
	//                   0123456789
	char statusLine[] = "HTTP/1.0 XXX \r\n";

	// A symbolic name for how long the status line is
	const int STATUS_LINE_LEN = 15;

	// the 2 is for the \r\n separator
	auto msgLen = STATUS_LINE_LEN + msg.m_headerLen + 2 + msg.m_body.len;

	// the entire message length
	char *res = static_cast<char *>(malloc(msgLen));

	memcpy(res, statusLine, STATUS_LINE_LEN);

	// set the status code digit by digit
	res[11] = '0' + (char)((msg.m_statusCode) % 10);
	res[10] = '0' + (char)((msg.m_statusCode / 10) % 10);
	res[9]  = '0' + (char)((msg.m_statusCode / 100) % 10);

	char *writer = res + STATUS_LINE_LEN;

	// add all headers
	for (auto const &[key, val] : msg.m_headerOptions) {

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

	memcpy(writer, msg.m_body.str, msg.m_body.len);
	writer += msg.m_body.len;

	return {res, msgLen};
}

u_char http::getMethodCode(const stringRefConst &requestMethod) {

	PROFILE_FUNCTION();

	static stringRefConst methods_strR[] = {
	    TO_STRINGREF("HTTP_INVALID"),
	    TO_STRINGREF("GET"),
	    TO_STRINGREF("HEAD"),
	    TO_STRINGREF("POST"),
	    TO_STRINGREF("PUT"),
	    TO_STRINGREF("DELETE"),
	    TO_STRINGREF("OPTIONS"),
	    TO_STRINGREF("CONNECT"),
	    TO_STRINGREF("TRACE"),
	    TO_STRINGREF("PATCH"),
	};

	if (requestMethod.len == 0) {
		return HTTP_INVALID_METHOD;
	}

	// since the codes are sequential starting from 1
	// the 0 is for an invalid http method / verb
	for (u_char i = 1; i < methods::HTTP_ENUM_LEN; ++i) {
		if (streq(requestMethod, methods_strR[i])) {
			return i;
		}
	}

	return HTTP_INVALID_METHOD;
}

u_char http::getVersionCode(const stringRefConst &httpVersion) {

	PROFILE_FUNCTION();

	if (httpVersion.len == 0) {
		return HTTP_VER_UNKN;
	}

	static stringRefConst versions_strR[] = {
	    TO_STRINGREF("undefined"),
	    TO_STRINGREF("HTTP/0.9"),
	    TO_STRINGREF("HTTP/1.0"),
	    TO_STRINGREF("HTTP/1.1"),
	    TO_STRINGREF("HTTP/2"),
	    TO_STRINGREF("HTTP/3"),
	};

	for (u_char i = 1; i < versions::HTTP_VER_ENUM_LEN; ++i) {
		if (streq(httpVersion, versions_strR[i])) {
			return i;
		}
	}

	return HTTP_VER_UNKN;
}

u_char http::getParameterCode(const stringRefConst &parameter) {

	PROFILE_FUNCTION();

	for (u_char i = 0; i < RQ_ENUM_LEN; ++i) {
		if (streq(parameter, headerRqStr[i]) == 0) {
			return i;
		}
	}

	return -1;
}

void http::parseOptions(const stringRefConst &segment, void (*fun)(stringRefConst a, stringRefConst b, inboundHttpMessage *ctx), const char *chunkSep, const char itemSep, inboundHttpMessage *ctx) {

	PROFILE_FUNCTION();

	// parse the remaining header options

	const auto addLen = strlen(chunkSep); // the lenght to move after the string is found

	auto           startOptions      = strnchr(segment.str, *chunkSep, segment.len) + addLen; // the plus 1 is needed to start after the \r\n
	size_t         lenToStartOptions = startOptions - segment.str;
	stringRefConst chunk             = {startOptions, lenToStartOptions};
	const auto     limit             = segment.str + segment.len;
	auto           limitLen          = segment.len - lenToStartOptions;

	while (chunk.str < limit) {

		auto crlf = strnchr(chunk.str, *chunkSep, limitLen);

		if (crlf == nullptr) {
			chunk.len = limitLen;
		} else {
			chunk.len = crlf - chunk.str;
		}

		auto sep = strnchr(chunk.str, itemSep, chunk.len);
		if (sep == nullptr) {
			logMalformedParameter(chunk);
		} else {

			// everything is fine here
			size_t         pos = sep - chunk.str;                // get the pointer difference from the start of the chunk and the position of the '='
			stringRefConst key = {chunk.str, pos};               // from the start of the chunk to before the '='
			stringRefConst val = {sep + 1, chunk.len - pos - 1}; // from after the '=' to the end of the chunk

			key = trim(key);
			val = trim(val);

			// check if we actually have a key, the value can be empty
			if (streq_str(key.str, "")) {
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
 *
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
*/

void http::addHeaderOption(const u_char option, const stringRefConst &value, outboundHttpMessage &msg) {
	auto opt = headerRpStr[option];

	stringRef cpy{
	    makeCopyConst(value),
	    value.len};

	// if something is already present at the requested position
	auto old = msg.m_headerOptions[option];
	if (old.str != nullptr) {
		// free it
		free(old.str);
	}
	msg.m_headerOptions[option] = cpy;

	// account the bytes that will be added later
	//                 name     ': ' value '\r\n'
	msg.m_headerLen += (opt.len + 2 + cpy.len + 2);
}

void http::setFilename(const stringRefConst &val, outboundHttpMessage &msg) {
	msg.m_filename = {makeCopyConst(val), val.len};
}
