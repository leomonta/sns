#pragma once

typedef enum {
	NO_ERROR = 0,
	FAILED_ALLOCATION,
	PARSING__NULL_CHUNK_SEPARATOR,
	PARSING__MALFORMED_MULTIPART_FORMDATA,
	PARSING_HEADER__MALFORMED_URL_ENCODING,
	PARSING_HEADER__MALFORMED_REQUEST,
	PARSING_HEADER__MALFORMED_REQUEST_LINE,
} ErrCode;

#define ERRFUNC [[nodiscard]] ErrCode

#define RET_IF_ERR(code)    \
	if (code != NO_ERROR) { \
		llog(LOG_ERROR, "Error: %s\n", get_pretty_error(code)); \
		return code;        \
	}


/**
 * Gives a human readable version of the error code
 *
 * @param `code` the param for which to given the message
 *
 * @return a string briefly explaining the error
 */
const char* get_pretty_error(ErrCode code);
