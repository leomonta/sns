#include "HttpMessage.h"

#include "MiniMap_StringRef_StringRef.h"
#include "StringRef.h"
#include "constants.h"
#include "utils.h"

#include <errno.h>
#include <logger.h>
#include <stdio.h>

void log_malformed_parameter(const StringRef *str_ref) {
	llog(LOG_WARNING, "Malformed parameter -> '%*s' \n", (int)str_ref->len, str_ref->str);
}

InboundHttpMessage parse_InboundMessage(const char *str) {

	InboundHttpMessage res = {};

	// use strlen so i'm sure it's not counting the null terminator
	auto msg_len = strlen(str);

	// save the message in a local pointer so i don't rely on std::string allocation plus a null terminator
	char *temp = malloc(msg_len + 1);
	TEST_ALLOC(temp)
	memcpy(temp, str, msg_len);
	temp[msg_len] = 0;

	res.raw_message_a = temp;
	res.parameters    = MiniMap_StringRef_StringRef_make(10);

	// body and header are divided by two newlines
	auto msg_separator = strstr(res.raw_message_a, "\r\n\r\n");

	unsigned long header_len = (unsigned long)(msg_separator - res.raw_message_a);

	StringRef header;

	// strstr return 0 if nothing is found or the given pointer if it is an empty string
	if (msg_separator == 0 || msg_separator == res.raw_message_a) {
		header   = (StringRef){nullptr, 0};
		res.body = (StringRef){nullptr, 0};
	} else {
		header   = (StringRef){res.raw_message_a, header_len};
		res.body = (StringRef){res.raw_message_a + header_len, msg_len - header_len};
	}

	// now i have the two stringRefs to the body and the header

	// extract metadata parameters and other stuff
	decompose_header(&header, &res);

	decompose_message(&res);

	return res;
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
	free(mex->resource_name.str);
}

void add_to_options(StringRef key, StringRef val, InboundHttpMessage *ctx) {
	ctx->header_options[get_parameter_code(&key)] = val;
}

void add_to_params(StringRef key, StringRef val, InboundHttpMessage *ctx) {
	MiniMap_StringRef_StringRef_set(&ctx->parameters, &key, &val, equal_StringRef);
}

void decompose_header(const StringRef *raw_header, InboundHttpMessage *msg) {

	// the first line should be "METHOD URL HTTP/Version"

	//  0 |1|    2
	// GET / HTTP/1.1\r\n
	ptrdiff_t first_space_len  = strnstr(raw_header->str, " ", raw_header->len) - raw_header->str;                                               // gets me until the space after the method verb
	ptrdiff_t second_space_len = strnstr(raw_header->str + first_space_len + 1, " ", raw_header->len) - (raw_header->str + first_space_len + 1); // until the space after the URL
	ptrdiff_t endline_len      = strnstr(raw_header->str, "\r\n", raw_header->len) - (raw_header->str + first_space_len + second_space_len + 2); // until the first newline

	// temporary stringRefs
	StringRef method_ref  = {raw_header->str, (size_t)first_space_len};
	StringRef version_ref = {raw_header->str + second_space_len + first_space_len + 2, (size_t)endline_len};

	// actually storing the values in the object
	msg->method  = get_method_code(&method_ref);
	msg->url     = (StringRef){raw_header->str + first_space_len + 1, (size_t)second_space_len}; // line[1];
	msg->version = get_version_code(&version_ref);

	// Now checks if there are query parameters

	// the position of the query parameter marker (if present)
	auto qmark_index = strnchr(msg->url.str, '?', msg->url.len) - msg->url.str;

	// if strnchr returns nullptr the result is negative, else is positive
	if (qmark_index > 0) {
		// confine the parameters in a single stringREf excluding the '?'
		StringRef query_parameters = {msg->url.str + qmark_index + 1, msg->url.len - (size_t)(qmark_index)-1};
		parse_options(&query_parameters, add_to_params, "&", '=', msg);

		// limit thw url to before the '?'
		msg->url.len -= (size_t)(qmark_index);
	}

	parse_options(raw_header, add_to_options, "\r\n", ':', msg);
}

void decompose_message(InboundHttpMessage *msg) {
	// if the client is sending some form data or other type od data we need to parse that

	auto content_type = msg->header_options[RQ_CONTENT_TYPE];

	// the content type indicates how parameters are showed
	if (strrefblnk(&content_type)) {
		return;
	}

	// type one, url encoded
	if (strncmp("application/x-www-form-urlencoded", content_type.str, content_type.len) == 0) {
		// the fields are separated from each others with a "&" and key -> value are separated with "="
		// plus they are encoded as a URL
		parse_options(&msg->body, add_to_params, "&", '=', msg);

		// type two plain text
	} else if (strncmp("text/plain", content_type.str, content_type.len) == 0) {
		parse_options(&msg->body, add_to_params, "\r\n", '=', msg);

		// part three multipart form-data
	} else if (strnstr("multipart/form-data;", content_type.str, content_type.len) != nullptr) {
		// find the divisor
		//               0              |      1
		// multipart/form-data; boundary=----asdwadawd
		// auto divisor = split(cType, "=")[1];
		auto eq = strnchr(content_type.str, '=', content_type.len);
		// sometimes the boundary is encolsed in quotes, take care of this case
		if (eq == nullptr) {
			llog(LOG_WARNING, "Malformed multipart form data, no '='\n");
		}

		size_t    temp    = (size_t)(content_type.str + content_type.len - eq);
		StringRef divisor = {eq, temp};

		if (*eq == '"') {
			++divisor.str;
			--divisor.len;
		}

		// http::parseFormData(body, divisor, parameters);
	}
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

u_char get_method_code(const StringRef *request_method) {

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

u_char get_version_code(const StringRef *http_version) {

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
		if (equal_StringRef(parameter, &header_request_options_str[i]) == 0) {
			return i;
		}
	}

	return (u_char)(-1);
}

void parse_options(const StringRef *segment, void (*fun)(StringRef a, StringRef b, InboundHttpMessage *ctx), const char *chunk_sep, const char item_sep, InboundHttpMessage *ctx) {

	const size_t add_len = strlen(chunk_sep); // the lenght to move after the string is found

	auto        start_options        = strnstr(segment->str, chunk_sep, segment->len) + add_len; // the plus 1 is needed to start after the \r\n
	size_t      len_to_start_options = (size_t)(start_options - segment->str);
	StringRef   chunk                = {start_options, len_to_start_options};
	const char *limit                = segment->str + segment->len;
	auto        limit_len            = segment->len - len_to_start_options;

	while (chunk.str < limit) {

		auto separator = strnstr(chunk.str, chunk_sep, limit_len);

		if (separator == nullptr) {
			chunk.len = limit_len;
		} else {
			chunk.len = (size_t)(separator - chunk.str);
		}

		auto sep = strnchr(chunk.str, item_sep, chunk.len);
		if (sep == nullptr) {
			log_malformed_parameter(&chunk);
		} else {

			// everything is fine here
			size_t    pos = (size_t)(sep - chunk.str);      // get the pointer difference from the start of the chunk and the position of the '='
			StringRef key = {chunk.str, pos};               // from the start of the chunk to before the '='
			StringRef val = {sep + 1, chunk.len - pos - 1}; // from after the '=' to the end of the chunk

			key = trim(&key);
			val = trim(&val);

			// check if we actually have a key, the value can be empty
			if (strrefblnk(&key)) {
				// finally put it in the map
				fun(key, val, ctx);
			}
		}

		// at the end of the chunk
		chunk.str = chunk.str + chunk.len + add_len; // move at the end of the string, over the ampersand
		limit_len -= chunk.len + add_len;            // recalculate the limitLen (limitLen = limitLen - len - 1) -> limitLen = limitLen - (len + 1)
		chunk.len = 0;                               // just to be sure
	}
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

bool compare_u_char(const u_char *lhs, const u_char *rhs) {
	return *lhs == *rhs;
}

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
	auto      res = MiniMap_u_char_StringOwn_get(&msg->header_options, &option, compare_u_char, &old);

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
	MiniMap_u_char_StringOwn_set(&msg->header_options, &option, &cpy, compare_u_char);

	// account the bytes that will be added later
	//                 name     ': ' value '\r\n'
	msg->header_len += (opt.len + 2 + cpy.len + 2);
}

void set_filename(const StringRef *val, OutboundHttpMessage *msg) {
	msg->resource_name = (StringOwn){copy_StringRef(val), val->len};
}
