#pragma once

#include "StringRef.h"

#include <string.h>

/**
 * simply get the time formetted following RFC822 regulation on GMT time
 *
 * @return a strig containing the date
 */
StringRef getUTC();

/**
 * Decode url character (e.g. %20 => " ") to ascii character, shamelessly copied from stackoverflow
 * copied from https://stackoverflow.com/questions/2673207/c-c-url-decode-library/2766963,
 *
 * @author Thank you ThomasH, https://stackoverflow.com/users/2012498/thomash
 *
 * Since the decoded url is always smaller than the encoded one this algo can be used in-place by providing the same string
 * for both src and dst
 *
 * @param[in] `dst` where to put the decoded strin
 * @param[in] `src` the string to decode
 */
void url_decode(StringOwn *dst, const StringRef *src);

/**
 * compress given data to gzip
 * used this (https://github.com/mapbox/gzip-hpp/blob/master/include/gzip/compress.hpp) as a reference
 *
 * @param[in] `data` the string reference pointing to the data to compress
 * @param[in] `output` where to put the result
 */
bool compress_gz(const StringRef *data, StringOwn *output);

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

/**
 * finds needle in haystack for at most count - 1 characters of haystack
 *
 * @param[in] `haystack` the string to search into
 * @param[in] `needle` the string to search for
 * @param[in] `count` the max amount of chars to check for in haystack
 *
 * @return a pointer to the first character of the occurence of needle in haystack, nullptr if not found or if count == 0
 */
const char *strnstr(const char *haystack, const char *needle, const size_t count);

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
 * returns the length of the given string while checking at most `count` characters
 *
 * @param[in] `str` the string to determine the length of. Escluding the null terminator
 * @param[in] `count` the maximum amount of characters to check for str
 *
 * @return the calculated length of str or `count` of not found
 */
size_t strnlen(const char *str, const size_t count);

/**
 * rework the stringRef to remove unwanted whitespaces in front or at the back of the content
 *
 * @param[in] `strRef` the stringRef to trim
 *
 * @return the trimmed stringRef
 */
StringRef trim(StringRef *strRef);

/**
 * check if every character in the stringref is a space
 * a space if any of these -> space, horizontal tab, and whitespaces (\n \r \v \f)
 *
 * @param[in] `strRef` the stringref to check
 *
 * @return true if the string is empty
 */
bool is_empty(const StringRef *strRef);

/**
 * given a sringref mallocs a copy of the string and returns it
 *
 * @param[in] `str` the string to copy
 *
 * @return the mallocated string
 */
char *copy_StringOwn(const StringOwn *str);

/**
 * given a string and its size mallocs a copy of the string and returns it
 *
 * @param[in] `str` the string to copy
 * @param[in] `size` the size of the string
 *
 * @return the mallocated string
 */
char *copy_StringRef(const StringRef *str);

/**
 * Return the string representation of the given number
 *
 * @param[in] `number` the numbero to converto to string
 *
 * @return a heap allocated string that represents the given number
 */
StringOwn num_to_string(size_t number);

StringRef get_reason_phrase(unsigned short statusCode);
