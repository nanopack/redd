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
#include "api/ip_list.h"
#include "helper.h"
#include "log.h"
#include "vxlan.h"

static char
*generate_data(int *size)
{
	msgpack_sbuffer *buffer = NULL;
	msgpack_packer *packer  = NULL;
	listIter *iterator      = listGetIterator(server.vxlan_ips, AL_START_HEAD);
	listNode *list_node     = NULL;
	char *return_char;
	char *ip_address_buffer;

	buffer = msgpack_sbuffer_new();
	msgpack_sbuffer_init(buffer);

	packer = msgpack_packer_new((void *)buffer, msgpack_sbuffer_write);
	msgpack_pack_map(packer, 1);
	msgpack_pack_raw(packer, 12);
	msgpack_pack_raw_body(packer, "ip_addresses", 12);
	msgpack_pack_array(packer, listLength(server.vxlan_ips));

	while ((list_node = listNext(iterator)) != NULL) {
		char *current	= (char *)list_node->value;
		ip_address_buffer		= pack_ip_address(current);
		msgpack_pack_map(packer, 1);
		msgpack_pack_key_value(packer, "ip_address", 10, ip_address_buffer, strlen(ip_address_buffer));
		free(ip_address_buffer);
		ip_address_buffer = NULL;
	}

	return_char = (char *)malloc(buffer->size + 1);
	memcpy(return_char, &buffer->data[0], buffer->size);
	return_char[buffer->size] = '\0';
	*size = buffer->size;

	msgpack_packer_free(packer);
	packer = NULL;
	msgpack_sbuffer_free(buffer);
	buffer = NULL;
	listReleaseIterator(iterator);

	return return_char;
}

static msgxchng_response_t
*device_list_generate_response(msgxchng_request_t *req)
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
handle_ip_list(api_client_t *client, msgxchng_request_t *req)
{
	msgxchng_response_t *res;

	res = device_list_generate_response(req);

	reply(client, res);

	/* cleanup */
	clean_msgxchng_request(req);
	free(req);
	req = NULL;
}