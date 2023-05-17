#include "logger.hpp"

#include <stdarg.h>
#include <stdio.h>

char currLogLevel = LOG_ALL;

void setLogLevel(const char newLogLevel) {
	currLogLevel = newLogLevel;
}

void log(const char logLevel, const char *mex, ...) {

	if (currLogLevel > logLevel) {
		return;
	}

	va_list args;
	va_start(args, mex);

	const char *prefix;

	switch (logLevel) {
	case LOG_ALL:
		prefix = "[  ALL  ]";
		break;
	case LOG_DEBUG:
		prefix = "[ DEBUG ]";
		break;
	case LOG_INFO:
		prefix = "[  INFO ]";
		break;
	case LOG_WARNING:
		prefix = "[WARNING]";
		break;
	case LOG_ERROR:
		prefix = "[ ERROR ]";
		break;
	case LOG_FATAL:
		prefix = "[ FATAL ]";
		break;
	}

	printf("%s: ", prefix);
	printf(mex, args);
	printf("\n");

	va_end(args);
}
