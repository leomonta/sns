#include "methods.hpp"

#include "pages.hpp"
#include "utils.hpp"

#include <filesystem>
#include <logger.h>
#include <profiler.hpp>
#include <regex>
#include <sys/stat.h>

// requested file possible states
#define FILE_FOUND            0
#define FILE_NOT_FOUND        1
#define FILE_IS_DIR_FOUND     2
#define FILE_IS_DIR_NOT_FOUND 3

#define Res Resources

namespace Res {

	// Http Server
	std::string baseDirectory;

	// for controlling debug prints
	std::map<std::string, std::string> mimeTypes;
} // namespace Res

int Methods::Head(const http::inboundHttpMessage &request, http::outboundHttpMessage &response) {

	PROFILE_FUNCTION();

	// info about the file requested, to not recheck later
	int fileInfo = FILE_FOUND;

	char *dst = new char[request.m_url.len + 1];
	memcpy(dst, request.m_url.str, request.m_url.len);
	dst[request.m_url.len] = '\0';

	// sinc the source is always longer or the same length of the output i can decode in-place
	urlDecode(dst, dst);

	// re set the filename as the base directory and the decoded filename
	std::string file = Res::baseDirectory + dst;

	llog(LOG_DEBUG, "[SERVER] Decoded '%s'\n", dst);

	delete[] dst;

	// usually to request index.html browsers do not specify it, they usually use /, if that's the case I add index.html
	// back access the last char of the string

	struct stat fileStat;
	auto        errCode = stat(file.c_str(), &fileStat);

	if (errCode != 0) {
		llog(LOG_WARNING, "[SERVER] File requested (%s) not found, %s\n", file.c_str(), strerror(errno));

		fileInfo = FILE_NOT_FOUND;
	}

	// check of I'm dealing with a directory
	if (S_ISDIR(fileStat.st_mode) || file.back() == '/') {
		auto correctedFile = file + "/index.html";
		errCode            = stat(correctedFile.c_str(), &fileStat);

		// index exists, use that
		if (errCode == 0) {
			llog(LOG_DEBUG, "[SERVER] Automatically added index.html on the url\n");
			fileInfo = FILE_IS_DIR_FOUND;
			file     = correctedFile;
		} else { // file does not exists use dir view
			llog(LOG_WARNING, "[SERVER] The file requested is a directory with no index.html. Fallback to dir view\n");
			fileInfo = FILE_IS_DIR_NOT_FOUND;
		}
	}

	// insert in the response message the necessaire header options, filename is used to determine the response code
	Methods::composeHeader(file, response, fileInfo);

	auto fileSize = std::to_string(fileStat.st_size);

	http::addHeaderOption(http::RP_Content_Length, {fileSize.c_str(), fileSize.size()}, response);
	http::addHeaderOption(http::RP_Cache_Control, {"max-age=3600", 12}, response);

	http::setFilename({file.c_str(), file.size()}, response);

	return fileInfo;
}

void Methods::Get(const http::inboundHttpMessage &request, http::outboundHttpMessage &response) {
	PROFILE_FUNCTION();

	// I just need to add the body to the head,
	auto fileInfo = Methods::Head(request, response);

	auto uncompressed = Methods::getContent(response.m_filename, fileInfo);

	std::string compressed = "";
	if (uncompressed != "") {
		compressGz({uncompressed.c_str(), uncompressed.length()}, compressed);
		llog(LOG_DEBUG, "[SERVER] Compressing response body\n");

		if (fileInfo == FILE_IS_DIR_NOT_FOUND) {
			http::addHeaderOption(http::RP_Content_Type, {"text/html", 9}, response);
		}
	}

	// set the content of the message
	char *temp      = makeCopyConst({compressed.c_str(), compressed.size()});
	response.m_body = {temp, compressed.size()};

	auto lenStr = std::to_string(compressed.length());
	http::addHeaderOption(http::RP_Content_Length, {lenStr.c_str(), lenStr.size()}, response);
	http::addHeaderOption(http::RP_Content_Encoding, {"gzip", 4}, response);
}

void Methods::composeHeader(const std::string &filename, http::outboundHttpMessage &msg, const int fileInfo) {

	PROFILE_FUNCTION();

	// I use map to easily manage key : value, the only problem is when i compile the header, the response code must be at the top
	// if the requested file actually exist
	if (fileInfo != FILE_NOT_FOUND) {

		// status code OK
		// result[http::RP_Status] = "200 OK";

		// get the content type
		std::string content_type = "";
		Methods::getContentType(filename, content_type);
		if (content_type == "") {
			content_type = "text/plain"; // fallback if finds nothing
		}

		// and actually add it in
		http::addHeaderOption(http::RP_Content_Type, {content_type.c_str(), content_type.size()}, msg);

	} else {
		// result[http::RP_Status]       = "404 Not Found";
		http::addHeaderOption(http::RP_Content_Type, {"text/html", 9}, msg);
	}

	// various header options

	auto UTC = getUTC();
	http::addHeaderOption(http::RP_Date, {UTC.c_str(), UTC.size()}, msg);
	http::addHeaderOption(http::RP_Connection, {"close", 5}, msg);
	http::addHeaderOption(http::RP_Vary, {"Accept-Encoding", 15}, msg);
	char srvr[] = "sns/x.x (ArchLinux64)";

	srvr[4] = '0' + serverVersionMajor;
	srvr[6] = '0' + serverVersionMinor;

	http::addHeaderOption(http::RP_Server, {srvr, 21}, msg);
}

std::string Methods::getContent(const stringRef &path, const int fileInfo) {

	PROFILE_FUNCTION();

	std::string content;
	std::string fileStr(path.str, path.len);

	if (fileInfo == FILE_FOUND || fileInfo == FILE_IS_DIR_FOUND) {

		// get the required file
		std::fstream ifs(fileStr, std::ios::binary | std::ios::in);

		// read the file in one go to a string
		//							start iterator							end iterator
		content.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

		ifs.close();
	}

	// if the file does not exist i load a default 404.html
	if (content.empty()) {

		// Load the internal 404 error page
		if (fileInfo == FILE_IS_DIR_NOT_FOUND) {
			llog(LOG_WARNING, "[SERVER] File not found. Loading the dir view\n");
			content = Methods::getDirView(fileStr);
		} else {
			llog(LOG_WARNING, "[SERVER] File not found. Loading deafult Error 404 page\n");
			content = Not_Found_Page;
		}
	}

	return content;
}

std::string Methods::getDirView(const std::string &path) {

	std::string dirItems;

	for (const auto &entry : std::filesystem::directory_iterator(path)) {
		auto        filename  = static_cast<std::string>(entry.path().filename());
		auto        url       = "./" + filename;
		auto        cftime    = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(entry.last_write_time()));
		std::string timestamp = std::asctime(std::localtime(&cftime));
		// timestamp.pop_back(); // remove the trailing \n

		std::string item = Dir_View_Page_Item;
		item             = std::regex_replace(item, std::regex("URL"), url);
		item             = std::regex_replace(item, std::regex("FILENAME"), filename);
		item             = std::regex_replace(item, std::regex("TIMESTAMP"), timestamp);
		dirItems.append(item);
	}

	std::string content = Dir_View_Page_Pre + dirItems + Dir_View_Page_Post;

	return content;
}

void Methods::setupContentTypes() {

	PROFILE_FUNCTION();

	Res::mimeTypes["abw"]    = "application/x-abiword";
	Res::mimeTypes["aac"]    = "audio/aac";
	Res::mimeTypes["arc"]    = "application/x-freearc";
	Res::mimeTypes["avif"]   = "image/avif";
	Res::mimeTypes["aac"]    = "audio/aac";
	Res::mimeTypes["avi"]    = "video/x-msvideo";
	Res::mimeTypes["azw"]    = "application/vnd.amazon.ebook";
	Res::mimeTypes["bin"]    = "application/octet-stream";
	Res::mimeTypes["bmp"]    = "image/bmp";
	Res::mimeTypes["bz"]     = "application/x-bzip";
	Res::mimeTypes["bz2"]    = "application/x-bzip2";
	Res::mimeTypes["cda"]    = "application/x-cdf";
	Res::mimeTypes["csh"]    = "application/x-csh";
	Res::mimeTypes["css"]    = "text/css";
	Res::mimeTypes["csv"]    = "text/csv";
	Res::mimeTypes["doc"]    = "application/msword";
	Res::mimeTypes["docx"]   = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	Res::mimeTypes["eot"]    = "application/vnd.ms-fontobject";
	Res::mimeTypes["exe"]    = "application/octet-stream";
	Res::mimeTypes["epub"]   = "application/epub+zip";
	Res::mimeTypes["gz"]     = "application/gzip";
	Res::mimeTypes["gif"]    = "image/gif";
	Res::mimeTypes["tml"]    = "text/html";
	Res::mimeTypes["htm"]    = "text/html";
	Res::mimeTypes["html"]   = "text/html";
	Res::mimeTypes["ico"]    = "image/vnd.microsoft.icon";
	Res::mimeTypes["ics"]    = "text/calendar";
	Res::mimeTypes["jar"]    = "application/java-archive";
	Res::mimeTypes["jpeg"]   = "image/jpeg";
	Res::mimeTypes["jpg"]    = "image/jpeg";
	Res::mimeTypes["js"]     = "text/javascript";
	Res::mimeTypes["json"]   = "application/json";
	Res::mimeTypes["jsonld"] = "application/ld+json";
	Res::mimeTypes["midi"]   = "audio/midi";
	Res::mimeTypes["mid"]    = "audio/midi";
	Res::mimeTypes["mjs"]    = "text/javascript";
	Res::mimeTypes["mp3"]    = "audio/mpeg";
	Res::mimeTypes["mp4"]    = "video/mp4";
	Res::mimeTypes["mpeg"]   = "video/mpeg";
	Res::mimeTypes["mpkg"]   = "application/vnd.apple.installer+xml";
	Res::mimeTypes["odp"]    = "application/vnd.oasis.opendocument.presentation";
	Res::mimeTypes["ods"]    = "application/vnd.oasis.opendocument.spreadsheet";
	Res::mimeTypes["odt"]    = "application/vnd.oasis.opendocument.text";
	Res::mimeTypes["oga"]    = "audio/ogg";
	Res::mimeTypes["ogv"]    = "video/ogg";
	Res::mimeTypes["ogx"]    = "application/ogg";
	Res::mimeTypes["opus"]   = "audio/opus";
	Res::mimeTypes["otf"]    = "font/otf";
	Res::mimeTypes["png"]    = "image/png";
	Res::mimeTypes["pdf"]    = "application/pdf";
	Res::mimeTypes["php"]    = "application/x-httpd-php";
	Res::mimeTypes["ppt"]    = "application/vnd.ms-powerpoint";
	Res::mimeTypes["pptx"]   = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	Res::mimeTypes["rar"]    = "application/vnd.rar";
	Res::mimeTypes["rtf"]    = "application/rtf";
	Res::mimeTypes["sh"]     = "application/x-sh";
	Res::mimeTypes["svg"]    = "image/svg+xml";
	Res::mimeTypes["swf"]    = "application/x-shockwave-flash";
	Res::mimeTypes["tar"]    = "application/x-tar";
	Res::mimeTypes["tiff"]   = "image/tiff";
	Res::mimeTypes["tif"]    = "image/tiff";
	Res::mimeTypes["ts"]     = "video/mp2t";
	Res::mimeTypes["ttf"]    = "font/ttf";
	Res::mimeTypes["txt"]    = "text/plain";
	Res::mimeTypes["vsd"]    = "application/vnd.visio";
	Res::mimeTypes["wav"]    = "audio/wav";
	Res::mimeTypes["weba"]   = "audio/webm";
	Res::mimeTypes["webm"]   = "video/webm";
	Res::mimeTypes["webp"]   = "image/webp";
	Res::mimeTypes["woff"]   = "font/woff";
	Res::mimeTypes["woff2"]  = "font/woff2";
	Res::mimeTypes["xhtml"]  = "application/xhtml+xml";
	Res::mimeTypes["xls"]    = "application/vnd.ms-excel";
	Res::mimeTypes["xlsx"]   = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	Res::mimeTypes["xml"]    = "application/xml";
	Res::mimeTypes["xul"]    = "application/vnd.mozilla.xul+xml";
	Res::mimeTypes["zip"]    = "application/zip";
	Res::mimeTypes["3gp"]    = "video/3gpp";
	Res::mimeTypes["3g2"]    = "video/3gpp2";
	Res::mimeTypes["7z"]     = "application/x-7z-compressed";
}

void Methods::getContentType(const std::string &filetype, std::string &result) {

	PROFILE_FUNCTION();

	auto parts = split(filetype, ".");

	result = Res::mimeTypes[parts.back()];
}

void Methods::setupBaseDir(const char *str) {
	Res::baseDirectory = str;
}
