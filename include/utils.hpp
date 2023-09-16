#pragma once

#include "stringRef.hpp"

#include <string>
#include <vector>

/**
 * split the given string with a single token, and return the vector of the splitted strings
 *
 * @param source the string to split
 * @param sep the separator to split the string
 *
 * @return a vector of the splitted strings
 */
std::vector<std::string> split(const std::string &source, const std::string &sep);

/**
 * simply get the time formetted following RFC822 regulation on GMT time
 *
 * @return a strig containing the date
 */
std::string getUTC();

/**
 * Decode url character (e.g. %20 => " ") to ascii character, shamelessly copied from stackoverflow
 * copied from https://stackoverflow.com/questions/2673207/c-c-url-decode-library/2766963,
 *
 * @author Thank you ThomasH, https://stackoverflow.com/users/2012498/thomash
 *
 * Since the decoded url is always smaller than the encoded one this algo can be used in-place by providing the same string
 * for both src and dst
 *
 * @param dst where to put the decoded strin
 * @param src the string to decode
 */
void urlDecode(char *dst, const char *src);

/**
 * compress given data to gzip
 * used this (https://github.com/mapbox/gzip-hpp/blob/master/include/gzip/compress.hpp) as a reference
 *
 * @param data the string reference pointing to the data to compress
 * @param output where to put the result
 */
void compressGz(const stringRef data, std::string &output);

/**
 * Thaks to https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
 * @author https://stackoverflow.com/users/9530/adam-rosenfield
 * plus some modification
 *
 * rewrite the string to be without spaces at the start and end
 * it places another null terminator if necessary
 *
 * @param str the string to trim
 */
void trimwhitespace(char *str);

/**
 * finds needle in haystack for at most count - 1 characters of haystack
 *
 * @param haystack the string to search into
 * @param needle the string to search for
 * @param count the max amount of chars to check for in haystack
 *
 * @return a pointer to the first character of the occurence of needl in haystack, nullptr if not found or if count == 0
 */
const char *strnstr(const char *haystack, const char *needle, const size_t count);

/**
 * finds the first occurrence of chr in the byte string pointed to by str to a max of count - 1
 *
 * @param str the string to search into
 * @param chr the char to search for
 * @param count the max amount of chars to check for in str
 *
 * @return pointer to the character found or nullptr if no such character is found
 */
const char *strnchr(const char *str, int chr, const size_t count);

/**
 * rework the stringRef to remove unwanted whitespaces in front or at the back of the content
 *
 * @param strRef the stringRef to modify
 */
void trim(stringRef &strRef);

/**
 * check if every character in the stringref is a space
 * a space if any of these -> space, horizontal tab, and whitespaces (\n \r \v \f)
 *
 * @param strRef the stringref to check
 */
bool isEmpty(const stringRef &strRef);

/**
 * print a string ref to stdout
 *
 * @param strRef the stirngRef to print
 */
void printStringRef(const stringRef &strRef);

/**
 * given a sringref mallocs a copy of the string and returns it
 * 
 * @param str the string to copy
 */
const char *makeCopy(const stringRef &str);