#include "server.hpp"

#include "logger.hpp"
#include "utils.hpp"

#include "json/include/json.hpp"
#include <filesystem>
#include <mutex>
#include <string.h>
#include <sys/stat.h>
#include <thread>

using json = nlohmann::json;

#define Res Resources

namespace Res {

	// Http Server
	std::string baseDirectory = "/";
	short       tcpPort       = 80;

	// for controlling debug prints
	std::mutex                         mtx;
	std::map<std::string, std::string> mimeTypes;

	std::thread requestAcceptor;

	tcpConn conn;
	bool    threadStop = false;
} // namespace Res

int main(int argc, char *argv[]) {

	parseArgs(argc, argv);

	setup();

	start();

	// cli like interface
	while (true) {
		char line[256];
		if (fgets(line, sizeof(line), stdin) != nullptr) {
			trimwhitespace(line);

			// simple commands
			if (strcmp(line, "exit") == 0) {
				stop();
				exit(0);
			} else if (strcmp(line, "restart") == 0) {
				restart();
			}

			printf("> ");
		}
	}

	stop();

	return 0;
}

void setup() {

	setupContentTypes();

	Res::conn.initialize(Res::tcpPort);

	if (Res::conn.isConnValid) {
		log(LOG_INFO, "Server Listening on port %d\n	On directory %s\n", Res::tcpPort, Res::baseDirectory.c_str());
	} else {
		log(LOG_FATAL, "TCP connection is invalid. Shutting down\n");
		exit(1);
	}
}

void stop() {

	Res::conn.terminate();

	Res::threadStop = true;

	Res::requestAcceptor.join();
}

void start() {

	Res::threadStop = false;

	Res::requestAcceptor = std::thread(acceptRequests, &Res::conn, &Res::threadStop);
}

void restart() {
	stop();
	setup();
	start();
}

void parseArgs(int argc, char *argv[]) {
	// server directory port
	//    0       1      3

	switch (argc) {
	case 3:
		Res::tcpPort = std::atoi(argv[3]);

	case 2:
		Res::baseDirectory = argv[1];
	}
}

/**
 * Actively wait for clients, if one is received spawn a thread and continue
 */
void acceptRequests(tcpConn *tcpConnection, bool *threadStop) {

	// used for controlling
	Socket      client = 0;
	std::string request;

	std::vector<std::thread> threads;

	// Receive until the peer shuts down the connection
	while (!(*threadStop)) {

		client = tcpConnection->acceptClientSock();

		if (client != -1) {
			// resolveRequest(client, &http);
			threads.push_back(std::thread(resolveRequest, client, tcpConnection, threadStop));
		}
	}

	for (size_t i = 0; i < threads.size(); ++i) {
		threads[i].join();
	}
}

/**
 * threaded, resolve the request obtained via the sockey
 */
void resolveRequest(Socket clientSocket, tcpConn *tcpConnection, bool *threadStop) {

	int bytesReceived;
	int bytesSent;

	std::string  request;
	HTTP_message response;

	while (!(*threadStop)) {

		// ---------------------------------------------------------------------- RECEIVE
		bytesReceived = tcpConnection->receiveRequest(clientSocket, request);

		// received some bytes
		if (bytesReceived > 0) {

			HTTP_message mex(request);

			// no really used
			switch (mex.method) {
			case HTTP_HEAD:
				Head(mex, response);
				break;

			case HTTP_GET:
				Get(mex, response);
				break;
			}

			// make the message a single formatted string
			response.compileMessage();

			log(LOG_INFO, "[Socket %d] Received request	%s \n", clientSocket, mex.headerOptions["Method"].c_str());

			// ------------------------------------------------------------------ SEND
			// acknowledge the segment back to the sender
			bytesSent = tcpConnection->sendResponse(clientSocket, response.message);

			break;
		}

		// received an error
		if (bytesReceived <= 0) {

			break;
		}
	}

	tcpConnection->shutDown(clientSocket);
	tcpConnection->closeSocket(clientSocket);
}

/**
 * the http method, set the value of result as the header that would have been sent in a GET response
 */
void Head(HTTP_message &inbound, HTTP_message &outbound) {

	const char *src = inbound.filename.c_str();
	char       *dst = new char[strlen(src) + 1];
	urlDecode(dst, src);

	// re set the filename as the base directory and the decoded filename
	std::string file = Res::baseDirectory + dst;

	delete[] dst;

	// usually to request index.html browsers does not specify it, they usually use /, if thats the case I scpecify index.html
	// back access the last char of the string
	if (file.back() == '/') {
		file += "index.html";
	}

	// check of I'm dealing with a directory
	struct stat fileStat;
	stat(file.c_str(), &fileStat);

	if (S_ISDIR(fileStat.st_mode)) {

		file += "/index.html";
	}

	// insert in the outbound message the necessaire header options, filename is used to determine the response code
	composeHeader(file, outbound.headerOptions);

	// i know that i'm loading an entire file, if i find a better solution i'll use it
	std::string content                      = getFile(file);
	outbound.headerOptions["Content-Lenght"] = std::to_string(content.length());
	outbound.headerOptions["Cache-Control"]  = "max-age=604800";
	outbound.filename                        = file;
}

/**
 * the http method, get both the header and the body
 */
void Get(HTTP_message &inbound, HTTP_message &outbound) {

	// I just need to add the body to the head,
	Head(inbound, outbound);

	outbound.rawBody = getFile(outbound.filename);

	std::string compressed;
	compressGz(compressed, outbound.rawBody.c_str(), outbound.rawBody.length());
	// set the content of the message
	outbound.rawBody = compressed;

	outbound.headerOptions["Content-Lenght"]   = std::to_string(compressed.length());
	outbound.headerOptions["Content-Encoding"] = "gzip";
}

/**
 * compose the header given the file requested
 */
void composeHeader(const std::string &filename, std::map<std::string, std::string> &result) {

	// I use map to easily manage key : value, the only problem is when i compile the header, the response code must be at the top

	// if the requested file actually exist
	if (std::filesystem::exists(filename)) {

		// status code OK
		result["HTTP/1.1"] = "200 OK";

		// get the file extension, i'll use it to get the content type
		std::string temp = split(filename, ".").back(); // + ~24 alloc

		// get the content type
		std::string content_type = "";
		getContentType(temp, content_type);

		// fallback if finds nothing
		if (content_type == "") {
			content_type = "text/plain";
		}

		// and actually add it in
		result["Content-Type"] = content_type;

	} else {
		// status code Not Found
		result["HTTP/1.1"]     = "404 Not Found";
		result["Content-Type"] = "text/html";
	}

	// various header options

	result["Date"]       = getUTC();
	result["Connection"] = "close";
	result["Vary"]       = "Accept-Encoding";
	result["Server"]     = "LeonardCustom/3.2 (ArchLinux64)";
}

/**
 * get the file to a string and if its empty return the page 404.html
 * https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c maybe better
 */
std::string getFile(const std::string &file) {

	// get the required file
	std::fstream ifs(file, std::ios::binary | std::ios::in);

	// read the file in one go to a string
	//							start iterator							end iterator
	std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

	// if the file does not exist i load a default 404.html
	if (content.empty()) {
		log(LOG_WARNING, "File %s not found\n", file.c_str());
		std::fstream ifs("404.html", std::ios::binary | std::ios::in);
		std::string  content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	}

	ifs.close();

	return content;
}

void setupContentTypes() {

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

void getContentType(const std::string &filetype, std::string &result) {

	auto parts = split(filetype, ".");

	result = Res::mimeTypes[parts.back()];
}