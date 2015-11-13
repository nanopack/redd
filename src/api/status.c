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
#include "api/status.h"
#include "helper.h"
#include "ip.h"
#include "log.h"
#include "node.h"
#include "redd.h"

static char
*generate_data(int *size)
{
	msgpack_sbuffer *buffer = NULL;
	msgpack_packer *packer  = NULL;
	char *return_char;

	buffer = msgpack_sbuffer_new();
	msgpack_sbuffer_init(buffer);

	packer = msgpack_packer_new((void *)buffer, msgpack_sbuffer_write);
	msgpack_pack_map(packer, 8);
	pack_nodes(packer);
	pack_ips(packer);
	msgpack_pack_key_value(packer, "tun_dev", 7, server.tun_name, strlen(server.tun_name));
	msgpack_pack_key_value(packer, "vxlan_dev", 9, server.vxlan_name, strlen(server.vxlan_name));
	msgpack_pack_key_value(packer, "vxlan_vni", 9, server.vxlan_vni, strlen(server.vxlan_vni));
	msgpack_pack_key_value(packer, "vxlan_group", 11, server.vxlan_group, strlen(server.vxlan_group));
	msgpack_pack_key_value(packer, "vxlan_port", 10, server.vxlan_port, strlen(server.vxlan_port));
	msgpack_pack_key_value(packer, "vxlan_interface", 15, server.vxlan_interface, strlen(server.vxlan_interface));

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
handle_status(api_client_t *client, msgxchng_request_t *req)
{
	msgxchng_response_t *res = node_generate_response(req);
	red_log(REDD_DEBUG, "request %s: status", req->id);

	reply(client, res);

	/* cleanup */
	clean_msgxchng_request(req);
	free(req);
	req = NULL;
}
