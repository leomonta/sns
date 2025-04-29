#pragma once

#include "stringRef.hpp"

#include <cstring>
#include <sys/types.h>

const static stringRef methods_strR[] = {
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

// Beautifully compile-time evaluated lookup tables for header options
const static stringRef headerRqStr[] = {
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

const static stringRef headerRpStr[] = {
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

// keeping this for consistency with the other strings
// but this should not be used
const static stringRef reasonPhrases[] = {
    TO_STRINGREF("Continue"),
    TO_STRINGREF("Switching Protocols"),
    TO_STRINGREF("OK"),
    TO_STRINGREF("Created"),
    TO_STRINGREF("Accepted"),
    TO_STRINGREF("Non-Authoritative Information"),
    TO_STRINGREF("No Content"),
    TO_STRINGREF("Reset Content"),
    TO_STRINGREF("Partial Content"),
    TO_STRINGREF("Multiple Choices"),
    TO_STRINGREF("Moved Permanently"),
    TO_STRINGREF("Found"),
    TO_STRINGREF("See Other"),
    TO_STRINGREF("Not Modified"),
    TO_STRINGREF("Use Proxy"),
    TO_STRINGREF("Temporary Redirect"),
    TO_STRINGREF("Bad Request"),
    TO_STRINGREF("Unauthorized"),
    TO_STRINGREF("Payment Required"),
    TO_STRINGREF("Forbidden"),
    TO_STRINGREF("Not Found"),
    TO_STRINGREF("Method Not Allowed"),
    TO_STRINGREF("Not Acceptable"),
    TO_STRINGREF("Proxy Authentication Required"),
    TO_STRINGREF("Request Time-out"),
    TO_STRINGREF("Conflict"),
    TO_STRINGREF("Gone"),
    TO_STRINGREF("Length Required"),
    TO_STRINGREF("Precondition Failed"),
    TO_STRINGREF("Request Entity Too Large"),
    TO_STRINGREF("Request-URI Too Large"),
    TO_STRINGREF("Unsupported Media Type"),
    TO_STRINGREF("Requested range not satisfiable"),
    TO_STRINGREF("Expectation Failed"),
    TO_STRINGREF("Internal Server Error"),
    TO_STRINGREF("Not Implemented"),
    TO_STRINGREF("Bad Gateway"),
    TO_STRINGREF("Service Unavailable"),
    TO_STRINGREF("Gateway Time-out"),
    TO_STRINGREF("HTTP Version not supported")};

inline stringRef getReasonPhrase(u_short statusCode) {

	switch (statusCode) {
	case 100:
		return TO_STRINGREF("Continue");
	case 101:
		return TO_STRINGREF("Switching Protocols");
	case 200:
		return TO_STRINGREF("OK");
	case 201:
		return TO_STRINGREF("Created");
	case 202:
		return TO_STRINGREF("Accepted");
	case 203:
		return TO_STRINGREF("Non-Authoritative Information");
	case 204:
		return TO_STRINGREF("No Content");
	case 205:
		return TO_STRINGREF("Reset Content");
	case 206:
		return TO_STRINGREF("Partial Content");
	case 300:
		return TO_STRINGREF("Multiple Choices");
	case 301:
		return TO_STRINGREF("Moved Permanently");
	case 302:
		return TO_STRINGREF("Found");
	case 303:
		return TO_STRINGREF("See Other");
	case 304:
		return TO_STRINGREF("Not Modified");
	case 305:
		return TO_STRINGREF("Use Proxy");
	case 307:
		return TO_STRINGREF("Temporary Redirect");
	case 400:
		return TO_STRINGREF("Bad Request");
	case 401:
		return TO_STRINGREF("Unauthorized");
	case 402:
		return TO_STRINGREF("Payment Required");
	case 403:
		return TO_STRINGREF("Forbidden");
	case 404:
		return TO_STRINGREF("Not Found");
	case 405:
		return TO_STRINGREF("Method Not Allowed");
	case 406:
		return TO_STRINGREF("Not Acceptable");
	case 407:
		return TO_STRINGREF("Proxy Authentication Required");
	case 408:
		return TO_STRINGREF("Request Time-out");
	case 409:
		return TO_STRINGREF("Conflict");
	case 410:
		return TO_STRINGREF("Gone");
	case 411:
		return TO_STRINGREF("Length Required");
	case 412:
		return TO_STRINGREF("Precondition Failed");
	case 413:
		return TO_STRINGREF("Request Entity Too Large");
	case 414:
		return TO_STRINGREF("Request-URI Too Large");
	case 415:
		return TO_STRINGREF("Unsupported Media Type");
	case 416:
		return TO_STRINGREF("Requested range not satisfiable");
	case 417:
		return TO_STRINGREF("Expectation Failed");
	case 500:
		return TO_STRINGREF("Internal Server Error");
	case 501:
		return TO_STRINGREF("Not Implemented");
	case 502:
		return TO_STRINGREF("Bad Gateway");
	case 503:
		return TO_STRINGREF("Service Unavailable");
	case 504:
		return TO_STRINGREF("Gateway Time-out");
	case 505:
		return TO_STRINGREF("HTTP Version not supported");
	default:
		return TO_STRINGREF("UNKN");
	}
}
