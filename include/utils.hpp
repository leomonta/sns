#include <string>
#include <vector>
#include <time.h>
#include <stdexcept>

#include "zlib.h"

std::vector<std::string> split(const std::string& source, const std::string& find);
std::string getUTC();
void urlDecode(char* dst, const char* src);
void compressGz(std::string& output, const char* data, std::size_t size);
