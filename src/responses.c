#include "responses.h"

// TODO: A static string that is a correct xxx simple response.
//
ERRFUNC NotFound404(const InboundHttpMessage *in, OutboundHttpMessage *out) {

	out->status_code = 404;
	return NO_ERROR;
}

ERRFUNC BadRequest400(const InboundHttpMessage *in, OutboundHttpMessage *out) {

	out->status_code = 400;
	auto err = add_header_option(RP_CONTENT_LENGTH, TO_STRINGVIEW("0"), out);
	SNS_RET_IF_ERR(err);
	return NO_ERROR;
}
