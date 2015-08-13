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
#include <stdlib.h>
#include <uv.h>

#include "node.h"
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

void add_vtep_node(char *hostname)
{
	vtep_node_t *vtep_node = malloc(sizeof(vtep_node_t));
	vtep_node->hostname = strdup(hostname);
	vtep_node->send_addr = uv_ip4_addr(vtep_node->hostname, VTEPD_DEFAULT_UDP_PORT);
	listAddNodeTail(server.nodes, (void *)vtep_node);
}

void remove_vtep_node(char *hostname)
{
	vtep_node_t key;
	listNode *node;
	key.hostname = hostname;
	node = listSearchKey(server.nodes, &key);
	if (node)
		listDelNode(server.nodes, node);
}
