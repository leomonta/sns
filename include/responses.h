#pragma once

#include "HttpMessage.h"
#include "errcode.h"

ERRFUNC NotFound404(const InboundHttpMessage *in, OutboundHttpMessage *out);

ERRFUNC BadRequest400(const InboundHttpMessage *in, OutboundHttpMessage *out);

ERRFUNC InternalServerEroor500(const InboundHttpMessage *in, OutboundHttpMessage *out);
