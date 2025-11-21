#include "methods.h"

#include "StringRef.h"
#include "constants.h"
#include "pages.h"
#include "utils.h"

#include <errno.h>
#include <logger.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// requested file possible states
#define FILE_FOUND            0
#define FILE_NOT_FOUND        1
#define FILE_IS_DIR_FOUND     2
#define FILE_IS_DIR_NOT_FOUND 3

const char INDEX_HTML[] = {'/', 'i', 'n', 'd', 'e', 'x', '.', 'h', 't', 'm', 'l'};
#define INDEX_HTML_LEN 11

static struct Res {

	// Http Server
	StringRef base_directory;

	// for controlling debug prints
	MiniMap_StringRef_StringRef mime_types;
} resources;

int Head(const InboundHttpMessage *request, OutboundHttpMessage *response) {

	// info about the file requested, to not recheck later
	int file_info = FILE_FOUND;

	StringOwn dst = {malloc(request->url.len + 1), request->url.len + 1};
	TEST_ALLOC(dst.str)
	dst.str[request->url.len] = '\0';

	// since the source is always longer or the same length of the output i can decode in-place
	url_decode(&dst, (StringRef *)(&request->url));
	dst.len = strlen(dst.str);

	// allocate space for, the base dir + the filename + index.html that might be added on top
	size_t    file_str_len = resources.base_directory.len + dst.len + INDEX_HTML_LEN;
	StringOwn file         = {malloc(file_str_len + 1), file_str_len};
	TEST_ALLOC(file.str)

	file.str[file_str_len] = '\0';

	strncpy(file.str, resources.base_directory.str, resources.base_directory.len);
	size_t file_end = resources.base_directory.len;
	strncpy(file.str + file_end, dst.str, dst.len);
	file_end += dst.len;
	file.str[file_end] = '\0';

	llog(LOG_DEBUG, "[SERVER] Decoded '%s'\n", dst.str);

	free(dst.str);

	// usually to request index.html browsers do not specify it, they usually use /, if that's the case I add index.html
	struct stat file_stat;
	auto        errCode = stat(file.str, &file_stat);

	if (errCode != 0) {
		llog(LOG_WARNING, "[SERVER] File requested (%*s) not found, %s\n", (int)file.len, file.str, strerror(errno));
		file_info = FILE_NOT_FOUND;
	}

	// check of I'm dealing with a directory
	if (S_ISDIR(file_stat.st_mode) || file.str[file_end - 1] == '/') {
		strncpy(file.str + file_end, INDEX_HTML, INDEX_HTML_LEN);
		errCode = stat(file.str, &file_stat);

		// index exists, use that
		if (errCode == 0) {
			llog(LOG_DEBUG, "[SERVER] Automatically added index.html on the url\n");
			file_info = FILE_IS_DIR_FOUND;
			file_end += INDEX_HTML_LEN;
		} else { // file does not exists use dir view
			llog(LOG_WARNING, "[SERVER] The file requested is a directory with no index.html. Fallback to dir view\n");
			file_info = FILE_IS_DIR_NOT_FOUND;
		}
	}
	file.str[file_end] = '\0';

	// insert in the response message the necessaire header options, filename is used to determine the response code
	compose_header((StringRef *)&file, response, file_info);

	StringRef sz            = {"0", 1};
	StringRef cache_control = {"max-age=3600", 12};
	add_header_option(RP_CONTENT_LENGTH, &sz, response);
	add_header_option(RP_CACHE_CONTROL, &cache_control, response);

	set_filename((StringRef *)&file, response);

	free(file.str);

	return file_info;
}

void Get(const InboundHttpMessage *request, OutboundHttpMessage *response) {

	// I just need to add the body to the head,
	auto file_info = Head(request, response);

	StringOwn proxy        = get_content(&response->resource_name, file_info);
	StringRef uncompressed = {proxy.str, proxy.len};
	bool      do_free      = true;
	if (uncompressed.len == 0) {
		llog(LOG_WARNING, "[SERVER] File not found. Loading deafult Error 404 page\n");

		uncompressed = Not_Found_Page;
		do_free      = false;
	}

	StringOwn compressed = {nullptr, 0};
	if (uncompressed.len != 0) {
		compress_gz(&uncompressed, &compressed);
		llog(LOG_DEBUG, "[SERVER] Compressing response body\n");

		if (file_info == FILE_IS_DIR_NOT_FOUND) {
			StringRef def = {"text/html", 9};
			add_header_option(RP_CONTENT_TYPE, &def, response);
		}
	}

	// set the content of the message
	char *temp     = copy_StringOwn(&compressed);
	response->body = (StringOwn){temp, compressed.len};

	auto len_str = num_to_string(compressed.len);
	auto gzip    = (StringRef){"gzip", 4};
	add_header_option(RP_CONTENT_LENGTH, (StringRef *)&len_str, response);
	add_header_option(RP_CONTENT_ENCODING, &gzip, response);

	if (do_free) {
		free((void *)uncompressed.str);
	}
}

void compose_header(const StringRef *filename, OutboundHttpMessage *msg, const int fileInfo) {

	// I use map to easily manage key : value, the only problem is when i compile the header, the response code must be at the top
	// if the requested file actually exist
	if (fileInfo != FILE_NOT_FOUND) {

		// status code OK
		msg->status_code = 200;

		// get the content type
		StringRef content_type;
		get_content_type(filename, &content_type);
		if (content_type.len == 0) {
			content_type = CAST_STRINGREF("text/plain"); // fallback if finds nothing
		}

		// and actually add it in
		add_header_option(RP_CONTENT_TYPE, &content_type, msg);

	} else {
		msg->status_code = 404;
		StringRef def    = {"text/html", 9};
		add_header_option(RP_CONTENT_TYPE, &def, msg);
	}

	// various header options

	auto UTC = getUTC();
	add_header_option(RP_DATE, &UTC, msg);
	StringRef connection = {"close", 5};
	add_header_option(RP_CONNECTION, &connection, msg);
	StringRef vary = {"Accept-Encoding", 15};
	add_header_option(RP_VARY, &vary, msg);
	char buffer[22];

	snprintf(buffer, 22, "sns/%c.%c (ArchLinux64)", '0' + server_version_major, '0' + server_version_minor);

	StringOwn server = {buffer, 21};

	add_header_option(RP_SERVER, (StringRef *)&server, msg);
}

StringOwn get_content(const StringOwn *path, const int file_info) {

	StringOwn content = {nullptr, 0};

	// nothing to see here folks
	if (file_info != FILE_FOUND && file_info != FILE_IS_DIR_FOUND) {
		return content;
	}

	// FIXME: loading the entire file inside memory, is this bad?
	FILE *f = fopen(path->str, "rb");
	// DEFER fclose(f)

	if (f == nullptr) {
		llog(LOG_ERROR, "[FILE] Could not read file %*s: %s\n", (int)path->len, path->str, strerror(errno));
		return content;
	}

	fseek(f, 0, SEEK_END);
	content.len = (size_t)ftell(f);

	// empty file, no need to malloc anything (thank you analyzer)
	if (content.len == 0) {
		fclose(f);
		return content;
	}

	fseek(f, 0, SEEK_SET);
	content.str = malloc(content.len + 1);
	TEST_ALLOC(content.str)
	if (content.str) {
		fread(content.str, 1, content.len, f);
	}
	fclose(f);
	content.str[content.len] = '\0';

	/*
	if (content.len == 0) {

	    if (file_info == FILE_IS_DIR_NOT_FOUND) {
	        llog(LOG_WARNING, "[SERVER] File not found. Loading the dir view\n");
	        content = get_dir_view((StringRef *)path);
	    }
	}
	*/

	return content;
}

StringOwn get_dir_view(const StringRef *path) {

	return (StringOwn){"", 0};
	/*
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
	*/
}

void get_content_type(const StringRef *filetype, StringRef *result) {

	auto after_dot = strrnchr(filetype->str, '.', filetype->len) + 1;

	StringRef part = {after_dot, strnlen(after_dot, (size_t)(filetype->len + filetype->str - after_dot))};

	MiniMap_StringRef_StringRef_get(&resources.mime_types, &part, equal_StringRef, result);
}

void setup_methods(StringRef *base_directory) {
	resources.mime_types = MiniMap_StringRef_StringRef_make(10);

	setup_content_types(&resources.mime_types);
	resources.base_directory.str = copy_StringRef(base_directory);
	resources.base_directory.len = base_directory->len;
}
