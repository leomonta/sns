#include "server.hpp"

#include "logger.hpp"
#include "profiler.hpp"
#include "utils.hpp"

#include "json/include/json.hpp"
#include <filesystem>
#include <mutex>
#include <string.h>
#include <sys/stat.h>
#include <thread>

using json = nlohmann::json;

#define Res Resources

const unsigned char serverVersionMajor = 3;
const unsigned char serverVersionMinor = 3;

namespace Res {

	// Http Server
	std::string baseDirectory = "/";
	short       tcpPort       = 80;

	// for controlling debug prints
	std::map<std::string, std::string> mimeTypes;

	std::thread requestAcceptor;

	Socket serverSocket;
	bool   threadStop = false;
} // namespace Res

int main(int argc, char *argv[]) {

	// Instrumentor::Get().BeginSession("Leonard server", "benchmarks/results.json");

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
				break;
			} else if (strcmp(line, "restart") == 0) {
				restart();
			}

			printf("> ");
		}
	}

	// Instrumentor::Get().EndSession();

	exit(0);

	return 0;
}

void setup() {

	PROFILE_FUNCTION();

	setupContentTypes();

	Res::serverSocket = tcpConn::initializeServer(Res::tcpPort);

	if (Res::serverSocket != INVALID_SOCKET) {
		log(LOG_INFO, "Server Listening on port %d\n	On directory %s\n", Res::tcpPort, Res::baseDirectory.c_str());
	} else {
		log(LOG_FATAL, "TCP connection is invalid. Shutting down\n");
		exit(1);
	}
}

void stop() {

	PROFILE_FUNCTION();

	tcpConn::terminate(Res::serverSocket);

	Res::threadStop = true;

	Res::requestAcceptor.join();
}

void start() {

	PROFILE_FUNCTION();

	Res::threadStop = false;

	Res::requestAcceptor = std::thread(acceptRequests, &Res::threadStop);
}

void restart() {

	PROFILE_FUNCTION();
	stop();
	setup();
	start();
}

void parseArgs(int argc, char *argv[]) {

	PROFILE_FUNCTION();
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
void acceptRequests(bool *threadStop) {

	PROFILE_FUNCTION();

	// used for controlling
	Socket      client = 0;
	std::string request;

	std::vector<std::thread> threads;

	// Receive until the peer shuts down the connection
	while (!(*threadStop)) {

		client = tcpConn::acceptClientSock(Res::serverSocket);

		if (client != -1) {
			// resolveRequest(client, &http);
			threads.push_back(std::thread(resolveRequest, client, threadStop));
		}
	}

	for (size_t i = 0; i < threads.size(); ++i) {
		threads[i].join();
	}
}

/**
 * threaded, resolve the request obtained via the sockey
 */
void resolveRequest(Socket clientSocket, bool *threadStop) {

	PROFILE_FUNCTION();

	int bytesReceived;

	std::string request;
	httpMessage response;

	while (!(*threadStop)) {

		// ---------------------------------------------------------------------- RECEIVE
		bytesReceived = tcpConn::receiveSegment(clientSocket, request);

		// received some bytes
		if (bytesReceived > 0) {

			httpMessage mex(request);

			// no really used
			switch (mex.method) {
			case http::HTTP_HEAD:
				Head(mex, response);
				break;

			case http::HTTP_GET:
				Get(mex, response);
				break;
			}

			// make the message a single formatted string
			auto res = http::compileMessage(response.header, response.body);

			log(LOG_INFO, "[Socket %d] Received request	%s \n", clientSocket, http::methodStr[mex.method]);

			// ------------------------------------------------------------------ SEND
			// acknowledge the segment back to the sender
			tcpConn::sendSegment(clientSocket, res);

			break;
		}

		// received an error
		if (bytesReceived <= 0) {

			break;
		}
	}

	tcpConn::shutdownSocket(clientSocket);
	tcpConn::closeSocket(clientSocket);
}

/**
 * the http method, set the value of result as the header that would have been sent in a GET response
 */
void Head(httpMessage &inbound, httpMessage &outbound) {

	PROFILE_FUNCTION();

	const char *src = inbound.url.c_str();
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
	composeHeader(file, outbound.header);

	// i know that i'm loading an entire file, if i find a better solution i'll use it
	std::string content                      = getFile(file);
	outbound.header[http::RP_Content_Length] = std::to_string(content.length());
	outbound.header[http::RP_Cache_Control]  = "max-age=604800";
	outbound.url                             = file;
}

/**
 * the http method, get both the header and the body
 */
void Get(httpMessage &inbound, httpMessage &outbound) {

	PROFILE_FUNCTION();

	// I just need to add the body to the head,
	Head(inbound, outbound);

	outbound.body = getFile(outbound.url);

	std::string compressed;
	compressGz(compressed, outbound.body.c_str(), outbound.body.length());
	// set the content of the message
	outbound.body = compressed;

	outbound.header[http::RP_Content_Length]   = std::to_string(compressed.length());
	outbound.header[http::RP_Content_Encoding] = "gzip";
}

/**
 * compose the header given the file requested
 */
void composeHeader(const std::string &filename, std::map<int, std::string> &result) {

	PROFILE_FUNCTION();

	// I use map to easily manage key : value, the only problem is when i compile the header, the response code must be at the top

	// if the requested file actually exist
	if (std::filesystem::exists(filename)) {

		// status code OK
		result[http::RP_Status] = "200 OK";

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
		result[http::RP_Content_Type] = content_type;

	} else {
		// status code -> Not Found
		result[http::RP_Status]       = "404 Not Found";
		result[http::RP_Content_Type] = "text/html";
	}

	// various header options

	result[http::RP_Date]       = getUTC();
	result[http::RP_Connection] = "close";
	result[http::RP_Vary]       = "Accept-Encoding";
	char srvr[]                 = "LeonardCustom/x.x (ArchLinux64)";

	srvr[14] = '0' + serverVersionMajor;
	srvr[16] = '0' + serverVersionMinor;

	result[http::RP_Server] = srvr;
}

/**
 * get the file to a string and if its empty return the page 404.html
 * https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c maybe better
 */
std::string getFile(const std::string &file) {

	PROFILE_FUNCTION();

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

void getContentType(const std::string &filetype, std::string &result) {

	PROFILE_FUNCTION();

	auto parts = split(filetype, ".");

	result = Res::mimeTypes[parts.back()];
}