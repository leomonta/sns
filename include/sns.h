#pragma once

#include "MiniMap_uint32_t_MessageProcessor.h"

typedef struct {
	MiniMap_uint32_t_MessageProcessor mapped_function;
} SNSServer;

/**
 * asks the `SNSserver` to call `fun` when any request on `route_name` is made
 *
 * @param[in] `rounte_name` the plain name of the route to intercept, 
 * 	nullptr for any route that is not already matched by another mapping
 * @param[in] `fun` the function to be called to manage the http reqeust,
 * 	nullptr to remove the mapping
 * @oaram[in] `SNSServer` the server where to apply the mapping
 *
 */
void register_rounte(const StringRef *route_name, const MessageProcessor *fun, SNSServer *server);
