#pragma once

#include "MiniMap_StringRef_MessageProcessor.h"
#include "tcpConn.h"

#include <openssl/types.h>

typedef struct {
	SSL   *ssl;
	Socket client_socket;
	MiniMap_StringRef_MessageProcessor *mapped_function;
} ResolverData;
