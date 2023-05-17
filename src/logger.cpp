#include "logger.hpp"

#include "profiler.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>

char currLogLevel = LOG_ALL;

void setLogLevel(const char newLogLevel) {
	currLogLevel = newLogLevel;
}

void log(const char logLevel, const char *mex, ...) {

	PROFILE_FUNCTION();

	if (currLogLevel > logLevel) {
		return;
	}

	va_list args;
	va_start(args, mex);

	const char *prefix = "[ UNKWN ]";

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

	auto tid = gettid();

	printf("\r%s [THREAD %6d] ", prefix, tid);
	vprintf(mex, args);
	printf("> ");
	fflush(stdout);

	va_end(args);
}
