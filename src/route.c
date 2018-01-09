// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet
/*
 * Copyright (c) 2015 Pagoda Box Inc
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public License, v.
 * 2.0. If a copy of the MPL was not distributed with this file, You can obtain one
 * at http://mozilla.org/MPL/2.0/.
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
#include "redd.h"
#include "vxlan.h"

static void
send_to_red(red_node_t *red, char *frame, int len)
{
	async_io_buf_t *buf = async_io_write_buf_get(&server.udp_async_io);
	if (buf == NULL)
		return;
	if (len > buf->maxlen) {
		red_log(REDD_DEBUG, "packet is larger than buffer");
		return;
	}

	buf->addr = red->send_addr;
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
		send_to_red((red_node_t *)node->value, frame, len);
	}
	listReleaseIterator(iter);
}

void 
handle_local_frame(char *frame, int len)
{
	struct iphdr *ip_header = (struct iphdr *)frame;
	// only route udp packets - protocol 17
	if (ip_header->protocol == 17) {
		struct udphdr *udp_header = (struct udphdr *)(frame + (ip_header->ihl * 4));
		if ((ip_header->ihl * 4) + ntohs(udp_header->len) == len) {
			int data_len = len - (ip_header->ihl * 4) - sizeof(struct udphdr);
			// don't send packet if data_len is less than 0
			if (data_len >= 0) {
				do_broadcast(frame + (ip_header->ihl * 4) + sizeof(struct udphdr), data_len);
			}
		} else {
			red_log(REDD_DEBUG, "lengths didn't add up");
		}
	} else {
		red_log(REDD_DEBUG, "protocol: %d", ip_header->protocol);
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
	red_log(REDD_DEBUG, "Shutting down routing");
	shutdown_tun();
	shutdown_udp();
	shutdown_vxlan();
}
