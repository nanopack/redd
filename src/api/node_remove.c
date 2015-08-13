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
#include "api/node_remove.h"
#include "helper.h"
#include "log.h"
#include "node.h"

static char
*parse_data(char *data, int data_len)
{
	char *host = blank_host();
	msgpack_zone *mempool = (msgpack_zone*)malloc(sizeof(msgpack_zone));
	msgpack_object deserialized;

	msgpack_zone_init(mempool, 4096);
	msgpack_unpack(data, data_len, NULL, mempool, &deserialized);

	if (deserialized.type == MSGPACK_OBJECT_MAP) {
		msgpack_object_kv* p = deserialized.via.map.ptr;
		msgpack_object_kv* const pend = deserialized.via.map.ptr + deserialized.via.map.size;

		for (; p < pend; ++p) {
			if (p->key.type == MSGPACK_OBJECT_RAW && p->val.type == MSGPACK_OBJECT_RAW) {
				if (!strncmp(p->key.via.raw.ptr, "node", p->key.via.raw.size))
					host = parse_host((char *)p->val.via.raw.ptr, p->val.via.raw.size);
			}
		}
	}

	msgpack_zone_destroy(mempool);
	free(mempool);
	mempool = NULL;
	return host;
}

void
handle_node_remove(api_client_t *client, msgxchng_request_t *req)
{
	char *host;
	msgxchng_response_t *res;

	host = parse_data(req->data, req->data_len);
	if (validate_host(host)) {
		remove_vtep_node(host);
		reply_success(client, req);
	} else {
		reply_error(client, req, "There was an error validating the node");
	}

	/* cleanup */
	clean_msgxchng_request(req);
	free(req);
	req = NULL;
}