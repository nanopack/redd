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

#include "ip.h"
#include "helper.h"
#include "log.h"
#include "util/adlist.h"
#include "util/cmd.h"
#include "vtepd.h"

void *dup_ip(void *ptr)
{
	vtep_ip_t *vtep_ip = malloc(sizeof(vtep_ip_t));
	vtep_ip_t *orig = (vtep_ip_t *)ptr;
	vtep_ip->ip_address = strdup(orig->ip_address);
	return (void *)vtep_ip;
}

void free_ip(void *ptr)
{
	vtep_ip_t *vtep_ip = (vtep_ip_t *)ptr;
	if (vtep_ip->ip_address) {
		free(vtep_ip->ip_address);
	}
	free(vtep_ip);
}

int match_ip(void *ptr, void *key)
{
	vtep_ip_t *vtep_ip = (vtep_ip_t *)ptr;
	vtep_ip_t *vtep_key = (vtep_ip_t *)key;
	if (strcmp(vtep_ip->ip_address, vtep_key->ip_address) == 0) {
		return 1;
	} else {
		return 0;
	}
}

void
pack_ip(msgpack_packer *packer, vtep_ip_t *ip)
{
	msgpack_pack_map(packer, 1);
	msgpack_pack_key_value(packer, "ip_address", 10, ip->ip_address, (int)strlen(ip->ip_address));
}

void
pack_ips(msgpack_packer *packer)
{
	listIter *iterator	= listGetIterator(server.ips, AL_START_HEAD);
	listNode *list_node	= NULL;
	vtep_ip_t *ip = NULL;
	msgpack_pack_raw(packer, 12);
	msgpack_pack_raw_body(packer, "ip_addresses", 12);
	msgpack_pack_array(packer, listLength(server.ips));
	while ((list_node = listNext(iterator)) != NULL) {
		ip = (vtep_ip_t *)list_node->value;
		pack_ip(packer, ip);
	}
	listReleaseIterator(iterator);
}

static void
init_ip(vtep_ip_t *ip)
{
	ip->ip_address = NULL;
}

vtep_ip_t
*unpack_ip(msgpack_object object)
{
	if (object.type != MSGPACK_OBJECT_MAP)
		return NULL;

	vtep_ip_t *ip = malloc(sizeof(vtep_ip_t));
	init_ip(ip);

	msgpack_object_kv* p    = object.via.map.ptr;
	msgpack_object_kv* pend = object.via.map.ptr + object.via.map.size;

	for (; p < pend; ++p) {
		if (p->key.type != MSGPACK_OBJECT_RAW || p->val.type != MSGPACK_OBJECT_RAW)
			continue;

		msgpack_object_raw *key = &(p->key.via.raw);
		msgpack_object_raw *val = &(p->val.via.raw);

		if (!strncmp(key->ptr, "ip_address", key->size)) {
			ip->ip_address = strndup(val->ptr, val->size);
		}
	}

	return ip;
}

void
save_ips()
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
		save_file_name = (char *)malloc(len + 4);
		sprintf(save_file_name, "%s%s", server.save_path, "ips");
	} else {
		save_file_name = (char *)malloc(len + 5);
		sprintf(save_file_name, "%s/%s", server.save_path, "ips");
	}

	buffer = msgpack_sbuffer_new();
	msgpack_sbuffer_init(buffer);

	packer = msgpack_packer_new((void *)buffer, msgpack_sbuffer_write);

	msgpack_pack_map(packer, 1);
	pack_ips(packer);

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
load_ips()
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
		save_file_name = (char *)malloc(len + 4);
		sprintf(save_file_name, "%s%s", server.save_path, "ips");
	} else {
		save_file_name = (char *)malloc(len + 5);
		sprintf(save_file_name, "%s/%s", server.save_path, "ips");
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
				if (!strncmp(p->key.via.raw.ptr, "ip_addresses", p->key.via.raw.size)) {
					if(p->val.type == MSGPACK_OBJECT_ARRAY) {
						vtep_ip_t *ip;

						for (int i = 0; i < p->val.via.array.size; i++) {
							ip = unpack_ip(p->val.via.array.ptr[i]);
							if (ip) {
								add_vtep_ip(ip);
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

static int
vxlan_has_ip(char *ip_address)
{
	int ret = 0;
	char *cmd = malloc(128);
	snprintf(cmd, 128, "ip addr show dev %s | grep %s", server.vxlan_name, ip_address);
	char *bash_cmd[] = {"bash","-c", cmd, 0};
	ret = run_cmd(bash_cmd);
	free(cmd);
	return !ret;
}

int
validate_ip(vtep_ip_t *ip)
{
	if (ip != NULL && validate_ip_address(ip->ip_address))
		return 1;
	else
		return 0;
}

int
add_vtep_ip(vtep_ip_t *ip)
{
	int ret = 0;
	listNode *node;
	if ((node = listSearchKey(server.ips, ip)) == NULL) {
		if (!vxlan_has_ip(ip->ip_address)) {
			char *cmd[] = {"ip", "addr", "add", ip->ip_address, "dev", server.vxlan_name, 0};
			ret = run_cmd(cmd);
		}
		if (ret == 0) {
			listAddNodeTail(server.ips, (void *)ip);
		} else {
			vtep_log(VTEPD_WARNING, "unable to add ip");
			free_ip(ip);
		}
	} else {
		free_ip(ip);
	}
	return ret;
}

int
remove_vtep_ip(vtep_ip_t *key)
{
	int ret = 0;
	listNode *node;
	if ((node = listSearchKey(server.ips, key)) != NULL) {
		listDelNode(server.ips, node);
		if (vxlan_has_ip(key->ip_address)) {
			char *cmd[] = {"ip", "addr", "del", key->ip_address, "dev", server.vxlan_name, 0};
			ret = run_cmd(cmd);
		}
	}
	return ret;
}
