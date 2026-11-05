#include "utils.h"

#include "logger.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define ZLIB_CONST
#include <zlib.h>

// these are thread local to prevent an allocation, if the values need to persist past a print, or similar, they need to be stored somewhere else
static thread_local char buffer[80];
static thread_local char digits_str_buffer[41]; // enough to represent 2^128, null terminator and then some

StringRef getUTC() {

	// Get the date in UTC/GMT
	time_t    unixtime;
	struct tm UTC;

	// set unix time on unixtime
	time(&unixtime);
	gmtime_r(&unixtime, &UTC);

	// format based on rfc822 revision rfc1123
	strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", &UTC);

	return (StringRef){buffer, 80};
}

ERRFUNC url_decode(StringOwn *dst, const StringRef *src) {

	char a, b;

	size_t write_index = 0;
	size_t read_index  = 0;

	while (read_index < src->len && write_index < dst->len) {

		// html whatever thingy to decode
		if (src->str[read_index] == '%' && read_index + 2 < src->len) {
			a = src->str[read_index + 1];
			b = src->str[read_index + 2];
			if (isxdigit((unsigned char)a) && isxdigit((unsigned char)b)) {
				if (a >= 'a') {
					a -= 'a' - 'A';
				}
				if (a >= 'A') {
					a -= ('A' - 10);
				} else {
					a -= '0';
				}
				if (b >= 'a') {
					b -= 'a' - 'A';
				}
				if (b >= 'A') {
					b -= ('A' - 10);
				} else {
					b -= '0';
				}
				dst->str[write_index] = (char)(16 * a) + b;
				write_index++;
				read_index += 3;
			} else {
				return PARSING_HEADER__MALFORMED_URL_ENCODING;
			}
		} else if (src->str[read_index] == '+') {
			dst->str[write_index] = ' ';
			write_index++;
			read_index++;
		} else {
			dst->str[write_index] = src->str[read_index];
			write_index++;
			read_index++;
		}
	}

	dst->len = write_index;

	return NO_ERROR;
}

bool compress_gz(const StringRef *data, StringOwn *output) {

	z_stream deflate_s;
	deflate_s.zalloc   = Z_NULL;
	deflate_s.zfree    = Z_NULL;
	deflate_s.opaque   = Z_NULL;
	deflate_s.avail_in = 0;
	deflate_s.next_in  = Z_NULL;

	constexpr int window_bits = 15 + 16; // gzip with windowbits of 15

	constexpr int mem_level = 8;

	if (deflateInit2(&deflate_s, Z_BEST_COMPRESSION, Z_DEFLATED, window_bits, mem_level, Z_DEFAULT_STRATEGY) != Z_OK) {
		llog(LOG_ERROR, "[COMPRESS] Deflate init failed\n.");
		return false;
	}

	deflate_s.next_in  = (const Bytef *)data->str;
	deflate_s.avail_in = (uInt)data->len;

	output->len = 0;
	do {
		size_t increase = data->len / 2 + 1024;
		if (output->len < (output->len + increase)) {
			output->str = realloc(output->str, output->len + increase);
		}

		deflate_s.avail_out = (unsigned int)(increase);
		deflate_s.next_out  = (Bytef *)((output->str + output->len));

		deflate(&deflate_s, Z_FINISH);
		output->len += (increase - deflate_s.avail_out);
	} while (deflate_s.avail_out == 0);

	deflateEnd(&deflate_s);

	return true;
}

void trimwhitespace(char *str) {

	char *nEnd;
	char *nStart = str;

	// Trim leading space
	while (isspace((unsigned char)*nStart)) {
		nStart++;
	}

	// All spaces?
	if (*nStart == 0) {
		return;
	}

	// Trim trailing space
	nEnd = nStart + strlen(nStart) - 1;
	while (nEnd > nStart && isspace((unsigned char)*nEnd)) {
		nEnd--;
	}

	// Write new null terminator character
	nEnd[1] = '\0';

	auto size = strlen(nStart);

	memcpy(nStart, str, size);

	str[size] = '\0';
}

const char *strnstr(const char *haystack, const char *needle, const size_t count) {

	if (count == 0) {
		return nullptr;
	}

	size_t Itr = 0;

	for (size_t i = 0; i < count; ++i) {

		if (haystack[i] == needle[Itr]) {
			++Itr;
			if (needle[Itr] == '\0') {
				// match
				return haystack + i - Itr + 1;
			}

		} else {
			// reset back the search
			i -= Itr;
			Itr = 0;
		}
	}

	// no match
	return nullptr;
}

const char *strnchr(const char *str, int chr, const size_t count) {

	for (size_t i = 0; i < count; ++i) {
		if (*str == chr) {
			return str;
		}

		++str;
	}

	return nullptr;
}

const char *strrnchr(const char *str, int chr, const size_t count) {

	for (size_t i = count - 1; i < count; --i) {
		if (str[i] == chr) {
			return str + i;
		}
	}

	return nullptr;
}

size_t strnlen(const char *str, const size_t count) {
	size_t res = 0;

	while (res < count && str[res] != '\0') {
		++res;
	}

	return res;
}

StringRef trim(StringRef *str_ref) {
	size_t newStart;

	for (newStart = 0; newStart < str_ref->len; ++newStart) {
		if (!isspace(str_ref->str[newStart])) {
			break;
		}
	}

	StringRef res = {
	    str_ref->str + newStart,
	    str_ref->len - newStart};

	size_t newEnd;
	for (newEnd = 0; newEnd < res.len; ++newEnd) {
		if (!isspace(res.str[res.len - newEnd - 1])) {
			break;
		}
	}
	res.len -= newEnd;

	return res;
}

bool strrefblank(const StringRef *str_ref) {

	if (str_ref->len == 0) {
		return true;
	}

	for (size_t i = 0; i < str_ref->len; ++i) {
		// isspace checks for
		// space, horizontal tab, and whitespaces (\n \r \v \f)
		if (!isspace(str_ref->str[i])) {
			return false;
		}
	}

	return true;
}

char *copy_StringOwn(const StringOwn *str) {
	auto cpy = malloc(str->len);
	// TEST_ALLOC(cpy)
	memcpy(cpy, str->str, str->len);
	return cpy;
}

char *copy_StringRef(const StringRef *str) {
	auto cpy = malloc(str->len);
	// TEST_ALLOC(cpy)
	memcpy(cpy, str->str, str->len);
	return cpy;
}

// https://stackoverflow.com/questions/1068849/how-do-i-determine-the-number-of-digits-of-an-integer-in-c
unsigned char get_num_digits(size_t n) {
	unsigned char r = 1;

	if (n >= 10000000000000000) {
		r += 16;
		n /= 10000000000000000;
	}
	if (n >= 100000000) {
		r += 8;
		n /= 100000000;
	}
	if (n >= 10000) {
		r += 4;
		n /= 10000;
	}
	if (n >= 100) {
		r += 2;
		n /= 100;
	}
	if (n >= 10) {
		r++;
	}

	return r;
}

StringRef num_to_string(size_t number) {

	unsigned digits = get_num_digits(number);

	char *str = digits_str_buffer;

	// < digits cause (unsigned 0 - 1 = UINT_MAX)
	for (size_t i = (digits - 1); i < digits; --i) {
		str[i] = (char)(number % 10) + '0';
		number = number / 10;
	}
	str[digits] = '\0';

	return (StringRef){str, digits};
}

bool compare_u_char(const unsigned char *lhs, const unsigned char *rhs) {
	return *lhs == *rhs;
}


StringRef get_reason_phrase(unsigned short statusCode) {

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
