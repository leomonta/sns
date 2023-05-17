#pragma once

#define LOG_ALL     0
#define LOG_DEBUG   1
#define LOG_INFO    2
#define LOG_WARNING 3
#define LOG_ERROR   4
#define LOG_FATAL   5

void log(const char logLevel, const char *mex, ...);

void setLogLevel(const char minLogLevel);