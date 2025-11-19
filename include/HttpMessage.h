#pragma once
#include "MiniMap_StringRef_StringRef.h"
#include "MiniMap_u_char_StringOwn.h"

#include <stdint.h>

// http method code
typedef enum : uint8_t {
	HTTP_INVALID_METHOD,
	HTTP_GET,
	HTTP_HEAD,
	HTTP_POST,
	HTTP_PUT,
	HTTP_DELETE,
	HTTP_OPTIONS,
	HTTP_CONNECT,
	HTTP_TRACE,
	HTTP_PATCH,
	HTTP_ENUM_LEN,
} HTTP_Method;

typedef enum : uint8_t {
	HTTP_VER_UNKN,
	HTTP_VER_09,
	HTTP_VER_10,
	HTTP_VER_11,
	HTTP_VER_2,
	HTTP_VER_3,
	HTTP_VER_ENUM_LEN,
} HTTP_Version;

typedef enum : uint8_t {
	// ReQuest options
	RQ_A_IM,
	RQ_ACCEPT,
	RQ_ACCEPT_CHARSET,
	RQ_ACCEPT_DATETIME,
	RQ_ACCEPT_ENCODING,
	RQ_ACCEPT_LANGUAGE,
	RQ_ACCESS_CONTROL_REQUEST_METHOD,
	RQ_ACCESS_CONTROL_REQUEST_HEADERS,
	RQ_AUTHORIZATION,
	RQ_CACHE_CONTROL,
	RQ_CONNECTION,
	RQ_CONTENT_ENCODING,
	RQ_CONTENT_LENGTH,
	RQ_CONTENT_MD5,
	RQ_CONTENT_TYPE,
	RQ_COOKIE,
	RQ_DATE,
	RQ_EXPECT,
	RQ_FORWARDED,
	RQ_FROM,
	RQ_HOST,
	RQ_HTTP2_SETTINGS,
	RQ_IF_MATCH,
	RQ_IF_MODIFIED_SINCE,
	RQ_IF_NONE_MATCH,
	RQ_IF_RANGE,
	RQ_IF_UNMODIFIED_SINCE,
	RQ_MAX_FORWARDS,
	RQ_ORIGIN,
	RQ_PRAGMA,
	RQ_PREFER,
	RQ_PROXY_AUTHORIZATION,
	RQ_RANGE,
	RQ_REFERER,
	RQ_TE,
	RQ_TRAILER,
	RQ_TRANSFER_ENCODING,
	RQ_USER_AGENT,
	RQ_UPGRADE,
	RQ_VIA,
	RQ_WARNING,
	RQ_ENUM_LEN,
} HTTPHeaderRequestOption;

typedef enum : uint8_t {
	// ResPonse Options
	RP_ACCEPT_CH,
	RP_ACCESS_CONTROL_ALLOW_ORIGIN,
	RP_ACCESS_CONTROL_ALLOW_CREDENTIALS,
	RP_ACCESS_CONTROL_EXPOSE_HEADERS,
	RP_ACCESS_CONTROL_MAX_AGE,
	RP_ACCESS_CONTROL_ALLOW_METHODS,
	RP_ACCESS_CONTROL_ALLOW_HEADERS,
	RP_ACCEPT_PATCH,
	RP_ACCEPT_RANGES,
	RP_AGE,
	RP_ALLOW,
	RP_ALT_SVC,
	RP_CACHE_CONTROL,
	RP_CONNECTION,
	RP_CONTENT_DISPOSITION,
	RP_CONTENT_ENCODING,
	RP_CONTENT_LANGUAGE,
	RP_CONTENT_LENGTH,
	RP_CONTENT_LOCATION,
	RP_CONTENT_MD5,
	RP_CONTENT_RANGE,
	RP_CONTENT_TYPE,
	RP_DATE,
	RP_DELTA_BASE,
	RP_ETAG,
	RP_EXPIRES,
	RP_IM,
	RP_LAST_MODIFIED,
	RP_LINK,
	RP_LOCATION,
	RP_P3P,
	RP_PRAGMA,
	RP_PREFERENCE_APPLIED,
	RP_PROXY_AUTHENTICATE,
	RP_PUBLIC_KEY_PINS,
	RP_RETRY_AFTER,
	RP_SERVER,
	RP_SET_COOKIE,
	RP_STRICT_TRANSPORT_SECURITY,
	RP_TRAILER,
	RP_TRANSFER_ENCODING,
	RP_TK,
	RP_UPGRADE,
	RP_VARY,
	RP_VIA,
	RP_WARNING,
	RP_WWW_AUTHENTICATE,
	RP_X_FRAME_OPTIONS,
	RP_ENUM_LEN,
} HTTPHeaderResponseOption;

/*
 * I use two representation of an http message for 2 reasons,
 * 1. the inbound include the entire raw message allocated, message used by every stringRef in the struct,
 * the outbound message does not contain the raw message, the parameters (form or query), the requested url and the request method,
 * but makes use of a numeric status code, these differences make the outbound one much smaller
 *
 * 2. the inbound can make use of a lot of const strings, the outbound cannot
 */

typedef struct {
	uint8_t                     method;                      // the appropriate http method, GET, POST, PATCH
	uint8_t                     version;                     // the version of the http header (1.0, 1.1, 2.0, ...)
	char                       *raw_message_a;               // the c string containing the entire header, the _a means it's heap allocated
	size_t                      header_len;                  // how many bytes are there in the header
	MiniMap_StringRef_StringRef parameters;                  // contain the data sent in the forms and query parameters
	StringRef                   header_options[RQ_ENUM_LEN]; // an 'hash map' where to store the decoded header options
	StringRef                   url;                         // the resource asked from the client
	StringRef                   body;                        // the content of the message, what the message is about
} InboundHttpMessage;

typedef struct {
	uint16_t                 status_code;    // 200, 404, 500, etc etc
	uint8_t                  version;        // the version of the http header (1.0, 1.1, 2.0, ...)
	MiniMap_u_char_StringOwn header_options; // represent the header as the collection of the single options -> value
	size_t                   header_len;     // how many bytes are there in the header
	StringOwn                body;           // the content of the message, what the message is about
	StringOwn                resource_name;  // the internal complete name for the resource present in the body
} OutboundHttpMessage;

InboundHttpMessage parse_InboundMessage(const char *str);

void destroy_InboundHttpMessage(InboundHttpMessage *mex);
void destroy_OutboundHttpMessage(OutboundHttpMessage *mex);

/**
 * deconstruct the raw header in a map with key (option) -> value
 * and fetch some Metadata such as verb, version, url, a parameters
 * @param rawHeader the StringRef containing the header as a char* string
 * @param msg the httpMessage to store the information extracted
 */
void decompile_header(const StringRef *raw_header, InboundHttpMessage *msg);

/**
 * Analyzes the incoming request for form data / other parameters and puts the result in the given message
 *
 * @param cType content Type (e.g. multipart form-data, plain text, url encoded)
 * @param msg the message to analyze
 */
void decompile_message(InboundHttpMessage *msg);

/**
 * Unite the header and the body in a single message and returns it
 * the compiled message does not have a null terminator and must be freed
 *
 * @param msg the message containing the header options and, eventually, the body
 * @return the 'compiled' message
 */
StringOwn compile_message(const OutboundHttpMessage *msg);

/**
 * Given a String representing the request method (GET, POST, PATCH, ...) returns the relative code
 *
 * @param requestMethod the StringRef containing the method requested
 * @return the code representing the relative method
 */
u_char get_method_code(const StringRef *request_method);

/**
 * Given a string representing the request http version (0., 1.0, 1.1) returns the relative code
 *
 * @param httpVersion the StringRef containing the httpVersion
 * @return the code representing the relative version
 */
u_char get_version_code(const StringRef *http_version);

/**
 * Given a string representing an header options (Content-type, Content-length, ...)
 *
 * @param version the StringRef containing the option
 * @return the code representing the relative option
 */
u_char get_parameter_code(const StringRef *parameter);

/**
 * Given an a string parses it using 'chunkSep' and 'itemSep' and then applies the given function
 *
 * The reason this function makes use of a function pointer + 2 separators is because the code needed to parse header options and some
 * form parameters is the same except these parameters
 *
 * @param str a StringRef pointing to the start of the string to analyze
 * @param fun a function that given key, value and the httpMessage inserts them in the correct map
 * @param chunkSep a string that delimits the separation of chunks (e.g. a single header option, a form value)
 * @param itemSep a char to separate the chunk in key and value (e.g. ':', '=')
 */
void parse_options(const StringRef *segment, void (*fun)(StringRef a, StringRef b, InboundHttpMessage *ctx), const char *chunk_seperator, const char item_separator, InboundHttpMessage *ctx);

// void      parseFormData(const std::string &params, std::string &divisor, std::unordered_map<std::string, std::string> &parameters);

/**
 * Given the header option code and the relative values copies i  in the residente nel
 * dell'utente inizializza
 * of bytes stored
 *
 * @param option the header option code
 * @param value the string value to assign to the option
 * @param msg the httpMessage that contains the header options map
 */
void add_header_option(const HTTPHeaderResponseOption option, const StringRef *value, OutboundHttpMessage *msg);

/**
 * Simple makes sure that a copy of the passed stringRef is inserted in the url of the http message
 *
 * @param val the value to copy and insert
 * @param msg the message to insert it in
 */
void set_filename(const StringRef *val, OutboundHttpMessage *msg);
