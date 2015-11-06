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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <uv.h>

#include "node.h"
#include "helper.h"
#include "log.h"
#include "util/adlist.h"
#include "redd.h"

void *dup_node(void *ptr)
{
	red_node_t *red_node = malloc(sizeof(red_node_t));
	red_node_t *orig = (red_node_t *)ptr;
	red_node->hostname = strdup(orig->hostname);
	red_node->send_addr = orig->send_addr;
	return (void *)red_node;
}

void free_node(void *ptr)
{
	red_node_t *red_node = (red_node_t *)ptr;
	if (red_node->hostname) {
		free(red_node->hostname);
	}
	free(red_node);
}

int match_node(void *ptr, void *key)
{
	red_node_t *red_node = (red_node_t *)ptr;
	red_node_t *red_key = (red_node_t *)key;
	if (strcmp(red_node->hostname, red_key->hostname) == 0) {
		return 1;
	} else {
		return 0;
	}
}

void
pack_node(msgpack_packer *packer, red_node_t *node)
{
	msgpack_pack_map(packer, 1);
	msgpack_pack_key_value(packer, "node", 4, node->hostname, (int)strlen(node->hostname));
}

void
pack_nodes(msgpack_packer *packer)
{
	listIter *iterator	= listGetIterator(server.nodes, AL_START_HEAD);
	listNode *list_node	= NULL;
	red_node_t *node = NULL;
	msgpack_pack_raw(packer, 5);
	msgpack_pack_raw_body(packer, "nodes", 5);
	msgpack_pack_array(packer, listLength(server.nodes));
	while ((list_node = listNext(iterator)) != NULL) {
		node = (red_node_t *)list_node->value;
		pack_node(packer, node);
	}
	listReleaseIterator(iterator);
}

static void
init_node(red_node_t *node)
{
	node->hostname = NULL;
	memset(&node->send_addr, 0, sizeof(struct sockaddr_in));
}

red_node_t
*unpack_node(msgpack_object object)
{
	if (object.type != MSGPACK_OBJECT_MAP)
		return NULL;

	red_node_t *node = malloc(sizeof(red_node_t));
	init_node(node);

	msgpack_object_kv* p    = object.via.map.ptr;
	msgpack_object_kv* pend = object.via.map.ptr + object.via.map.size;

	for (; p < pend; ++p) {
		if (p->key.type != MSGPACK_OBJECT_RAW || p->val.type != MSGPACK_OBJECT_RAW)
			continue;

		msgpack_object_raw *key = &(p->key.via.raw);
		msgpack_object_raw *val = &(p->val.via.raw);

		if (!strncmp(key->ptr, "node", key->size)) {
			node->hostname = strndup(val->ptr, val->size);
		}
	}

	return node;
}

void
unpack_nodes(msgpack_object object)
{
	if (object.type != MSGPACK_OBJECT_MAP)
		return;
	msgpack_object_kv* p = object.via.map.ptr;
	msgpack_object_kv* const pend = object.via.map.ptr + object.via.map.size;

	for (; p < pend; ++p) {
		if (p->key.type == MSGPACK_OBJECT_RAW && p->val.type == MSGPACK_OBJECT_ARRAY) {
			if (!strncmp(p->key.via.raw.ptr, "nodes", p->key.via.raw.size)) {
				if(p->val.type == MSGPACK_OBJECT_ARRAY) {
					red_node_t *node;

					for (int i = 0; i < p->val.via.array.size; i++) {
						node = unpack_node(p->val.via.array.ptr[i]);
						if (node) {
							add_red_node(node);
						}
					}
				}
			}
		}
	}
}

void
save_nodes()
{
	save_data("nodes", pack_nodes);
}

void
load_nodes()
{
	load_data("nodes", unpack_nodes);
}

int
validate_node(red_node_t *node)
{
	if (node != NULL && validate_host(node->hostname))
		return 1;
	else
		return 0;
}

void
add_red_node(red_node_t *node)
{
	if (listSearchKey(server.nodes, node) == NULL) {
		node->send_addr = uv_ip4_addr(node->hostname, atoi(server.vxlan_port));
		listAddNodeTail(server.nodes, (void *)node);
	} else {
		free_node(node);
	}
}

void
remove_red_node(red_node_t *key)
{
	listNode *node;
	if ((node = listSearchKey(server.nodes, key)) != NULL) {
		listDelNode(server.nodes, node);
	}
}
