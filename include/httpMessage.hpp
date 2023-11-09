#pragma once
#include "stringRef.hpp"

#include <fstream>
#include <string>
#include <unordered_map>

/*
I use two representation of an http message for 2 reasons,
1. the inbound include the entire raw message allocated, message used by every stringRef in the struct,
 the outbound message does not contain the raw message, the parameters (form or query), the requested url and the request method,
 but makes use of a numeric status code, these differences make the outbound one much smaller

2. the inbound can make use of a lot of const strings, the outbound cannot
*/

struct inboundHttpMessage {
	u_char                                             m_method;        // the appropriate http method, GET, POST, PATCH
	u_char                                             m_version;       // the version of the http header (1.0, 1.1, 2.0, ...)
	std::unordered_map<u_char, stringRefConst>         m_headerOptions; // represent the header as the collection of the single options -> value
	size_t                                             m_headerLen;     // how many bytes are there in the header
	const char                                        *m_rawMessage_a;  // the c string containing the entire header, the _a means it's heap allocated
	std::unordered_map<stringRefConst, stringRefConst> m_parameters;    // contain the data sent in the forms and query parameters
	stringRefConst                                     m_url;           // the resource asked from the client
	stringRefConst                                     m_body;          // the content of the message, what the message is about
};

struct outboundHttpMessage {

	u_char                                m_statusCode;    // 200, 404, 500, etc etc
	u_char                                m_version;       // the version of the http header (1.0, 1.1, 2.0, ...)
	std::unordered_map<u_char, stringRef> m_headerOptions; // represent the header as the collection of the single options -> value
	size_t                                m_headerLen;     // how many bytes are there in the header
	stringRef                             m_body;          // the content of the message, what the message is about
	stringRef                             m_filename;      // the internal complete filename for the resource present in the body
};

inboundHttpMessage makeInboundMessage(const char *str);
// outboundHttpMessage makeOutboundMessage();

void destroyInboundHttpMessage(const inboundHttpMessage *mex);
void destroyOutboundHttpMessage(const outboundHttpMessage *mex);

namespace http {

	/**
	 * deconstruct the raw header in a map with key (option) -> value
	 * and fetch some Metadata such as verb, version, url, a parameters
	 * @param rawHeader the stringRef containing the header as a char* string
	 * @param msg the httpMessage to store the information extracted
	 */
	void decompileHeader(const stringRefConst &rawHeader, inboundHttpMessage &msg);

	/**
	 * Analyzes the incoming request for form data / other parameters and puts the result in the given message
	 *
	 * @param cType content Type (e.g. multipart form-data, plain text, url encoded)
	 * @param msg the message to analyze
	 */
	void decompileMessage(inboundHttpMessage &msg);

	/**
	 * Unite the header and the body in a single message and returns it
	 * the compiled message does not have a null terminator
	 *
	 * @param msg the message containing the header options and, eventually, the body
	 * @return the 'compiled' message
	 */
	stringRef compileMessage(const outboundHttpMessage &msg);

	/**
	 * Given a string representing the request method (GET, POST, PATCH, ...) returns the relative code
	 *
	 * @param requestMethod the stringRef containing the method requested
	 * @return the code representing the relative method
	 */
	u_char getMethodCode(const stringRefConst &requestMethod);

	/**
	 * Given a string representing the request http version (0., 1.0, 1.1) returns the relative code
	 *
	 * @param httpVersion the stringRef containing the httpVersion
	 * @return the code representing the relative version
	 */
	u_char getVersionCode(const stringRefConst &httpVersion);

	/**
	 * Given a string representing an header options (Content-type, Content-length, ...)
	 *
	 * @param version the stringRef containing the option
	 * @return the code representing the relative option
	 */
	u_char getParameterCode(const stringRefConst &parameter);

	/**
	 * Given an a string parses it using 'chunkSep' and 'itemSep' and then applies the given function
	 *
	 * The reason this function makes use of a function pointer + 2 separators is because the code needed to parse header options and some
	 * form parameters is the same except these parameters
	 *
	 * @param str a stringRef pointing to the start of the string to analyze
	 * @param fun a function that given key, value and the httpMessage inserts them in the correct map
	 * @param chunkSep a string that delimits the separation of chunks (e.g. a single header option, a form value)
	 * @param itemSep a char to separate the chunk in key and value (e.g. ':', '=')
	 */
	void parseOptions(const stringRefConst &segment, void (*fun)(stringRefConst a, stringRefConst b, inboundHttpMessage *ctx), const char *chunkSep, const char itemSep, inboundHttpMessage *ctx);

	// void      parseFormData(const std::string &params, std::string &divisor, std::unordered_map<std::string, std::string> &parameters);

	/**
	 * Given the header option code and the relative values copies it in the httpMessage
	 * This function should always be used instead of directly accessing the header options of the message since this also keeps track of the amount
	 * of bytes stored
	 *
	 * @param option the header option code
	 * @param value the string value to assign to the option
	 * @param msg the httpMessage that contains the header options map
	 */
	void addHeaderOption(const u_char option, const stringRefConst &value, outboundHttpMessage &msg);

	/**
	 * Simple makes sure that a copy of the passed stringRef is inserted in the url of the http message
	 *
	 * @param val the value to copy and insert
	 * @param msg the message to insert it in
	 */
	void setFilename(const stringRefConst &val, outboundHttpMessage &msg);

	// http method code
	enum methods : u_char {
		HTTP_INVALID,
		HTTP_GET,
		HTTP_HEAD,
		HTTP_POST,
		HTTP_PUT,
		HTTP_DELETE,
		HTTP_OPTIONS,
		HTTP_CONNECT,
		HTTP_TRACE,
		HTTP_PATCH
	};

	enum versions : u_char {
		HTTP_09,
		HTTP_10,
		HTTP_11,
		HTTP_2,
		HTTP_3
	};

	enum headerOptionRq : u_char {
		// ReQuest options
		RQ_A_IM,
		RQ_Accept,
		RQ_Accept_Charset,
		RQ_Accept_Datetime,
		RQ_Accept_Encoding,
		RQ_Accept_Language,
		RQ_Access_Control_Request_Method,
		RQ_Access_Control_Request_Headers,
		RQ_Authorization,
		RQ_Cache_Control,
		RQ_Connection,
		RQ_Content_Encoding,
		RQ_Content_Length,
		RQ_Content_MD5,
		RQ_Content_Type,
		RQ_Cookie,
		RQ_Date,
		RQ_Expect,
		RQ_Forwarded,
		RQ_From,
		RQ_Host,
		RQ_HTTP2_Settings,
		RQ_If_Match,
		RQ_If_Modified_Since,
		RQ_If_None_Match,
		RQ_If_Range,
		RQ_If_Unmodified_Since,
		RQ_Max_Forwards,
		RQ_Origin,
		RQ_Pragma,
		RQ_Prefer,
		RQ_Proxy_Authorization,
		RQ_Range,
		RQ_Referer,
		RQ_TE,
		RQ_Trailer,
		RQ_Transfer_Encoding,
		RQ_User_Agent,
		RQ_Upgrade,
		RQ_Via,
		RQ_Warning,
		RQ_ENUM_LEN,
	};

	enum headerOptionRp : u_char {
		// ResPonse Options
		RP_Accept_CH,
		RP_Access_Control_Allow_Origin,
		RP_Access_Control_Allow_Credentials,
		RP_Access_Control_Expose_Headers,
		RP_Access_Control_Max_Age,
		RP_Access_Control_Allow_Methods,
		RP_Access_Control_Allow_Headers,
		RP_Accept_Patch,
		RP_Accept_Ranges,
		RP_Age,
		RP_Allow,
		RP_Alt_Svc,
		RP_Cache_Control,
		RP_Connection,
		RP_Content_Disposition,
		RP_Content_Encoding,
		RP_Content_Language,
		RP_Content_Length,
		RP_Content_Location,
		RP_Content_MD5,
		RP_Content_Range,
		RP_Content_Type,
		RP_Date,
		RP_Delta_Base,
		RP_ETag,
		RP_Expires,
		RP_IM,
		RP_Last_Modified,
		RP_Link,
		RP_Location,
		RP_P3P,
		RP_Pragma,
		RP_Preference_Applied,
		RP_Proxy_Authenticate,
		RP_Public_Key_Pins,
		RP_Retry_After,
		RP_Server,
		RP_Set_Cookie,
		RP_Strict_Transport_Security,
		RP_Trailer,
		RP_Transfer_Encoding,
		RP_Tk,
		RP_Upgrade,
		RP_Vary,
		RP_Via,
		RP_Warning,
		RP_WWW_Authenticate,
		RP_X_Frame_Options,
		RP_ENUM_LEN,
	};

} // namespace http
