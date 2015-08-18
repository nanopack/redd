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
#include "util/adlist.h"
#include "vtepd.h"

void *dup_node(void *ptr)
{
	vtep_node_t *vtep_node = malloc(sizeof(vtep_node_t));
	vtep_node_t *orig = (vtep_node_t *)ptr;
	vtep_node->hostname = strdup(orig->hostname);
	vtep_node->send_addr = orig->send_addr;
	return (void *)vtep_node;
}

void free_node(void *ptr)
{
	vtep_node_t *vtep_node = (vtep_node_t *)ptr;
	if (vtep_node->hostname) {
		free(vtep_node->hostname);
	}
	free(vtep_node);
}

int match_node(void *ptr, void *key)
{
	vtep_node_t *vtep_node = (vtep_node_t *)ptr;
	vtep_node_t *vtep_key = (vtep_node_t *)key;
	if (strcmp(vtep_node->hostname, vtep_key->hostname) == 0) {
		return 1;
	} else {
		return 0;
	}
}

void
pack_node(msgpack_packer *packer, vtep_node_t *node)
{
	msgpack_pack_map(packer, 1);
	msgpack_pack_key_value(packer, "node", 4, node->hostname, (int)strlen(node->hostname));
}

void
pack_nodes(msgpack_packer *packer)
{
	listIter *iterator	= listGetIterator(server.nodes, AL_START_HEAD);
	listNode *list_node	= NULL;
	vtep_node_t *node = NULL;
	msgpack_pack_raw(packer, 5);
	msgpack_pack_raw_body(packer, "nodes", 5);
	msgpack_pack_array(packer, listLength(server.nodes));
	while ((list_node = listNext(iterator)) != NULL) {
		node = (vtep_node_t *)list_node->value;
		pack_node(packer, node);
	}
	listReleaseIterator(iterator);
}

static void
init_node(vtep_node_t *node)
{
	node->hostname = NULL;
	memset(&node->send_addr, 0, sizeof(struct sockaddr_in));
}

vtep_node_t
*unpack_node(msgpack_object object)
{
	if (object.type != MSGPACK_OBJECT_MAP)
		return NULL;

	vtep_node_t *node = malloc(sizeof(vtep_node_t));
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
save_nodes()
{
	msgpack_sbuffer *buffer = NULL;
	msgpack_packer *packer  = NULL;
	char *save_file_name;
	FILE *save_file;

	if (server.save_path == NULL) {
		return;
	}
	len = strlen(server.save_path);
	if (len == 0) {
		return;
	}
	if (server.save_path[len - 1] == '/') {
		save_file_name = (char *)malloc(len + 6);
		sprintf(save_file_name, "%s%s", server.save_path, "nodes");
	} else {
		save_file_name = (char *)malloc(len + 7);
		sprintf(save_file_name, "%s/%s", server.save_path, "nodes");
	}

	buffer = msgpack_sbuffer_new();
	msgpack_sbuffer_init(buffer);

	packer = msgpack_packer_new((void *)buffer, msgpack_sbuffer_write);

	msgpack_pack_map(packer, 1);
	pack_nodes(packer);

	if ((save_file = fopen(save_file_name, "w")) != NULL) {
		size_t written;
		written = fwrite((void *)&buffer->data[0], 1, buffer->size, save_file);
		fclose(save_file);
		if (written != buffer->size) {
			vtep_log(VTEPD_WARNING, "Failed to write to %s", save_file_name);
		}
	} else {
		vtep_log(VTEPD_WARNING, "Failed to open %s", save_file_name);
	}

	free(save_file_name);
	save_file_name = NULL;
	msgpack_packer_free(packer);
	packer = NULL;
	msgpack_sbuffer_free(buffer);
	buffer = NULL;
}

void
load_nodes()
{
	char *save_file_name;
	char *save_data;
	FILE *save_file;
	int len;
	struct stat file_stat;
	msgpack_zone *mempool;
	msgpack_object deserialized;
	if (server.save_path == NULL) {
		return;
	}
	len = strlen(server.save_path);
	if (len == 0) {
		return;
	}
	if (server.save_path[len - 1] == '/') {
		save_file_name = (char *)malloc(len + 6);
		sprintf(save_file_name, "%s%s", server.save_path, "nodes");
	} else {
		save_file_name = (char *)malloc(len + 7);
		sprintf(save_file_name, "%s/%s", server.save_path, "nodes");
	}

	if (stat(save_file_name, &file_stat) != 0) {
		free(save_file_name);
		return;
	}
	if (!S_ISREG(file_stat.st_mode)) {
		free(save_file_name);
		return;
	}
	len = file_stat.st_size;
	if ((save_file = fopen(save_file_name, "r")) != NULL) {
		size_t bytes_read;
		save_data = (char *)malloc(len);
		bytes_read = fread((void *)save_data, 1, len, save_file);
		fclose(save_file);
		if (bytes_read != len) {
			vtep_log(VTEPD_WARNING, "Failed to read data from %s", save_file_name);
			free(save_file_name);
			free(save_data);
			return;
		}
	} else {
		vtep_log(VTEPD_WARNING, "Failed to open %s", save_file_name);
		free(save_file_name);
		return;
	}
	mempool = (msgpack_zone *)malloc(sizeof(msgpack_zone));
	msgpack_zone_init(mempool, 4096);
	msgpack_unpack(save_data, len, NULL, mempool, &deserialized);

	if (deserialized.type == MSGPACK_OBJECT_MAP) {
		msgpack_object_kv* p = deserialized.via.map.ptr;
		msgpack_object_kv* const pend = deserialized.via.map.ptr + deserialized.via.map.size;

		for (; p < pend; ++p) {
			if (p->key.type == MSGPACK_OBJECT_RAW && p->val.type == MSGPACK_OBJECT_ARRAY) {
				if (!strncmp(p->key.via.raw.ptr, "nodes", p->key.via.raw.size)) {
					if(p->val.type == MSGPACK_OBJECT_ARRAY) {
						vtep_node_t *node;

						for (int i = 0; i < p->val.via.array.size; i++) {
							node = unpack_node(p->val.via.array.ptr[i]);
							if (node) {
								add_vtep_node(node);
							}
						}
					}
				}
			}
		}
	}
	free(save_file_name);
	save_file_name = NULL;
	free(save_data);
	save_data = NULL;
	msgpack_zone_destroy(mempool);
	free(mempool);
	mempool = NULL;
}

int
validate_node(vtep_node_t *node)
{
	if (node != NULL && validate_host(node->hostname))
		return 1;
	else
		return 0;
}

void
add_vtep_node(vtep_node_t *node)
{
	if (listSearchKey(server.nodes, node) == NULL) {
		node->send_addr = uv_ip4_addr(node->hostname, atoi(server.vxlan_port));
		listAddNodeTail(server.nodes, (void *)node);
	} else {
		free_node(node);
	}
}

void
remove_vtep_node(vtep_node_t *key)
{
	listNode *node;
	if ((node = listSearchKey(server.nodes, key)) != NULL) {
		listDelNode(server.nodes, node);
	}
}
