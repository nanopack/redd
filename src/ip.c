// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet
/*
 * Copyright (c) 2015 Pagoda Box Inc
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public License, v.
 * 2.0. If a copy of the MPL was not distributed with this file, You can obtain one
 * at http://mozilla.org/MPL/2.0/.
 */

#include <string.h>
#include <stdlib.h>
#include <uv.h>

#include "ip.h"
#include "helper.h"
#include "log.h"
#include "util/adlist.h"
#include "util/cmd.h"
#include "redd.h"

void *dup_ip(void *ptr)
{
	red_ip_t *red_ip = malloc(sizeof(red_ip_t));
	red_ip_t *orig = (red_ip_t *)ptr;
	red_ip->ip_address = strdup(orig->ip_address);
	return (void *)red_ip;
}

void free_ip(void *ptr)
{
	red_ip_t *red_ip = (red_ip_t *)ptr;
	if (red_ip->ip_address) {
		free(red_ip->ip_address);
	}
	free(red_ip);
}

int match_ip(void *ptr, void *key)
{
	red_ip_t *red_ip = (red_ip_t *)ptr;
	red_ip_t *red_key = (red_ip_t *)key;
	if (strcmp(red_ip->ip_address, red_key->ip_address) == 0) {
		return 1;
	} else {
		return 0;
	}
}

void
pack_ip(msgpack_packer *packer, red_ip_t *ip)
{
	msgpack_pack_map(packer, 1);
	msgpack_pack_key_value(packer, "ip_address", 10, ip->ip_address, (int)strlen(ip->ip_address));
}

void
pack_ips(msgpack_packer *packer)
{
	listIter *iterator	= listGetIterator(server.ips, AL_START_HEAD);
	listNode *list_node	= NULL;
	red_ip_t *ip = NULL;
	msgpack_pack_raw(packer, 12);
	msgpack_pack_raw_body(packer, "ip_addresses", 12);
	msgpack_pack_array(packer, listLength(server.ips));
	while ((list_node = listNext(iterator)) != NULL) {
		ip = (red_ip_t *)list_node->value;
		pack_ip(packer, ip);
	}
	listReleaseIterator(iterator);
}

static void
init_ip(red_ip_t *ip)
{
	ip->ip_address = NULL;
}

red_ip_t
*unpack_ip(msgpack_object object)
{
	if (object.type != MSGPACK_OBJECT_MAP)
		return NULL;

	red_ip_t *ip = malloc(sizeof(red_ip_t));
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
unpack_ips(msgpack_object object)
{
	if (object.type != MSGPACK_OBJECT_MAP)
		return;
	msgpack_object_kv* p = object.via.map.ptr;
	msgpack_object_kv* const pend = object.via.map.ptr + object.via.map.size;

	for (; p < pend; ++p) {
		if (p->key.type == MSGPACK_OBJECT_RAW && p->val.type == MSGPACK_OBJECT_ARRAY) {
			if (!strncmp(p->key.via.raw.ptr, "ip_addresses", p->key.via.raw.size)) {
				if(p->val.type == MSGPACK_OBJECT_ARRAY) {
					red_ip_t *ip;

					for (int i = 0; i < p->val.via.array.size; i++) {
						ip = unpack_ip(p->val.via.array.ptr[i]);
						if (ip) {
							add_red_ip(ip);
						}
					}
				}
			}
		}
	}
}

void
save_ips()
{
	save_data("ips", pack_ips);
}

void
load_ips()
{
	load_data("ips", unpack_ips);
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
validate_ip(red_ip_t *ip)
{
	if (ip != NULL && validate_ip_address(ip->ip_address))
		return 1;
	else
		return 0;
}

int
add_red_ip(red_ip_t *ip)
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
			red_log(REDD_WARNING, "unable to add ip");
			free_ip(ip);
		}
	} else {
		free_ip(ip);
	}
	return ret;
}

int
remove_red_ip(red_ip_t *key)
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
