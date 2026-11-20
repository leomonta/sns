#pragma once

#include "MiniVector_MessageProcessor.h"
#include "MiniVector_StringView.h"
#include "MiniVector_regex_t.h"
#include "tcpConn.h"

#include <openssl/types.h>

typedef struct {
	// I need to store the message processors and the paths associated,
	// but also the compiled regex if they are regex route
	// but since I can't compare `regex_t` I also need to store the original string that it's based on (this is needed for deletion)
	// so I store all three and they are connected by the index. i.e. route at index 5 is (potentially) compiled to the regex at index 5 and the processors that should be used for it is at index 5
	MiniVector_StringView       names;
	MiniVector_regex_t          regexes;
	MiniVector_MessageProcessor processors;
	MessageProcessor            default_processor; // the message processor to use if none of other matches
} SNSRoutes;

typedef struct {
	SSL       *ssl;
	const Socket     client_socket;
	const SNSRoutes *routes_info;
} ResolverData;
