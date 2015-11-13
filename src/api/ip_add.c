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
#include "api/ip_add.h"
#include "helper.h"
#include "ip.h"
#include "log.h"
#include "vxlan.h"

static red_ip_t
*parse_data(char *data, int data_len)
{
	red_ip_t *ip = NULL;
	msgpack_zone *mempool = (msgpack_zone*)malloc(sizeof(msgpack_zone));
	msgpack_object deserialized;

	msgpack_zone_init(mempool, 4096);
	msgpack_unpack(data, data_len, NULL, mempool, &deserialized);

	ip = unpack_ip(deserialized);

	msgpack_zone_destroy(mempool);
	free(mempool);
	mempool = NULL;
	return ip;
}

void
handle_ip_add(api_client_t *client, msgxchng_request_t *req)
{
	red_ip_t *ip;
	msgxchng_response_t *res;

	ip = parse_data(req->data, req->data_len);
	if (validate_ip(ip)) {
		if (add_red_ip(ip) == 0) {
			reply_success(client, req);
			save_ips();
		} else {
			reply_error(client, req, "There was an error trying to add the ip address");
		}
	} else {
		free_ip(ip);
		reply_error(client, req, "There was an error validating the ip address");
	}

	/* cleanup */
	clean_msgxchng_request(req);
	free(req);
	req = NULL;
}
