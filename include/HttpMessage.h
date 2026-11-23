#pragma once
#include "MiniMap_HTTPResponseOption_StringOwn.h"
#include "MiniMap_StringRef_StringRef.h"
#include "constants.h"
#include "errcode.h"

#include <stdint.h>

/*
 * I use two representation of an http message for 2 reasons,
 * 1. the inbound include the entire raw message allocated, message used by every stringRef in the struct,
 * the outbound message does not contain the raw message, the parameters (form or query), the requested url and the request method,
 * but makes use of a numeric status code, these differences make the outbound one much smaller
 *
 * 2. the inbound can make use of a lot of const strings, the outbound cannot
 */

typedef struct {
	char                       *raw_message_a;               // the c string containing the entire header, the _a means it's heap allocated. It might contain decoding artefacts since url decoding happends in place
	MiniMap_StringRef_StringRef parameters;                  // contain the data sent in the forms and query parameters
	StringRef                   header_options[RQ_ENUM_LEN]; // an 'hash map' where to store the decoded header options
	StringRef                   url;                         // the resource asked from the client
	StringRef                   body;                        // the content of the message, what the message is about. It might contain decoding artefacts since url decoding happends in place
	HTTPMethod                  method;                      // the appropriate http method, GET, POST, PATCH
	HTTPVersion                 version;                     // the version of the http header (1.0, 1.1, 2.0, ...)
} InboundHttpMessage;

typedef struct {
	MiniMap_HTTPResponseOption_StringOwn header_options; // represent the header as the collection of the single options -> value
	size_t                               header_len;     // how many bytes are there in the header
	StringOwn                            body;           // the content of the message, what the message is about
	uint16_t                             status_code;    // 200, 404, 500, etc etc
	HTTPVersion                          version;        // the version of the http header (1.0, 1.1, 2.0, ...)
} OutboundHttpMessage;

/**
 * \fn ErrCode MessageProcessor(const InboundHttpMessage *in_message, OutboundHttpMessage *out_message)
 * User define function that fill in the desired data into the `out_message` and sents the appropriate headers
 *
 * The user should set the headers, the body content and the status code to complete the request. Regardless, a no-op would result in a valid response with status code 204
 * THE HEADERS SHOULD NOT BE ADDED DIRECTLY TO THE UNDERLYING STRUCTURE BUT THE APPROPRIATE METHOD `add_header_option` SHOULD BE USED
 *
 * \param[in] in_message the message received from the client and routed to this `MessageProcessor`.
 *   It is always properly constructed and __should not__ be destroyed.
 * \param[out] out_message the message to send to the client.
 *   It is always properly constructed and __should not__ be destroyed.
 *   It has the following defaults: Status code: 204, Http Version: HTTPT/1.1, Content-length: 0, Server: <Current server version>, Date: <approximate date before the call to this MessageProcessor>
 *
 * \return an error code describing if and which error occurred
 *  - Any ErrCode other than `NO_OP` or `NO_ERROR` will cause the server to ingore the content of the `out_message` and respond with a <500 Internal Server Error> to the client
 */
typedef ErrCode (*MessageProcessor)(const InboundHttpMessage *in_message, OutboundHttpMessage *out_message);

typedef ErrCode (*OptionsProcessor)(StringRef *a, StringRef *b, InboundHttpMessage *ctx);

/**
 * Reads the given HTTP message `str` ands construct `mex` with the data found
 *
 * \param[in] str the null terminated string containing the entire message,
 *     the inbound mmessage will take ownership of this and free it in `destroy_InboundHttpMessage`.
 * \param[out] mex the InboundHttpMessage to construct
 *
 * \return an error code describing if and which error occurred
 *
 * - `PARSING_HEADER__MALFORMED_REQUEST`: The request is missing the `\r\n\r\n` separator.
 *     The InboundHttpMessage structure was partly constructed and __should not__ be used, but it __should__ be destroyed
 * - `FAILED_ALLOCATION`: the internal parameter map could not be allocated.
 *     The InboundHttpMessage structure was partly constructed and __should not__ be used, but it __should__ be destroyed
 * - `PARSING_HEADER__MALFORMED_REQUEST_LINE`: the request line does not follow the standard.
 *     Nothing was parsed nor stored.
 *     The InboundHttpMessage structure was partly constructed and __should not__ be used, but it __should__ be destroyed
 * - `FAILED_ALLOCATION`:
 *     The query parameters are partly parsed, but the url and the body are still not validated.
 *     The InboundHttpMessage structure was partly constructed and __should not__ be used, but it __should__ be destroyed
 * - `PARSING_HEADER__MALFORMED_PERCENT_ENCODING`: there were wrong percent encodings in the header options and we refuse to continue.
 *     The query parameters are partly parsed, but the url and the body are still not validated.
 *     The InboundHttpMessage structure was partly constructed and __should not__ be used, but it __should__ be destroyed
 * - `PARSING_HEADER__INVALID_URL`: the url is either blank or does not follow the specifications.
 *     All the query parameters have been parsed, but the url and body are still not validated.
 *     The InboundHttpMessage structure was partly constructed and __should not__ be used, but it __should__ be destroyed
 * - `FAILED_ALLOCATION`: could not store the parameter in the internal structure.
 *     Body parsing halted midway, the values stored are correct but some options might be missing.
 *     The InboundHttpMessage structure was partly constructed and __should not__ be used, but it __should__ be destroyed
 * - `PARSING_HEADER__MALFORMED_PERCENT_ENCODING`: invaild percent encoding found.
 *     Body parsing halted midway, the values stored are correct but some options might be missing.
 *     The InboundHttpMessage structure was partly constructed and __should not__ be used, but it __should__ be destroyed
 */
ERRFUNC create_InboundHttpMessage(char *str, InboundHttpMessage *mex);

/**
 * Frees all the associated resources and zeroes all the fields
 *
 * Destroying a null `InbundHttpMessage` is a NO-OP
 *
 * @param[in] mex the InboundHttpMessage to destroy, this will also free the string passed to `create_InboundHttpMessage`
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC destroy_InboundHttpMessage(InboundHttpMessage *mex);

/**
 * Sets up the internal structure of an empty `OutboundHttpMessage` to initialize
 *
 * \param[in] mex the OutboundHttpMessage to create.
 *
 * @return an error code describing if and which error occurred
 *
 * - `FAILED_ALLOCATION`: could not allocate the internal header option map
 *     The OutboundHttpMessage structure was partly constructed and __should not__ be used
 */
ERRFUNC create_OutboundHttpMessage(OutboundHttpMessage *mex);

/**
 * Frees all the associated resources and zeroes all field.
 *
 * Destroying a null `OutbundHttpMessage` is a NO-OP
 *
 * @param[in] mex the OutboundHttpMessage to destroy
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC destroy_OutboundHttpMessage(OutboundHttpMessage *mex);

/**
 * Deconstruct the raw header in a map with key (option) -> value
 * and fetch some Metadata such as verb, version, url, a parameters
 *
 * @param[in] raw_header the StringRef containing the header as a char* string
 * @param[out] msg the httpMessage where to store the information extracted
 *
 * @return an error code describing if and which error occurred
 *
 * - `PARSING_HEADER__MALFORMED_REQUEST_LINE`: the request line does not follow the standard.
 *     Nothing was parsed nor stored
 * - `FAILED_ALLOCATION`:
 *     The query parameters are partly parsed, but the url and the body are still not validated.
 * - `PARSING_HEADER__MALFORMED_PERCENT_ENCODING`: the were wrong percent encodings in the header options and we refuse to continue.
 *     The query parameters are partly parsed, but the url and the body are still not validated.
 * - `PARSING_HEADER__INVALID_URL`: the url is either blank or does not follow the specifications.
 *     All the query parameters have been parsed, but the url and body are still not validated.
 */
ERRFUNC decompose_header(const StringRef *raw_header, InboundHttpMessage *msg);

/**
 * Analyzes the incoming request for form data / other parameters and puts the result in the given message
 *
 * @param[in] msg the message to analyze
 *
 * @return an error code describing if and which error occurred
 *
 * - `FAILED_ALLOCATION`: could not store the parameter in the internal structure.
 *     Parsing halted midway, the values stored are correct but some options might be missing
 * - `PARSING_HEADER__MALFORMED_PERCENT_ENCODING`: invaild percent encoding found.
 *     Parsing halted midway, the values stored are correct but some options might be missing
 */
ERRFUNC decompose_message(InboundHttpMessage *msg);

/**
 * Unite the header and the body in a single message and returns it
 * the compiled message does not have a null terminator and must be freed
 *
 * @param[in] msg the message containing the header options and, eventually, the body
 * @param[out] res where to place the composed message. If an error occurred this value should be discard and there is no need to free it
 *
 * @return an error code describing if and which error occurred
 *
 * - `FAILED_ALLOCATION`: could not allocate the buffer for the entire response. No data was written
 */
ERRFUNC compose_message(const OutboundHttpMessage *msg, StringOwn *res);

/**
 * Given a String representing the request method (GET, POST, PATCH, ...) returns the relative code
 *
 * @param[in] request_method the StringView of the method requested
 *
 * @return the code representing the relative method, HTTP_INVALID_METHOD if the given StringView represents an unknown http method
 */
HTTPMethod get_method_code(const StringView *request_method);

/**
 * Given a string representing the request http version (0., 1.0, 1.1) returns the relative code
 *
 * @param[in] http_version the StringView of the http version
 *
 * @return the code representing the given version, HTTP_VER_UNKN if the given StringView represents an unknown http version
 */
HTTPVersion get_version_code(const StringView *http_version);

/**
 * Given a string representing an header options (Content-type, Content-length, ...)
 *
 * @param[in] option the StringView of the option
 *
 * @return the code representing the given option, RQ_ENUM_LEN if the given StringView represent an unknown request header
 */
HTTPRequestOption get_header_options_code(const StringView *option);

/**
 * Given an a string in the shape ...is...CS...is...CS...is...
 * Where `is' is `item_separator` and `CS` is `chunk_seperator`
 *
 * The reason this function makes use of a function pointer + 2 separators is because the code needed to parse header options and some
 * form parameters is the same except these parameters
 *
 * @param[in] str a StringRef of the string to analyze
 * @param[in] fun a function that given key, value and context process them correctly, the given value might be `nullptr` if there was no `item_separator` in the chunk
 * @param[in] chunk_sep a string that delimits the separation of chunks (e.g. a single header option, a form value)
 * @param[in] item_sep a char to separate the chunk in key and value (e.g. ':', '=')
 * @param[in] ctx the context to pass to the `OptionsProcessor`
 *
 * @return an error code describing if and which error occurred
 *
 * - `PARSING__NULL_CHUNK_SEPARATOR`: the `chunk_sep` passed is either nullptr or "". No parsing done
 * - The error code received from the `fun` OptionsProcessor if different than `NO_OP` and `NO_ERROR`: Parsing is underway.
 *     Stored values are correct but some options might be missing.
 */
ERRFUNC parse_options(const StringRef *segment, const OptionsProcessor fun, const char *chunk_seperator, const char item_separator, InboundHttpMessage *ctx);

// void      parseFormData(const std::string &params, std::string &divisor, std::unordered_map<std::string, std::string> &parameters);

/**
 * Given the header option code and the relative value adds them to the OutboundHttpMessage header options and calculates how much space they take for later
 * ONE SHOULD NOT ADD OPTIONS DIRECTLY TO THE UNDERLYING MAP AND AVOID THIS FUNCTION, AS IT SETS UP DATA NECESSARY FOR A CORRECT MESSAGE COMPOSITION LATER
 *
 * @param[in] option the header option code
 * @param[in] value the string value to assign to the option, this structure will not be stored, a copy will e created and that will be used
 * @param[in] msg the OutboundHttpMessage where to add the header option
 *
 * @return an error code describing if and which error occurred
 *
 * - `FAILED_ALLOCATION`: could not copy the given value to a new buffer.
 *     No data was added
 * - `INVALID_ARGUMENT`: If `option` is not an enum value
 *     No data was added
 */
ERRFUNC add_header_option(const HTTPResponseOption option, const StringView value, OutboundHttpMessage *msg);
