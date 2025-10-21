#pragma once

#include "StringRef.h"
#include "server.h"

typedef struct {
	unsigned short tcp_port;
	StringRef      base_dir;
} CliArgs;

/**
 * given the cli arguments set's the server options accordingly
 *
 * @param argc the count of arguments
 * @param argv the values of the arguments
 */
CliArgs parse_args(const int argc, const char *argv[]);

/**
 * Setup the server, loads libraries and stuff
 *
 * to call one
 */
void setup(CliArgs args, RuntimeInfo *res);

/**
 * stop the server and its threads
 */
void stop(RuntimeInfo *rti);

/**
 * stop and immediatly starts the server again
 */
void restart(CliArgs ca, RuntimeInfo *rti);

/**
 * starts the server main thread
 */
void start(RuntimeInfo *rti);
