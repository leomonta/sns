#pragma once

#include "StringRef.h"
#include "constants.h"
#include "errcode.h"

#include <limits.h>
#include <string.h>

#define CLAMP_TO_POSITIVE_INT(sz) sz > INT_MAX ? INT_MAX - 1 : (int)(sz)

/**
 * simply get the time formetted following RFC822 regulation on GMT time
 *
 * @return a static thread local strig containing the date
 */
StringRef getUTC();

/**
 * Decode url character (e.g. %20 => " ") to ascii character, shamelessly copied from stackoverflow
 * copied from https://stackoverflow.com/questions/2673207/c-c-url-decode-library/2766963,
 *
 * @author Thank you ThomasH, https://stackoverflow.com/users/2012498/thomash
 *
 * Since the decoded url is always smaller than the encoded one this algo can be used in-place by providing the same stringRef / stringView
 * for both src and dst
 *
 * @param[in] `dst` where to put the decoded strin. If an error occurred the value written here should be discarded
 * @param[in] `src` the string to decode
 * @param[in] `do_plus` whether to replace `+` with ` `
 *
 * @return an error code describing if and which error occurred
 *
 * - `PARSING_HEADER__MALFORMED_PERCENT_ENCODING`: an incorrect percent encoding was found. The decocing stops mid editign and the value in dst should be discarded
 */
ERRFUNC url_decode(StringRef *dst, const StringView *src, bool do_plus);

/**
 * compress given data to gzip
 * used this (https://github.com/mapbox/gzip-hpp/blob/master/include/gzip/compress.hpp) as a reference
 * maximum 4GiB of data (2^32 B) are accepted
 *
 * @param[in] `data` the string reference pointing to the data to compress
 * @param[in] `output` where to put the result, the content of this `StringOwn` will be ignored and overwritten.
 *   If an error occurred its data should be ignored, else the pointer contained should be freed when no longer needed
 *
 *
 * @return an error code describing if and which error occurred
 *
 * - `INVALID_ARGUMENT`: `data`'s length exceed the maximum allowed
 *     No data available to return, `output` should be discarded
 * - `FAILED_ALLOCATION`: could not allocate the internal buffer
 *     No data available to return, `output` should be discarded
 * - `ZLIB__INIT_FAILED`: deflateInit failed.
 *     No data available to return, `output` should be discarded
 * - `ZLIB__COMPRESS_FAILED` deflate or deflateEnd failed.
 *     No data available to return, `output` should be discarded
 */
ERRFUNC compress_gz(const StringView *data, StringOwn *output);

/**
 * Thaks to https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
 * @author https://stackoverflow.com/users/9530/adam-rosenfield
 * plus some modification
 *
 * rewrite the string to be without spaces at the start and end
 * it places another null terminator if necessary
 *
 * @param[in] `str` the string to trim
 */
void trimwhitespace(char *str);

#if !defined(__FreeBSD__) // bsd has strnstr in string.h
/**
 * finds needle in haystack for at most `count` characters of haystack
 *
 * @param[in] `haystack` the string to search into
 * @param[in] `needle` the string to search for
 * @param[in] `count` the max amount of chars to check for in haystack
 *
 * @return a pointer to the first character of the occurence of needle in haystack,
 *   nullptr if not found or if count == 0
 *   haystack if `needle` == `nullptr`
 */
const char *strnstr(const char *haystack, const char *needle, const size_t count);

#endif

/**
 * finds the first occurrence of chr in the byte string pointed to by str to a max of count - 1
 *
 * @param[in] `str` the string to search into
 * @param[in] `chr` the char to search for
 * @param[in] `count` the max amount of chars to check for in str
 *
 * @return pointer to the character found or nullptr if no such character is found
 */
const char *strnchr(const char *str, int chr, const size_t count);

/**
 * finds the last occurrence of chr in the byte string pointed to by str to a max of count - 1
 *
 * @param[in] `str` the string to search into
 * @param[in] `chr` the char to search for
 * @param[in] `count` the max amount of chars to check for in str
 *
 * @return pointer to the character found or nullptr if no such character is found
 */
const char *strrnchr(const char *str, int chr, const size_t count);

/**
 * rework the stringRef to remove unwanted whitespaces in front or at the back of the content
 *
 * @param[in] `strRef` the stringRef to trim
 *
 * @return the trimmed stringRef
 */
StringRef trim_StringRef(StringRef *strRef);

/**
 * check if every character in the stringref is a space
 * a space if any of these -> space, horizontal tab, and whitespaces (\n \r \v \f)
 *
 * @param[in] `strRef` the stringref to check
 *
 * @return true if the string is empty
 */
bool strrefblank(const StringRef *strRef);

/**
 * Return the string representation of the given number
 *
 * @param[in] `number` the numbero to convert to string
 *
 * @return a static thread local string that represents the given number
 */
StringRef num_to_string(size_t number);

/**
 * returnrs if the two Response Options are equal
 *
 * @param[in] `lhs`, `rhs` the Response Options to compare
 *
 * @return true if they are identical, false otherwise
 */
bool compare_HTTPResponseOption(const HTTPResponseOption *lhs, const HTTPResponseOption *rhs);

StringView get_reason_phrase(unsigned short statusCode);
