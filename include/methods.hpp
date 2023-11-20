#pragma once

#include "httpMessage.hpp"

#include <sslConn.hpp>

const unsigned char serverVersionMajor = 4;
const unsigned char serverVersionMinor = 0;

namespace Methods {

	struct resolver_data {
		SSL   *ssl;
		Socket clientSocket;
	};

	/**
	 * Fills the outbound httpMessage with the correct headers for the Head method
	 *
	 * @param request the request message received from the client
	 * @param response what will be communicated to the client
	 */
	int Head(const inboundHttpMessage &request, outboundHttpMessage &response);

	/**
	 * Fill the outbounf httpMessage with the correct headers and body info for the Get method
	 *
	 * @param request the request message received from the client
	 * @param response what will be communicated to the client
	 */
	void Get(const inboundHttpMessage &request, outboundHttpMessage &response);

	/**
	 * Given the name of a file, retrives all of the info to put in the http header
	 *
	 * @param filename the name of the file
	 * @param msg httpMessage where to put the data
	 * @param fileInfo additional fileinfo presence in the filesystem
	 */
	void composeHeader(const std::string &filename, outboundHttpMessage &msg, const int fileInfo);

	/**
	 * given a path returns the file as a string if it is present, a dir view if it's a directory
	 *
	 * @param path the path to read the file / directory
	 * @param fileInfo info if the file is present or not
	 *
	 * @return the plain text string of the path
	 */
	std::string getContent(const stringRef &path, const int fileInfo);

	/**
	 * Given a path of a directory return its dirview in html
	 *
	 * @param path the path to the directory
	 *
	 * @return the html for the dirview
	 */
	std::string getDirView(const std::string &path);

	/**
	 * loads in the hash map for the mime types
	 */
	void setupContentTypes();

	/**
	 * Initilizes the default directory variable with the given string
	 *
	 * @param str the string used to intializer the baseDirectory internal variable
	 */
	void setupBaseDir(const char *str);
	/**
	 * sets result as the mime type correct for the give fyle extension
	 *
	 * @param filetype the file type / extension
	 * @param result where to put the mimetype
	 */
	void getContentType(const std::string &filetype, std::string &result);

} // namespace Methods
