#pragma once

#include "tcpConn.h"

#include <openssl/types.h>

typedef struct {
	SSL   *ssl;
	Socket client_socket;
} ResolverData;
