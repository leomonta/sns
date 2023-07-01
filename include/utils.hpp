#pragma once

#include <string>
#include <vector>

#include "stringRef.hpp"

std::vector<std::string> split(const std::string &source, const std::string &find);
std::string              getUTC();
void                     urlDecode(char *dst, const char *src);
void                     compressGz(std::string &output, const char *data, size_t size);
void                     trimwhitespace(char *str);
const char              *strnstr(const char *haystack, const char *needle, const size_t count);
const char              *strnchr(const char *str, int chr, const size_t count);
void                     trim(stringRef &strRef);