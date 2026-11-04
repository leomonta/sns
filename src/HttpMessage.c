#include "HttpMessage.h"

#include "MiniMap_StringRef_StringRef.h"
#include "StringRef.h"
#include "constants.h"
#include "utils.h"

#include <assert.h>
#include <errno.h>
#include <logger.h>
#include <stdio.h>

void log_malformed_oprion(const StringRef *str_ref) {
	llog(LOG_WARNING, "Malformed option -> '%.*s'\n", CLAMP_TO_POSITIVE_INT(str_ref->len), str_ref->str);
}

ERRFUNC parse_InboundMessage(const char *str, InboundHttpMessage *mex) {

	memset(mex, '0', sizeof(InboundHttpMessage));

	// use strlen so i'm sure it's not counting the null terminator
	auto msg_len = strlen(str);

	// save the message in a local pointer so i don't rely on std::string allocation plus a null terminator
	char *temp = malloc(msg_len + 1);
	TEST_ALLOC(temp)
	memcpy(temp, str, msg_len);
	temp[msg_len] = 0;

	mex->raw_message_a = temp;
	mex->parameters    = MiniMap_StringRef_StringRef_make(10, equal_StringRef);

	// body and header are divided by two newlines
	auto msg_separator = strstr(mex->raw_message_a, "\r\n\r\n");

	if (msg_separator == nullptr) {
		return PARSING__MALFORMED_REQUEST;
	}

	unsigned long header_len = (unsigned long)(msg_separator - mex->raw_message_a);

	StringRef header;

	// strstr return 0 if nothing is found or the given pointer if it is an empty string
	if (msg_separator == 0 || msg_separator == mex->raw_message_a) {
		header    = (StringRef){nullptr, 0};
		mex->body = (StringRef){nullptr, 0};
	} else {
		header    = (StringRef){mex->raw_message_a, header_len};
		mex->body = (StringRef){mex->raw_message_a + header_len + 4, msg_len - header_len - 4};
	}

	// now i have the two stringRefs to the body and the header

	// extract metadata parameters and other stuff
	const auto res = decompose_header(&header, mex);
	RET_IF_ERR(res)

	decompose_message(mex);

	return NO_ERROR;
}

void destroy_InboundHttpMessage(InboundHttpMessage *mex) {

	if (mex->raw_message_a != nullptr) {
		free(mex->raw_message_a);
	}

	MiniMap_StringRef_StringRef_destroy(&mex->parameters);
}

void destroy_OutboundHttpMessage(OutboundHttpMessage *mex) {

	// for (const auto &[key, val] : mex->m_headerOptions) {
	for (size_t i = 0; i < mex->header_options.values.count; ++i) {
		free(mex->header_options.values.data[i].str);
	}

	MiniMap_u_char_StringOwn_destroy(&mex->header_options);
	free(mex->body.str);
}

void add_to_options(StringRef key, StringRef val, InboundHttpMessage *ctx) {
	const auto index = get_parameter_code(&key);
	if (index < 0 || index > RQ_ENUM_LEN) {
		llog(LOG_WARNING, "[HEADER] Could not recognize parameter '%.*s = %.*s'\n", CLAMP_TO_POSITIVE_INT(key.len), key.str, CLAMP_TO_POSITIVE_INT(val.len), val.str);
	}
	ctx->header_options[index] = val;
}

void add_to_params(StringRef key, StringRef val, InboundHttpMessage *ctx) {
	MiniMap_StringRef_StringRef_set(&ctx->parameters, &key, &val);
}

ERRFUNC decompose_header(const StringRef *raw_header, InboundHttpMessage *msg) {

	// the first line should be "METHOD URL HTTP/Version"

	//  0 |1|    2
	// GET / HTTP/1.1\r\n
	ptrdiff_t first_space_len = strnchr(raw_header->str, ' ', raw_header->len) - raw_header->str; // gets me until the space after the method verb
	if (first_space_len < 0) {
		return PARSING_HEADER__MALFORMED_REQUEST_LINE;
	}

	const char *after_first_space = raw_header->str + first_space_len + 1;
	ptrdiff_t   second_space_len  = strnchr(after_first_space, ' ', raw_header->len) - after_first_space; // until the space after the URL
	if (second_space_len < 0) {
		return PARSING_HEADER__MALFORMED_REQUEST_LINE;
	}

	const char *after_second_space = raw_header->str + first_space_len + second_space_len + 2;
	ptrdiff_t   endline_len        = strnstr(raw_header->str, "\r\n", raw_header->len) - after_second_space; // until the first newline
	if (endline_len < 0) {
		return PARSING_HEADER__MALFORMED_REQUEST_LINE;
	}

	// temporary stringRefs
	StringRef method_ref  = {raw_header->str, (size_t)first_space_len};
	StringRef version_ref = {after_second_space, (size_t)endline_len};

	// actually storing the values in the object
	msg->method  = get_method_code(&method_ref);
	msg->url     = (StringRef){after_first_space, (size_t)second_space_len};
	msg->version = get_version_code(&version_ref);

	// Now checks if there are query parameters

	// the position of the query parameter marker (if present)
	auto qmark_index = strnchr(msg->url.str, '?', msg->url.len) - msg->url.str;

	// if strnchr returns nullptr the result is negative, else is positive
	if (qmark_index > 0) {
		// confine the parameters in a single stringREf excluding the '?'
		StringRef  query_parameters = {msg->url.str + qmark_index + 1, msg->url.len - (size_t)(qmark_index)-1};
		const auto err              = parse_options(&query_parameters, add_to_params, "&", '=', msg);

		RET_IF_ERR(err)

		// limit thw url to before the '?'
		msg->url.len = (size_t)(qmark_index);
	}

	return parse_options(raw_header, add_to_options, "\r\n", ':', msg);
}

ERRFUNC decompose_message(InboundHttpMessage *msg) {
	// if the client is sending some form data or other type od data we need to parse that

	auto content_type = msg->header_options[RQ_CONTENT_TYPE];

	// the content type indicates how parameters are showed
	if (strrefblank(&content_type)) {
		return NO_ERROR;
	}

	// type one, url encoded
	if (strncmp("application/x-www-form-urlencoded", content_type.str, content_type.len) == 0) {
		// the fields are separated from each others with a "&" and key -> value are separated with "="
		// plus they are encoded as a URL
		return parse_options(&msg->body, add_to_params, "&", '=', msg);

		// type two, plain text
	} else if (strncmp("text/plain", content_type.str, content_type.len) == 0) {
		return parse_options(&msg->body, add_to_params, "\r\n", '=', msg);

		// type three, multipart form-data
	} else if (strnstr(content_type.str, "multipart/form-data;", content_type.len) != nullptr) {
		// find the divisor
		//               0              |      1
		// multipart/form-data; boundary=----asdwadawd
		// auto divisor = split(cType, "=")[1];
		auto eq = strnchr(content_type.str, '=', content_type.len);
		// sometimes the boundary is encolsed in quotes, take care of this case
		if (eq == nullptr) {
			llog(LOG_WARNING, "Malformed multipart form data, no '='\n");
			return PARSING__MALFORMED_MULTIPART_FORMDATA;
		}

		StringRef divisor = {eq, (size_t)(eq - content_type.str)};

		if (*eq == '"') {
			++divisor.str;
			--divisor.len;
		}

		llog(LOG_ERROR, "[PARSING] Multipart formdata not implemented\n");
		// http::parseFormData(body, divisor, parameters);
	}

	return NO_ERROR;
}

StringOwn compose_message(const OutboundHttpMessage *msg) {

	// I preconstruct the status line so i don't have to do multiple allocations and string concatenations
	// The value to modify are at
	//                             11
	//                   012345678901
	char status_line[] = "HTTP/1.0 XXX ";
	auto phrase        = get_reason_phrase(msg->status_code);

	// A symbolic name for how long the status line is
	const size_t STATUS_LINE_LEN = sizeof(status_line) - 1;

	// the 2 is for the \r\n separator of the body
	// the other +2 if for the status line \r\n
	auto msg_len = STATUS_LINE_LEN + phrase.len + 2 + msg->header_len + 2 + msg->body.len;

	// the entire message length
	char *res = malloc(msg_len);
	TEST_ALLOC(res)
	memset(res, '\0', msg_len);

	memcpy(res, status_line, STATUS_LINE_LEN);
	memcpy(res + STATUS_LINE_LEN, phrase.str, phrase.len);
	memcpy(res + STATUS_LINE_LEN + phrase.len, "\r\n", 2);

	// set the status code digit by digit
	res[11] = '0' + (char)((msg->status_code) % 10);
	res[10] = '0' + (char)((msg->status_code / 10) % 10);
	res[9]  = '0' + (char)((msg->status_code / 100) % 10);

	char *writer = res + STATUS_LINE_LEN + phrase.len + 2;

	// add all headers
	for (size_t i = 0; i < msg->header_options.values.count; ++i) {

		auto key = msg->header_options.keys.data[i];
		auto val = msg->header_options.values.data[i];

		// the format is
		// name: value\r\n
		memcpy(writer, header_response_options_str[key].str, header_response_options_str[key].len);
		writer += header_response_options_str[key].len;

		memcpy(writer, ": ", 2);
		writer += 2;

		memcpy(writer, val.str, val.len);
		writer += val.len;

		memcpy(writer, "\r\n", 2);
		writer += 2;
	}

	// headr body separator
	memcpy(writer, "\r\n", 2);
	writer += 2;

	memcpy(writer, msg->body.str, msg->body.len);
	writer += msg->body.len;

	StringOwn r = {
	    .str = res,
	    .len = msg_len,
	};
	return r;
}

HTTP_Method get_method_code(const StringRef *request_method) {

	if (request_method->len == 0) {
		return HTTP_INVALID_METHOD;
	}

	// since the codes are sequential starting from 1
	// the 0 is for an invalid http method / verb
	for (u_char i = 1; i < HTTP_ENUM_LEN; ++i) {
		if (equal_StringRef(request_method, &methods_str[i])) {
			return i;
		}
	}

	return HTTP_INVALID_METHOD;
}

HTTP_Version get_version_code(const StringRef *http_version) {

	if (http_version->len == 0) {
		return HTTP_VER_UNKN;
	}

	for (u_char i = 1; i < HTTP_VER_ENUM_LEN; ++i) {
		if (equal_StringRef(http_version, &versions_str[i])) {
			return i;
		}
	}

	return HTTP_VER_UNKN;
}

u_char get_parameter_code(const StringRef *parameter) {

	for (u_char i = 0; i < RQ_ENUM_LEN; ++i) {
		if (equal_StringRef(parameter, &header_request_options_str[i])) {
			return i;
		}
	}

	return (u_char)(-1);
}

errcode parse_options(const StringRef *segment, const OptionsProcessor fun, const char *chunk_sep, const char item_sep, InboundHttpMessage *ctx) {

	StringRef   window        = {segment->str, segment->len};
	const char *segment_end   = segment->str + segment->len;
	const auto  chunk_sep_len = strlen(chunk_sep);

	assert(chunk_sep_len > 0); // for when the string is a compile time constant
	if (chunk_sep_len <= 0) {
		return PARSING__NULL_CHUNK_SEPARATOR;
	}

	// ok as long as
	//  - the window start is inside the segment
	//  - the window end is inside the segment
	while (window.str < segment_end && (window.str + window.len <= segment_end)) {

		auto separator = strnstr(window.str, chunk_sep, window.len);

		// the chunk start is correct either way
		StringRef chunk = {window.str, window.len};

		// if the chunk separator was found that will dictate the length, else the chunk will be rest of the window (i.e. the rest of the data)
		if (separator != nullptr) {
			chunk.len = (size_t)(separator + chunk_sep_len - window.str);
		}

		auto sep = strnchr(chunk.str, item_sep, chunk.len);
		if (sep == nullptr) {
			log_malformed_oprion(&chunk);
		} else {

			// everything is fine here
			size_t    pos = (size_t)(sep - chunk.str);      // get the pointer difference from the start of the chunk and the position of the 'item_sep'
			StringRef key = {chunk.str, pos};               // from the start of the chunk to before the 'item_sep'
			StringRef val = {sep + 1, chunk.len - pos - 1}; // from after the 'item_sep' to the end of the chunk

			key = trim(&key);
			val = trim(&val);

			// check if we actually have a key, the value can be empty
			if (!strrefblank(&key)) {
				fun(key, val, ctx);
			}
		}

		// start again at the end of the chunk
		window.str = separator + chunk_sep_len; // this + `chunk_sep_len` skips the chunk so the next search does not find it again,
		                                        // and also advances sligthly the window, pushing it out of segment and triggering the exit condition when there is no more data to parse
		window.len -= chunk.len;
	}

	return NO_ERROR;
}

/**
 * Parse the data divided by special divisor in form data and then insert them in the parameters map in the httpMessage
 *
 * @param params the parameters encodedn in form data
 * @param divisor the unique divisor string used in the data
 * @param parameters the map where to put the parameters found
 *
void http::parseFormData(const std::string &params, std::string &divisor, std::unordered_map<std::string, std::string> &parameters) {

    PROFILE_FUNCTION();

    std::vector<std::string> temp;

    auto datas = split(params, divisor);

    // the last place always contains "--", no real data in here
    datas.pop_back();
    datas.erase(datas.begin());

    for (size_t i = 0; i < datas.size(); i++) {
        // 0 |||||                    1                        ||||||2|||||  3   | 4  |
        //   \r\n|Content-Disposition: form-data; name="field1"|\r\n| \r\n|value1|\r\n|
        //                            0                                      1
        std::vector<std::string> option_value = split(datas[i], "\r\n");
        option_value.pop_back();                      // pos 4
        option_value.erase(option_value.begin() + 2); // pos 2
        option_value.erase(option_value.begin());     // pos 0

        //                0              ||      1      ||         2
        // Content-Disposition: form-data; name="field1"; filename="example.txt"
        std::vector<std::string> options = split(option_value[0], "; ");

        // jump directly to "name="
        std::string name = options[1].substr(5);
        // remove heading and trailing double quotes "
        name.pop_back();
        name.erase(name.begin());

        parameters[name] = option_value[1];
    }
}
*/

void add_header_option(const HTTPHeaderResponseOption option, const StringRef *value, OutboundHttpMessage *msg) {

	auto opt = header_response_options_str[option];

	size_t len      = value->len;
	size_t calc_len = strnlen(value->str, value->len);
	if (calc_len < len) {
		len = calc_len;
	}

	StringRef t = {value->str, len};

	StringOwn cpy = {
	    copy_StringRef(&t),
	    len,
	};

	// if something is already present at the requested position
	StringOwn old = {};
	auto      res = MiniMap_u_char_StringOwn_get(&msg->header_options, &option, &old);

	// this spilt if is here because there might be an insertion of a stringref with nullptr but size > 0

	// something was present, need to unreserve the space it reserved
	if (res) {
		msg->header_len -= (opt.len + 2 + old.len + 2);

		// free it if it was something
		if (old.str != nullptr) {
			// free it
			free(old.str);
		}
	}
	MiniMap_u_char_StringOwn_set(&msg->header_options, &option, &cpy);

	// account the bytes that will be added later
	//                 name     ': ' value '\r\n'
	msg->header_len += (opt.len + 2 + cpy.len + 2);
}
