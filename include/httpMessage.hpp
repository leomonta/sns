#pragma once
#include "stringRef.hpp"

#include <fstream>
#include <string>
#include <unordered_map>

class httpMessage {
public:
	u_char                                   method;        // the appropriate http method, GET, POST, PATCH
	u_char                                   statusCode;    // 200, 404, 500, etc etc
	u_char                                   version;       // the version of the http header (1.0, 1.1, 2.0, ...)
	std::unordered_map<u_char, stringRef>    headerOptions; // represent the header as the collection of the single options -> value
	size_t                                   headerLen;     // how many bytes are there in the header
	char                                    *rawMessage_a;  // the c string containing the entire header, the _a means it's heap allocated
	std::unordered_map<stringRef, stringRef> parameters;    // contain the data sent in the forms and query parameters
	stringRef                                url;           // the resource asked from the client
	stringRef                                body;          // the content of the message, what the message is about

	httpMessage(){};

	httpMessage(std::string &raw_message);
	httpMessage(const char *raw_message);

	~httpMessage();
};

namespace http {
	void      decompileHeader(const stringRef &rawHeader, httpMessage &msg);
	void      decompileMessage(const stringRef &cType, httpMessage *msg, stringRef &body);
	stringRef compileMessage(const httpMessage &msg);
	u_char    getMethodCode(const stringRef &requestMethod);
	u_char    getVersionCode(const stringRef &httpVersion);
	u_char    getParameterCode(const stringRef &parameter);
	void      parseOptions(const stringRef &head, void (*fun)(stringRef a, stringRef b, httpMessage *ctx), const char *chunkSep, const char itemSep, httpMessage *ctx);
	void      parseFormData(const std::string &params, std::string &divisor, std::unordered_map<std::string, std::string> &parameters);
	void      addHeaderOption(const u_char option, const stringRef &value, httpMessage &msg);
	void      setUrl(const stringRef &val, httpMessage &msg);
	// void        parseQueryParameters(const stringRef &query, std::unordered_map<stringRef, stringRef> &parameters);
	// void        parseHeaderOptions(const stringRef &head, std::unordered_map<int, stringRef> &headerOptions);

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
