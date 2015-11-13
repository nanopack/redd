// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet
/*
 * Copyright (c) 2015 Pagoda Box Inc
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public License, v.
 * 2.0. If a copy of the MPL was not distributed with this file, You can obtain one
 * at http://mozilla.org/MPL/2.0/.
 */

#include <stdlib.h>
#include <msgxchng.h>

#include "api.h"
#include "api/node_remove.h"
#include "helper.h"
#include "log.h"
#include "node.h"

static red_node_t
*parse_data(char *data, int data_len)
{
	red_node_t *node = NULL;
	msgpack_zone *mempool = (msgpack_zone*)malloc(sizeof(msgpack_zone));
	msgpack_object deserialized;

	msgpack_zone_init(mempool, 4096);
	msgpack_unpack(data, data_len, NULL, mempool, &deserialized);

	node = unpack_node(deserialized);

	msgpack_zone_destroy(mempool);
	free(mempool);
	mempool = NULL;
	return node;
}

void
handle_node_remove(api_client_t *client, msgxchng_request_t *req)
{
	red_node_t *node;
	msgxchng_response_t *res;

	node = parse_data(req->data, req->data_len);
	if (validate_node(node)) {
		remove_red_node(node);
		reply_success(client, req);
		save_nodes();
	} else {
		reply_error(client, req, "There was an error validating the node");
	}

	/* cleanup */
	free_node(node);
	clean_msgxchng_request(req);
	free(req);
	req = NULL;
}
