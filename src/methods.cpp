#include "methods.hpp"

#include "pages.hpp"
#include "stringRef.hpp"
#include "utils.hpp"

#include <cstdio>
#include <cstring>
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

namespace Res {

	// Http Server
	stringRefConst baseDirectory;

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

	// since the source is always longer or the same length of the output i can decode in-place
	urlDecode(dst, dst);
	size_t url_len = strlen(dst);


	#define INDEX_HTML "/index.html"
	#define INDEX_HTML_LEN 11

	// allocate space for, the base dir + the filename + index.html that might be added on top
	size_t file_str_len = Res::baseDirectory.len + url_len + INDEX_HTML_LEN + 1;
	char  *file         = new char[file_str_len];

	file[file_str_len - 1] = '\0';

	strncpy(file, Res::baseDirectory.str, Res::baseDirectory.len);
	size_t file_end = Res::baseDirectory.len;
	strncpy(file + file_end, dst, url_len);
	file_end += url_len;
	file[file_end] = '\0';

	llog(LOG_DEBUG, "[SERVER] Decoded '%s'\n", dst);

	delete[] dst;

	// usually to request index.html browsers do not specify it, they usually use /, if that's the case I add index.html
	struct stat fileStat;
	auto        errCode = stat(file, &fileStat);

	if (errCode != 0) {
		llog(LOG_WARNING, "[SERVER] File requested (%s) not found, %s\n", file, strerror(errno));
		fileInfo = FILE_NOT_FOUND;
	}

	// check of I'm dealing with a directory
	if (S_ISDIR(fileStat.st_mode) || file[file_end - 1] == '/') {
		strncpy(file + file_end, INDEX_HTML, INDEX_HTML_LEN);
		errCode            = stat(file, &fileStat);

		// index exists, use that
		if (errCode == 0) {
			llog(LOG_DEBUG, "[SERVER] Automatically added index.html on the url\n");
			fileInfo = FILE_IS_DIR_FOUND;
			file_end += INDEX_HTML_LEN;
		} else { // file does not exists use dir view
			llog(LOG_WARNING, "[SERVER] The file requested is a directory with no index.html. Fallback to dir view\n");
			fileInfo = FILE_IS_DIR_NOT_FOUND;
		}
	}
	file[file_end] = '\0';

	// insert in the response message the necessaire header options, filename is used to determine the response code
	Methods::composeHeader(file, response, fileInfo);

	llog(LOG_DEBUG, "[SERVER] Header Composed\n");

	auto fileSize = std::to_string(fileStat.st_size);

	http::addHeaderOption(http::RP_Content_Length, {fileSize.c_str(), fileSize.size()}, response);
	http::addHeaderOption(http::RP_Cache_Control, {"max-age=3600", 12}, response);
	llog(LOG_DEBUG, "[SERVER] Fized headers set\n");

	http::setFilename({file, file_str_len}, response);

	delete[] file;

	llog(LOG_DEBUG, "[SERVER] Fixed headers set\n");

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
		msg.m_statusCode = 200;

		// get the content type
		std::string content_type = "";
		Methods::getContentType(filename, content_type);
		if (content_type == "") {
			content_type = "text/plain"; // fallback if finds nothing
		}

		// and actually add it in
		http::addHeaderOption(http::RP_Content_Type, {content_type.c_str(), content_type.size()}, msg);

	} else {
		msg.m_statusCode = 404;
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
		auto        url       = filename;
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


void Methods::getContentType(const std::string &filetype, std::string &result) {

	PROFILE_FUNCTION();

	auto parts = split(filetype, ".");

	result = Res::mimeTypes[parts.back()];
}

void Methods::setup(stringRefConst str) {
	Methods::setupContentTypes(Res::mimeTypes);
	Res::baseDirectory = str;
}
