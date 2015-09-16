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

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <fcntl.h>

#include "api.h"
#include "async_io.h"
#include "helper.h"
#include "log.h"
#include "node.h"
#include "route.h"
#include "tun.h"
#include "udp.h"
#include "util/adlist.h"
#include "vtepd.h"
#include "vxlan.h"

static void
send_to_vtep(vtep_node_t *vtep, char *frame, int len)
{
	async_io_buf_t *buf = async_io_write_buf_get(&server.udp_async_io);
	if (buf == NULL)
		return;
	if (len > buf->maxlen) {
		vtep_log(VTEPD_DEBUG, "packet is larger than buffer");
		return;
	}

	buf->addr = vtep->send_addr;
	memcpy(buf->buf, frame, len);
	buf->len = len;
	ngx_queue_insert_tail(&server.udp_async_io.write_io.ready_queue,&buf->queue);
	async_io_poll_start(&server.udp_async_io);
}

static void 
do_broadcast(char *frame, int len)
{
	listIter *iter = listGetIterator(server.nodes, AL_START_HEAD);
	listNode *node;
	while ((node = listNext(iter)) != NULL) {
		send_to_vtep((vtep_node_t *)node->value, frame, len);
	}
	listReleaseIterator(iter);
}

void 
handle_local_frame(char *frame, int len)
{
	struct iphdr *ip_header = (struct iphdr *)frame;
	struct udphdr *udp_header = (struct udphdr *)(frame + (ip_header->ihl * 4));
	if ((ip_header->ihl * 4) + ntohs(udp_header->len) == len) {
		do_broadcast(frame + (ip_header->ihl * 4) + sizeof(struct udphdr), len - (ip_header->ihl * 4) - sizeof(struct udphdr));
	} else {
		printf("len: %d, ip_header: %d, ip_header->len: %d, udp_header: %d\n", len, ip_header->ihl, ntohs(ip_header->tot_len), ntohs(udp_header->len));
		int i;
		for (i = 0; i < len; i++) {
			printf("%02x", frame[i]);
		}
		printf("\n");
		for (i = 0; i < len; i++) {
			printf("%c", frame[i]);
		}
		printf("\n");
		vtep_log(VTEPD_DEBUG, "lengths didn't add up");
	}
}

void 
init_routing()
{
	init_tun();
	init_udp();
	init_vxlan();
}

void
shutdown_routing()
{
	vtep_log(VTEPD_DEBUG, "Shutting down routing");
	shutdown_tun();
	shutdown_udp();
	shutdown_vxlan();
}
