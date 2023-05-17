/**
 * TODO: add a cli to stop the server
 */
#include <filesystem>
#include <mutex>
#include <thread>

#include "HTTP_conn.hpp"
#include "HTTP_message.hpp"
#include "json.hpp"
#include "utils.hpp"

#undef break

using json = nlohmann::json;

const char *server_init_file = "server_options.ini";

// Http Server
std::string HTTP_Basedir = "/";
std::string HTTP_IP		 = "127.0.0.1";
std::string HTTP_Port	 = "80";

// for controlling debug prints
std::mutex						   mtx;
std::map<std::string, std::string> contents;

void acceptRequests(HTTP_conn *htt, bool *threadStopp);
void resolveRequest(Socket clientSocket, HTTP_conn *httpConnectio, bool *threadStopn);

void		readIni();
void		Head(HTTP_message &inbound, HTTP_message &outbound);
void		Get(HTTP_message &inbound, HTTP_message &outbound);
void		composeHeader(const char *filename, std::map<std::string, std::string> &result);
std::string getFile(const char *file);

void setupContentTypes();

void getContentType(const std::string &filetype, std::string &result);

int main() {

	readIni();
	setupContentTypes();
	// initialize winsock and the server options
	HTTP_conn http(HTTP_Basedir, HTTP_IP, HTTP_Port);

	bool threadStop = false;

	auto requestAcceptor = std::thread(acceptRequests, &http, &threadStop);

	std::string input = "";

	while (true) {
		std::cin >> input;

		if (input.compare("exit") == 0) {
			threadStop = true;
			break;
		}
	}

	requestAcceptor.join();

	return 0;
}

void acceptRequests(HTTP_conn *http, bool *threadStop) {

	// used for controlling
	Socket		client = 0;
	std::string request;

	std::vector<std::thread> threads;

	// Receive until the peer shuts down the connection
	while (!(*threadStop)) {

		client = http->acceptClientSock();

		if (client != -1) {
			// resolveRequest(client, &http);
			threads.push_back(std::thread(resolveRequest, client, http, threadStop));
		}
	}

	for (size_t i = 0; i < threads.size(); ++i) {
		threads[i].join();
	}
}

void resolveRequest(Socket clientSocket, HTTP_conn *httpConnection, bool *threadStop) {

	int bytesReceived;
	int bytesSent;

	std::string	 request;
	HTTP_message response;

	while (!(*threadStop)) {

		// ---------------------------------------------------------------------- RECEIVE
		bytesReceived = httpConnection->receiveRequest(clientSocket, request);

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

			// ------------------------------------------------------------------ SEND
			// acknowledge the segment back to the sender
			bytesSent = httpConnection->sendResponse(clientSocket, response.message);

			// send failed, close socket and close program
			if (bytesSent == 0) {
				std::cout << " [Error]: Send failed with error: " << std::endl;
			}

			break;
		}

		// received an error
		if (bytesReceived <= 0) {
			std::cout << "[Error]: Cannot keep on listening" << std::endl;

			break;
		}
	}

	httpConnection->shutDown(clientSocket);
	httpConnection->closeSocket(clientSocket);
}

/**
 * retrive initilization data from the .ini file
 */
void readIni() {

	std::string				 buf;
	std::fstream			 Read(server_init_file, std::ios::in);
	std::vector<std::string> key_val;

	// read props from the ini file and sets the important variables
	if (Read.is_open()) {
		while (std::getline(Read, buf)) {
			key_val = split(buf, "=");

			if (key_val.size() > 1) {
				key_val[1].erase(key_val[1].find_last_not_of(" \n\r\t") + 1);

				// sectioon [HTTP]
				if (key_val[0] == "IP") {
					HTTP_IP = key_val[1];
				} else if (key_val[0] == "HTTP_Port") {
					HTTP_Port = key_val[1];
				} else if (key_val[0] == "Base_Directory") {
					HTTP_Basedir = key_val[1];
				}
			}
		}

		Read.close();
	} else {
		std::cout << "server_options.ini file not found!\n Proceeding with default values!" << std::endl;
	}
}

/**
 * the http method, set the value of result as the header that would have been sent in a GET response
 */
void Head(HTTP_message &inbound, HTTP_message &outbound) {

	char *dst = (char *)(inbound.filename.c_str());
	urlDecode(dst, inbound.filename.c_str());

	// re set the filename as the base directory and the decoded filename
	std::string file = HTTP_Basedir + dst;

	// usually to request index.html browsers does not specify it, they usually use /, if thats the case I scpecify index.html
	// back access the last char of the string
	if (file.back() == '/') {
		file += "index.html";
	}

	// insert in the outbound message the necessaire header options, filename is used to determine the response code
	composeHeader(file.c_str(), outbound.headerOptions);

	// i know that i'm loading an entire file, if i find a better solution i'll use it
	std::string content						 = getFile(file.c_str());
	outbound.headerOptions["Content-Lenght"] = std::to_string(content.length());
	outbound.filename						 = file;
}

/**
 * the http method, get both the header and the body
 */
void Get(HTTP_message &inbound, HTTP_message &outbound) {

	// I just need to add the body to the head,
	Head(inbound, outbound);

	outbound.rawBody = getFile(outbound.filename.c_str());

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
void composeHeader(const char *filename, std::map<std::string, std::string> &result) {

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
		result["HTTP/1.1"]	   = "404 Not Found";
		result["Content-Type"] = "text/html";
	}

	// various header options

	result["Date"]		 = getUTC();
	result["Connection"] = "close";
	result["Vary"]		 = "Accept-Encoding";
	result["Server"]	 = "LeonardoCustom/3.0 (Win64)";
}

/**
 * get the file to a string and if its empty return the page 404.html
 * https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c maybe better
 */
std::string getFile(const char *file) {

	// get the required file
	std::fstream ifs(file, std::ios::binary | std::ios::in);

	// read the file in one go to a string
	//							start iterator							end iterator
	std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

	// if the file does not exist i load a default 404.html
	if (content.empty()) {
		std::fstream ifs("404.html", std::ios::binary | std::ios::in);
		std::string	 content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	}

	ifs.close();

	return content;
}

void setupContentTypes() {

	contents["abw"]	   = "application/x-abiword";
	contents["aac"]	   = "audio/aac";
	contents["arc"]	   = "application/x-freearc";
	contents["avif"]   = "image/avif";
	contents["aac"]	   = "audio/aac";
	contents["avi"]	   = "video/x-msvideo";
	contents["azw"]	   = "application/vnd.amazon.ebook";
	contents["bin"]	   = "application/octet-stream";
	contents["bmp"]	   = "image/bmp";
	contents["bz"]	   = "application/x-bzip";
	contents["bz2"]	   = "application/x-bzip2";
	contents["cda"]	   = "application/x-cdf";
	contents["csh"]	   = "application/x-csh";
	contents["css"]	   = "text/css";
	contents["csv"]	   = "text/csv";
	contents["doc"]	   = "application/msword";
	contents["docx"]   = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	contents["eot"]	   = "application/vnd.ms-fontobject";
	contents["epub"]   = "application/epub+zip";
	contents["gz"]	   = "application/gzip";
	contents["gif"]	   = "image/gif";
	contents["tml"]	   = "text/html";
	contents["htm"]	   = "text/html";
	contents["html"]   = "text/html";
	contents["ico"]	   = "image/vnd.microsoft.icon";
	contents["ics"]	   = "text/calendar";
	contents["jar"]	   = "application/java-archive";
	contents["jpeg"]   = "image/jpeg";
	contents["jpg"]	   = "image/jpeg";
	contents["js"]	   = "text/javascript";
	contents["json"]   = "application/json";
	contents["jsonld"] = "application/ld+json";
	contents["midi"]   = "audio/midi";
	contents["mid"]	   = "audio/midi";
	contents["mjs"]	   = "text/javascript";
	contents["mp3"]	   = "audio/mpeg";
	contents["mp4"]	   = "video/mp4";
	contents["mpeg"]   = "video/mpeg";
	contents["mpkg"]   = "application/vnd.apple.installer+xml";
	contents["odp"]	   = "application/vnd.oasis.opendocument.presentation";
	contents["ods"]	   = "application/vnd.oasis.opendocument.spreadsheet";
	contents["odt"]	   = "application/vnd.oasis.opendocument.text";
	contents["oga"]	   = "audio/ogg";
	contents["ogv"]	   = "video/ogg";
	contents["ogx"]	   = "application/ogg";
	contents["opus"]   = "audio/opus";
	contents["otf"]	   = "font/otf";
	contents["png"]	   = "image/png";
	contents["pdf"]	   = "application/pdf";
	contents["php"]	   = "application/x-httpd-php";
	contents["ppt"]	   = "application/vnd.ms-powerpoint";
	contents["pptx"]   = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	contents["rar"]	   = "application/vnd.rar";
	contents["rtf"]	   = "application/rtf";
	contents["sh"]	   = "application/x-sh";
	contents["svg"]	   = "image/svg+xml";
	contents["swf"]	   = "application/x-shockwave-flash";
	contents["tar"]	   = "application/x-tar";
	contents["tiff"]   = "image/tiff";
	contents["tif"]	   = "image/tiff";
	contents["ts"]	   = "video/mp2t";
	contents["ttf"]	   = "font/ttf";
	contents["txt"]	   = "text/plain";
	contents["vsd"]	   = "application/vnd.visio";
	contents["wav"]	   = "audio/wav";
	contents["weba"]   = "audio/webm";
	contents["webm"]   = "video/webm";
	contents["webp"]   = "image/webp";
	contents["woff"]   = "font/woff";
	contents["woff2"]  = "font/woff2";
	contents["xhtml"]  = "application/xhtml+xml";
	contents["xls"]	   = "application/vnd.ms-excel";
	contents["xlsx"]   = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	contents["xml"]	   = "application/xml";
	contents["xul"]	   = "application/vnd.mozilla.xul+xml";
	contents["zip"]	   = "application/zip";
	contents["3gp"]	   = "video/3gpp";
	contents["3g2"]	   = "video/3gpp2";
	contents["7z"]	   = "application/x-7z-compressed";
}

void getContentType(const std::string &filetype, std::string &result) {

	auto parts = split(filetype, ".");

	result = contents[parts.back()];
}