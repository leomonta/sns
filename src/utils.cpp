#include "utils.hpp"

#include <stdexcept>
#include <ctime>
#include <cstring>
#include <vector>
#include <zlib.h>

std::vector<std::string> split(const std::string &source, const std::string &sep) {

	std::vector<std::string> res;
	std::string              haystack(source);

	size_t      pos = 0;
	std::string token;
	// find where the token is in the source
	while ((pos = haystack.find(sep)) != std::string::npos) {
		// retrive the string before it
		token = haystack.substr(0, pos);
		// insert it in the result
		res.push_back(token);
		// remove it from the source
		haystack.erase(0, pos + sep.length());
	}
	// insert the last substring
	res.push_back(haystack);

	return res;
}

std::string getUTC() {

	// Get the date in UTC/GMT
	time_t unixtime;
	tm     UTC;
	char   buffer[80];

	// set unix time on unixtime
	time(&unixtime);
	gmtime_r(&unixtime, &UTC);

	// format based on rfc822 revision rfc1123
	strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", &UTC);

	return std::string(buffer);
}

void urlDecode(char *dst, const char *src) {

	char a, b;
	while (*src) {
		if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
			if (a >= 'a')
				a -= 'a' - 'A';
			if (a >= 'A')
				a -= ('A' - 10);
			else
				a -= '0';
			if (b >= 'a')
				b -= 'a' - 'A';
			if (b >= 'A')
				b -= ('A' - 10);
			else
				b -= '0';
			*dst++ = static_cast<char>(16 * a) + b;
			src += 3;
		} else if (*src == '+') {
			*dst++ = ' ';
			src++;
		} else {
			*dst++ = *src++;
		}
	}
	*dst++ = '\0';
}

void compressGz(const stringRefConst data, std::string &output) {

	z_stream deflate_s;
	deflate_s.zalloc   = Z_NULL;
	deflate_s.zfree    = Z_NULL;
	deflate_s.opaque   = Z_NULL;
	deflate_s.avail_in = 0;
	deflate_s.next_in  = Z_NULL;

	constexpr int window_bits = 15 + 16; // gzip with windowbits of 15

	constexpr int mem_level = 8;

	if (deflateInit2(&deflate_s, Z_BEST_COMPRESSION, Z_DEFLATED, window_bits, mem_level, Z_DEFAULT_STRATEGY) != Z_OK) {
		throw std::runtime_error("deflate init failed");
	}

	deflate_s.next_in  = (Bytef *)data.str;
	deflate_s.avail_in = (uInt)data.len;

	std::size_t size_compressed = 0;
	do {
		size_t increase = data.len / 2 + 1024;
		if (output.size() < (size_compressed + increase)) {
			output.resize(size_compressed + increase);
		}

		deflate_s.avail_out = static_cast<unsigned int>(increase);
		deflate_s.next_out  = reinterpret_cast<Bytef *>((&output[0] + size_compressed));

		deflate(&deflate_s, Z_FINISH);
		size_compressed += (increase - deflate_s.avail_out);
	} while (deflate_s.avail_out == 0);

	deflateEnd(&deflate_s);
	output.resize(size_compressed);
}

void simpleMemcpy(char *src, char *dst, size_t size) {

	while (size > 0) {
		*dst++ = *src++;
		--size;
	}
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

	simpleMemcpy(nStart, str, size);

	str[size] = '\0';
}

const char *strnstr(const char *haystack, const char *needle, const size_t count) {

	if (count == 0) {
		return nullptr;
	}

	auto Itr = 0;

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

stringRefConst trim(stringRefConst &strRef) {
	size_t newStart;

	for (newStart = 0; newStart < strRef.len; ++newStart) {
		if (strRef.str[newStart] != ' ') {
			break;
		}
	}

	stringRefConst res = {
	    strRef.str + newStart,
	    strRef.len - newStart
	};

	size_t newEnd;
	for (newEnd = 0; newEnd < res.len; ++newEnd) {
		if (res.str[res.len - newEnd - 1] != ' ') {
			break;
		}
	}
	res.len -= newEnd;

	return res;
}

bool isEmpty(const stringRefConst &strRef) {

	if (strRef.len == 0) {
		return true;
	}

	for (size_t i = 0; i < strRef.len; ++i) {
		// is space check for
		// space, horizontal tab, and whitespaces (\n \r \v \f)
		if (!isspace(strRef.str[i])) {
			return false;
		}
	}

	return true;
}

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

char *makeCopy(const stringRef &str) {
	auto cpy = static_cast<char *>(malloc(str.len));
	memcpy(cpy, str.str, str.len);
	return cpy;
}

char *makeCopyConst(const stringRefConst &str) {
	auto cpy = static_cast<char *>(malloc(str.len));
	memcpy(cpy, str.str, str.len);
	return cpy;
}

bool streq(const stringRefConst &lhs, const stringRefConst &rhs) {
	if (lhs.len != rhs.len) {
		return false;
	}

	for (size_t i = 0; i < lhs.len; ++i) {
		if (lhs.str[i] != rhs.str[i]) {
			return false;
		}
	}

	return true;
}

bool streq_str(const char *lhs, const char *rhs) {

	for (; *lhs != '\0' && *rhs != '\0'; ++lhs, ++rhs) {
		if (*lhs != *rhs) {
			return false;
		}
	}

	return true;
}
