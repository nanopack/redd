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

// helper functions

#include <ctype.h>
#include <msgpack.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "helper.h"

void 
msgpack_pack_key_value(msgpack_packer *packer, char *key, int key_len, char *value, int value_len)
{
	msgpack_pack_raw(packer, key_len);
	msgpack_pack_raw_body(packer, key, key_len);
	msgpack_pack_raw(packer, value_len);
	msgpack_pack_raw_body(packer, value, value_len);
}

char
*parse_ip_address(char *ip_address_string, int len)
{
	char *ip_address = strndup(ip_address_string, len);
	return ip_address;
}

char
*parse_host(char *host_string, int len)
{
	char *host = strndup(host_string, len);
	return host;
}

int
validate_ip_address(char *ip_address)
{
	if (host != NULL)
		return 1;
	else
		return 0;
}

int
validate_host(char *host)
{
	// add check to see if host lookup works?
	if (host != NULL)
		return 1;
	else
		return 0;
}

char
*blank_ip_address()
{
	return NULL;
}

char
*blank_host()
{
	return NULL;
}

char 
*pack_ip_address(char *ip_address)
{
	char *packed;
	packed = strdup(ip_address);
	return packed;
}

char 
*pack_host(char *host)
{
	char *packed;
	packed = strdup(host);
	return packed;
}

void
ngx_empty_into(ngx_queue_t *from,ngx_queue_t *to,int limit)
{
	ngx_queue_t *q;
	while (!ngx_queue_empty(from) && limit-- != 0) {
		q = ngx_queue_last(from);
		ngx_queue_remove(q);
		ngx_queue_insert_head(to,q);
	}
}