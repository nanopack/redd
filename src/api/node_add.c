// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2013 Pagoda Box, Inc.  All rights reserved.
 */

#include <stdlib.h>
#include <msgxchng.h>

#include "api.h"
#include "api/node_add.h"
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
handle_node_add(api_client_t *client, msgxchng_request_t *req)
{
	red_node_t *node;
	msgxchng_response_t *res;

	node = parse_data(req->data, req->data_len);
	if (validate_node(node)) {
		add_red_node(node);
		reply_success(client, req);
		save_nodes();
	} else {
		free_node(node);
		reply_error(client, req, "There was an error validating the node");
	}

	/* cleanup */
	clean_msgxchng_request(req);
	free(req);
	req = NULL;
}
