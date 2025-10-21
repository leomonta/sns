#include "utils.h"

#include "logger.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

static char buffer[80];

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

void url_decode(StringOwn *dst, const StringRef *src) {

	char a, b;

	size_t write_index = 0;
	size_t read_index  = 0;

	while (read_index < src->len && write_index < dst->len) {

		// html whatever thingy to decode
		if (src->str[read_index] == '%' && read_index < src->len + 2) {
			a = src->str[read_index + 1];
			b = src->str[read_index + 2];
			if (isxdigit(a) && isxdigit(b)) {
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

	dst->str[write_index] = '\0';
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

	deflate_s.next_in  = (Bytef *)data->str;
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
		if (str_ref->str[newStart] != ' ') {
			break;
		}
	}

	StringRef res = {
	    str_ref->str + newStart,
	    str_ref->len - newStart};

	size_t newEnd;
	for (newEnd = 0; newEnd < res.len; ++newEnd) {
		if (res.str[res.len - newEnd - 1] != ' ') {
			break;
		}
	}
	res.len -= newEnd;

	return res;
}

bool strrefblnk(const StringRef *str_ref) {

	if (str_ref->len == 0) {
		return true;
	}

	for (size_t i = 0; i < str_ref->len; ++i) {
		// is space check for
		// space, horizontal tab, and whitespaces (\n \r \v \f)
		if (!isspace(str_ref->str[i])) {
			return false;
		}
	}

	return true;
}

char *copy_StringOwn(const StringOwn *str) {
	auto cpy = malloc(str->len);
	memcpy(cpy, str->str, str->len);
	return cpy;
}

char *copy_StringRef(const StringRef *str) {
	auto cpy = malloc(str->len);
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

StringOwn num_to_string(size_t number) {

	unsigned digits = get_num_digits(number);

	char *str = malloc(digits);

	// < digits cause (unsigned 0 - 1 = UINT_MAX)
	for (unsigned i = digits - 1; i < digits; --i) {
		str[i] = (char)(number % 10) + '0';
		number = number / 10;
	}

	return (StringOwn){str, digits};
}

StringRef get_reason_phrase(unsigned short statusCode) {

	switch (statusCode) {
	case 100:
		return CAST_STRINGREF("Continue");
	case 101:
		return CAST_STRINGREF("Switching Protocols");
	case 200:
		return CAST_STRINGREF("OK");
	case 201:
		return CAST_STRINGREF("Created");
	case 202:
		return CAST_STRINGREF("Accepted");
	case 203:
		return CAST_STRINGREF("Non-Authoritative Information");
	case 204:
		return CAST_STRINGREF("No Content");
	case 205:
		return CAST_STRINGREF("Reset Content");
	case 206:
		return CAST_STRINGREF("Partial Content");
	case 300:
		return CAST_STRINGREF("Multiple Choices");
	case 301:
		return CAST_STRINGREF("Moved Permanently");
	case 302:
		return CAST_STRINGREF("Found");
	case 303:
		return CAST_STRINGREF("See Other");
	case 304:
		return CAST_STRINGREF("Not Modified");
	case 305:
		return CAST_STRINGREF("Use Proxy");
	case 307:
		return CAST_STRINGREF("Temporary Redirect");
	case 400:
		return CAST_STRINGREF("Bad Request");
	case 401:
		return CAST_STRINGREF("Unauthorized");
	case 402:
		return CAST_STRINGREF("Payment Required");
	case 403:
		return CAST_STRINGREF("Forbidden");
	case 404:
		return CAST_STRINGREF("Not Found");
	case 405:
		return CAST_STRINGREF("Method Not Allowed");
	case 406:
		return CAST_STRINGREF("Not Acceptable");
	case 407:
		return CAST_STRINGREF("Proxy Authentication Required");
	case 408:
		return CAST_STRINGREF("Request Time-out");
	case 409:
		return CAST_STRINGREF("Conflict");
	case 410:
		return CAST_STRINGREF("Gone");
	case 411:
		return CAST_STRINGREF("Length Required");
	case 412:
		return CAST_STRINGREF("Precondition Failed");
	case 413:
		return CAST_STRINGREF("Request Entity Too Large");
	case 414:
		return CAST_STRINGREF("Request-URI Too Large");
	case 415:
		return CAST_STRINGREF("Unsupported Media Type");
	case 416:
		return CAST_STRINGREF("Requested range not satisfiable");
	case 417:
		return CAST_STRINGREF("Expectation Failed");
	case 500:
		return CAST_STRINGREF("Internal Server Error");
	case 501:
		return CAST_STRINGREF("Not Implemented");
	case 502:
		return CAST_STRINGREF("Bad Gateway");
	case 503:
		return CAST_STRINGREF("Service Unavailable");
	case 504:
		return CAST_STRINGREF("Gateway Time-out");
	case 505:
		return CAST_STRINGREF("HTTP Version not supported");
	default:
		return CAST_STRINGREF("UNKN");
	}
}

