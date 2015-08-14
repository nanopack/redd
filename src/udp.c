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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

#include "async_io.h"
#include "udp.h"
#include "vtepd.h"

static int
udp_set_nonblock(int fd)
{
	int	val;

	val = fcntl(fd, F_GETFL, 0);
	if (val < 0) {
		vtep_log(VTEPD_WARNING, "fcntl(%d, F_GETFL, 0): %s", fd, strerror(errno));
	}
	if (val & O_NONBLOCK) {
		vtep_log(VTEPD_DEBUG, "fd %d is O_NONBLOCK", fd);
	}
	val |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, val) == -1) {
		vtep_log(VTEPD_WARNING, "fcntl(%d, F_SETFL, O_NONBLOCK): %s", fd, strerror(errno));
	}

	return (val);
}

static int 
udp_read_each(void* data, async_io_buf_t* elem)
{
	async_io_t *async_io = (async_io_t *)data;
	int retval;
	struct sockaddr_in remaddr;
	socklen_t addrlen = sizeof(remaddr);
	// vtep_log(VTEPD_DEBUG, "UDP %i is reading into %p which has %i bytes available", async_io->async_io.fd, elem, elem->maxlen);
	retval = recvfrom(async_io->read_io.fd, elem->buf, elem->maxlen, 0, (struct sockaddr *)&remaddr, &addrlen);
	if (retval < 0) {
		// we need to find out why errno is 0 sometimes
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		}
		// vtep_log(VTEPD_DEBUG, "UDP %i returned %i", async_io->fd, retval);
		// vtep_log(VTEPD_WARNING, "FATAL ERROR: getmsg returned (%d) - (%d) %s", retval, errno, strerror(errno));
		return 0;
	}
	elem->len = retval;
	// vtep_log(VTEPD_DEBUG, "UDP %i has more? %i cntl: %i data: %i", async_io->fd, retval,MORECTL,MOREDATA);
	return 1;
}

static void 
udp_read_done(void* data, async_io_buf_t* elem)
{
	async_io_t *async_io = (async_io_t *)data;
	server.udp_recv_count++;
	server.udp_recv_amount += elem->len;
}

static void 
udp_read_cb(void* data, int status)
{
	async_io_t *async_io = (async_io_t *)data;
}

static int 
udp_write_each(void* data, async_io_buf_t* elem)
{
	async_io_t *async_io = (async_io_t *)data;
	int retval;
	retval = sendto(async_io->write_io.fd, elem->buf, elem->len, 0, (struct sockaddr *)&elem->addr, sizeof(elem->addr));
	if (retval < 0)
		return 0;
	return 1;
}

static void 
udp_write_done(void* data, async_io_buf_t* elem)
{
	server.udp_send_count++;
	server.udp_send_amount += elem->len;
}

static void 
udp_write_cb(void* data, int status)
{
	async_io_t *async_io = (async_io_t *)data;
	// async_io->write_io.working = false;
}

void
udp_init()
{
	int fd;
	struct sockaddr_in udp_addr;
	vtep_log(VTEPD_DEBUG, "Initing udp");
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(!fd){
		vtep_log(VTEPD_DEBUG, "UDP open socket failed to open");
		exit(1);
	}
	vtep_log(VTEPD_DEBUG, "UDP fd: %d", fd);
	udp_set_nonblock(fd);
	udp_addr = uv_ip4_addr(server.udp_listen_address, 0);
	if (bind(fd, (struct sockaddr *)&udp_addr, sizeof(struct sockaddr))==-1){
	    vtep_log(VTEPD_DEBUG, "UDP bind socket failed to open");
	    exit(1);
	}
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &server.udp_send_buf, sizeof(int));
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &server.udp_recv_buf, sizeof(int));

	async_io_init(&server.udp_async_io, fd, (void *)&server.udp_async_io,
		2048, 16, udp_read_each, udp_read_done, udp_read_cb,
		2048, 256, udp_write_each, udp_write_done, udp_write_cb);
}