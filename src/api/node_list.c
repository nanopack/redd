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
#include "api/node_list.h"
#include "helper.h"
#include "log.h"
#include "node.h"
#include "vtepd.h"
#include "util/adlist.h"

static char
*generate_data(int *size)
{
	msgpack_sbuffer *buffer = NULL;
	msgpack_packer *packer  = NULL;
	char *return_char;

	buffer = msgpack_sbuffer_new();
	msgpack_sbuffer_init(buffer);

	packer = msgpack_packer_new((void *)buffer, msgpack_sbuffer_write);

	pack_nodes(packer);

	return_char = (char *)malloc(buffer->size + 1);
	memcpy(return_char, &buffer->data[0], buffer->size);
	return_char[buffer->size] = '\0';
	*size = buffer->size;

	msgpack_packer_free(packer);
	packer = NULL;
	msgpack_sbuffer_free(buffer);
	buffer = NULL;

	return return_char;
}

static msgxchng_response_t
*node_generate_response(msgxchng_request_t *req)
{
	msgxchng_response_t *res;
	int size;

	char *data = generate_data(&size);
	res = new_msgxchng_response(req->id, req->id_len, data, size, "complete", 8);

	free(data);
	data = NULL;

	return res;
}

void
handle_node_list(api_client_t *client, msgxchng_request_t *req)
{
	msgxchng_response_t *res;

	res = node_generate_response(req);

	reply(client, res);

	/* cleanup */
	clean_msgxchng_request(req);
	free(req);
	req = NULL;
}
