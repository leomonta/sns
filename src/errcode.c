#include "errcode.h"

const char *get_pretty_error(ErrCode code) {

	switch (code) {

	case NO_ERROR:
		return "No error";
	case FAILED_ALLOCATION:
		return "Could not allocated more memory";
	case PARSING__NULL_CHUNK_SEPARATOR:
		return "Refused to parse without a chunk separator";
	case PARSING__MALFORMED_MULTIPART_FORMDATA:
		return "x-www-form-urlencoded data does not follow the correct format";
	case PARSING_HEADER__MALFORMED_URL_ENCODING:
		return "Unknown percentage encoding fragment";
	case PARSING_HEADER__MALFORMED_REQUEST:
		return "Request does not respect the standard";
	case PARSING_HEADER__MALFORMED_REQUEST_LINE:
		return "Unknown values in the request line";
	}
}
