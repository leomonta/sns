#pragma once
#include <fstream>
#include <map>
#include <string>
#include <vector>

#define REQUEST_HEADERS  41
#define RESPONSE_HEADERS 49

class httpMessage;

namespace http {
	void        decompileHeader(const std::string &rawHeader, httpMessage &msg);
	void        decompileMessage(const std::string &cType, std::map<std::string, std::string> &parameters, std::string &body);
	std::string compileMessage(const std::map<int, std::string> &header, const std::string &body);
	int         getMethodCode(const std::string &requestMethod);
	int         getVersionCode(const std::string &httpVersion);
	int         getParameterCode(const std::string &parameter);
	void        parseQueryParameters(const std::string &query, std::map<std::string, std::string> &parameters);
	void        parsePlainParameters(const std::string &params, std::map<std::string, std::string> &parameters);
	void        parseFormData(const std::string &params, std::string &divisor, std::map<std::string, std::string> &parameters);

	static const char *methodStr[] = {
	    "INVALID",
	    "GET",
	    "HEAD",
	    "POST",
	    "PUT",
	    "DELETE",
	    "OPTIONS",
	    "CONNECT",
	    "TRACE",
	    "PATCH"};

	// http method code
	enum methods {
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

	enum versions {
		HTTP_09,
		HTTP_10,
		HTTP_11,
		HTTP_2,
		HTTP_3
	};

	static const char *headerRqStr[] = {
	    "A-IM",
	    "Accept",
	    "Accept-Charset",
	    "Accept-Datetime",
	    "Accept-Encoding",
	    "Accept-Language",
	    "Access-Control-Request-Method",
	    "Access-Control-Request-Headers",
	    "Authorization",
	    "Cache-Control",
	    "Connection",
	    "Content-Encoding",
	    "Content-Length",
	    "Content-MD5",
	    "Content-Type",
	    "Cookie",
	    "Date",
	    "Expect",
	    "Forwarded",
	    "From",
	    "Host",
	    "HTTP2-Settings",
	    "If-Match",
	    "If-Modified-Since",
	    "If-None-Match",
	    "If-Range",
	    "If-Unmodified-Since",
	    "Max-Forwards",
	    "Origin",
	    "Pragma",
	    "Prefer",
	    "Proxy-Authorization",
	    "Range",
	    "Referer",
	    "TE",
	    "Trailer",
	    "Transfer-Encoding",
	    "User-Agent",
	    "Upgrade",
	    "Via",
	    "Warning"};

	static const char *headerRpStr[] = {
	    "Accept-CH",
	    "Access-Control-Allow-Origin",
	    "Access-Control-Allow-Credentials",
	    "Access-Control-Expose-Headers",
	    "Access-Control-Max-Age",
	    "Access-Control-Allow-Methods",
	    "Access-Control-Allow-Headers",
	    "Accept-Patch",
	    "Accept-Ranges",
	    "Age",
	    "Allow",
	    "Alt-Svc",
	    "Cache-Control",
	    "Connection",
	    "Content-Disposition",
	    "Content-Encoding",
	    "Content-Language",
	    "Content-Length",
	    "Content-Location",
	    "Content-MD5",
	    "Content-Range",
	    "Content-Type",
	    "Date",
	    "Delta-Base",
	    "ETag",
	    "Expires",
	    "IM",
	    "Last-Modified",
	    "Link",
	    "Location",
	    "P3P",
	    "Pragma",
	    "Preference-Applied",
	    "Proxy-Authenticate",
	    "Public-Key-Pins",
	    "Retry-After",
	    "Server",
	    "Set-Cookie",
	    "Strict-Transport-Security",
	    "Trailer",
	    "Transfer-Encoding",
	    "Tk",
	    "Upgrade",
	    "Vary",
	    "Via",
	    "Warning",
	    "WWW-Authenticate",
	    "X-Frame-Options",
	    "Status"};

	enum headerOptionRq {
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
	};

	enum headerOptionRp {
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
		RP_Status
	};

} // namespace http

class httpMessage {
public:
	int                                method;     // the appropriate http method
	int                                version;    // the version of the http header (1.0, 1.1, 2.0, ...)
	std::map<int, std::string>         header;     // represent the header as the collection of the single options -> value
	std::map<std::string, std::string> parameters; // contain the data sent in the forms and query parameters
	std::string                        body;       // represent the body as a single string
	std::string                        url;        // the resource asked from the client

	httpMessage(){};

	httpMessage(std::string &raw_message);
};