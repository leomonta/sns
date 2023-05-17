#include "utils.hpp"

/**
* split the given string with a single token, and return the vector of the splitted strings
*/
std::vector<std::string> split(const std::string& source, const std::string& find) {
	std::vector<std::string> res;
	std::string haystack(source);

	size_t pos = 0;
	std::string token;
	// find where the token is in the source
	while ((pos = haystack.find(find)) != std::string::npos) {
		// retrive the string before it
		token = haystack.substr(0, pos);
		// insert it in the result
		res.insert(res.end(), token);
		// remove it from the source
		haystack.erase(0, pos + find.length());
	}
	// insert the last substring
	res.insert(res.end(), haystack);

	return res;
}

/**
* Simply get the time formetted following RFC822 regulation on GMT time
*/
std::string getUTC() {

	// Get the date in UTC/GMT
	time_t unixtime;
	tm UTC;
	char buffer[80];

	// set unix time on unixtime
	time(&unixtime);
	gmtime_r(&unixtime, &UTC);

	// format based on rfc822 revision rfc1123
	strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", &UTC);

	return std::string(buffer);
}

/**
* Decode url character (%20 => " ") to ascii character, NOT MINE, JUST COPY PASTED
* Thank you ThomasH, https://stackoverflow.com/users/2012498/thomash at https://stackoverflow.com/questions/2673207/c-c-url-decode-library/2766963,
*/
void urlDecode(char* dst, const char* src) {
	char a, b;
	while (*src) {
		if ((*src == '%') &&
			((a = src[1]) && (b = src[2])) &&
			(isxdigit(a) && isxdigit(b))) {
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
			*dst++ = 16 * a + b;
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


/**
* compress data to gzip
* used this (https://github.com/mapbox/gzip-hpp/blob/master/include/gzip/compress.hpp) as a reference
*/
void compressGz(std::string& output, const char* data, std::size_t size) {

	z_stream deflate_s;
	deflate_s.zalloc = Z_NULL;
	deflate_s.zfree = Z_NULL;
	deflate_s.opaque = Z_NULL;
	deflate_s.avail_in = 0;
	deflate_s.next_in = Z_NULL;

	constexpr int window_bits = 15 + 16; // gzip with windowbits of 15

	constexpr int mem_level = 8;

	if (deflateInit2(&deflate_s, Z_BEST_COMPRESSION, Z_DEFLATED, window_bits, mem_level, Z_DEFAULT_STRATEGY) != Z_OK) {
		throw std::runtime_error("deflate init failed");
	}

	deflate_s.next_in = (Bytef*) data;
	deflate_s.avail_in = (uInt) size;

	std::size_t size_compressed = 0;
	do {
		size_t increase = size / 2 + 1024;
		if (output.size() < (size_compressed + increase)) {
			output.resize(size_compressed + increase);
		}

		deflate_s.avail_out = static_cast<unsigned int>(increase);
		deflate_s.next_out = reinterpret_cast<Bytef*>((&output[0] + size_compressed));

		deflate(&deflate_s, Z_FINISH);
		size_compressed += (increase - deflate_s.avail_out);
	} while (deflate_s.avail_out == 0);

	deflateEnd(&deflate_s);
	output.resize(size_compressed);
}
