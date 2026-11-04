#pragma once

typedef enum {
	NO_ERROR = 0,
	FAILED_ALLOCATION,
	PARSING__NULL_CHUNK_SEPARATOR,
	PARSING__MALFORMED_REQUEST,
	PARSING__MALFORMED_MULTIPART_FORMDATA,
	PARSING_HEADER__MALFORMED_REQUEST_LINE,
} errcode;

#define ERRFUNC [[nodiscard]] errcode

#define RET_IF_ERR(code)    \
	if (code != NO_ERROR) { \
		return code;        \
	}
