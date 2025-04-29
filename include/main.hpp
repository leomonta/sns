#pragma once

#include "server.hpp"
#include "stringRef.hpp"

struct cliArgs {
	unsigned short tcpPort;
	stringRef      baseDir;
};

/**
 * given the cli arguments set's the server options accordingly
 *
 * @param argc the count of arguments
 * @param argv the values of the arguments
 */
cliArgs parseArgs(const int argc, const char *argv[]);

/**
 * Setup the server, loads libraries and stuff
 *
 * to call one
 */
runtimeInfo setup(cliArgs args);

/**
 * stop the server and its threads
 */
void stop(runtimeInfo *rti);

/**
 * stop and immediatly starts the server again
 */
void restart(cliArgs ca, runtimeInfo *rti);

/**
 * starts the server main thread
 */
void start(runtimeInfo *rti);
