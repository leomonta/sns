#pragma once

#include "HttpMessage.h"
#include "StringRef.h"

typedef void (*MessageProcessor)(const HTTP_Method *method, const InboundHttpMessage *in_message, OutboundHttpMessage *out_message);

void register_rounte(StringRef *route_name, MessageProcessor *fun);
