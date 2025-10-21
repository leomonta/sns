#pragma once

#include "HttpMessage.h"
#include "StringRef.h"

#include <sslConn.h>

typedef struct {
	SSL   *ssl;
	Socket client_socket;
} ResolverData;

static const ResolverData EMPTY_RESOLVER_DATA = {nullptr, INVALID_SOCKET};

/**
 * Fills the outbound httpMessage with the correct headers for the Head method
 *
 * @param request the request message received from the client
 * @param response what will be communicated to the client
 */
int Head(const InboundHttpMessage *request, OutboundHttpMessage *response);

/**
 * Fill the outbounf httpMessage with the correct headers and body info for the Get method
 *
 * @param request the request message received from the client
 * @param response what will be communicated to the client
 */
void Get(const InboundHttpMessage *request, OutboundHttpMessage *response);

/**
 * Given the name of a file, retrives all of the info to put in the http header
 *
 * @param filename the name of the file
 * @param msg httpMessage where to put the data
 * @param fileInfo additional fileinfo presence in the filesystem
 */
void compose_header(const StringRef *filename, OutboundHttpMessage *msg, const int file_info);

/**
 * given a path returns the file as a string if it is present, a dir view if it's a directory
 *
 * @param path the path to read the file / directory
 * @param fileInfo info if the file is present or not
 *
 * @return the plain text string of the path
 */
StringOwn get_content(const StringOwn *path, const int file_info);

/**
 * Given a path of a directory return its dirview in html
 *
 * @param path the path to the directory
 *
 * @return the html for the dirview
 */
StringOwn get_dir_view(const StringRef *path);

/**
 * loads in the hash map for the mime types
 */
void setup_content_types(MiniMap_StringRef_StringRef *mime_types);

/**
 * Initilizes the default directory variable with the given string
 *
 * @param str the string used to intializer the base_directory internal variable
 */
void setup_methods(StringRef *base_directory);
/**
 * sets result as the mime type correct for the give fyle extension
 *
 * @param filetype the file type / extension
 * @param result where to put the mimetype
 */
void get_content_type(const StringRef *filetype, StringRef *result);
