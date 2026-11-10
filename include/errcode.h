#pragma once

#include "errno.h"
#include "logger.h"

typedef enum {
	NO_ERROR = 0,                           // No error happened
	NO_OP    = 1,                           // There is no data to give back, whataver the value set in the out parameter should be ignored
	FAILED_ALLOCATION,                      // malloc / realloc failed
	INVALID_ARGUMENT,                       // the argument given breach the contract of the function
	PARSING__NULL_CHUNK_SEPARATOR,          // malformed or unknown separator to decode (e.g. '\r\n' in header options)
	PARSING__MALFORMED_MULTIPART_FORMDATA,  // the body of a mltipart form-data request does not comply with the specs
	PARSING_HEADER__INVALID_URL,            // the url is null or not in form that is expected
	PARSING_HEADER__MALFORMED_URL_ENCODING, // the two characters after the '%' are not hex digits or they don't resolve to a known character
	PARSING_HEADER__MALFORMED_REQUEST,      // the request is missing the '\r\n\r\n' body-head separator
	PARSING_HEADER__MALFORMED_REQUEST_LINE, // Method, url, or version are not present or not a standard value
	PARSING_HEADER__UKNOWN_PARAMETER,       // the client has used a parameter that we don't deal with
	THREADPOOL__FAILED_MUTEX_INIT,          // the mutex could not be initialized
	THREADPOOL__FAILED_SEMAPHORE_INIT,      // the semaphore could not be initialized
	THREADPOOL__FAILED_THREAD_CREATION,     // the threads could threads for the thread pool could not be started
	ZLIB__INIT_FAILED,                      // zlib init failed
} ErrCode;

#define ERRFUNC [[nodiscard]] ErrCode

#define RET_IF_ERR(code) \
	if (code > NO_OP) {  \
		return code;     \
	}

#define TEST_ALLOC(ptr)                                                                             \
	if (ptr == NULL) {                                                                              \
		llog(LOG_FATAL, "[ALLOCATION] Allocation returned NULL: %d, %s\n", errno, strerror(errno)); \
		return FAILED_ALLOCATION;                                                                   \
	}
