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
 * Copyright 2015 Pagoda Box, Inc.  All rights reserved.
 */

#include <netinet/ip.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <stropts.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "async_io.h"
#include "util/cmd.h"
#include "route.h"
#include "tun.h"
#include "vtepd.h"

static int 
tun_read_each(void* data, async_io_buf_t* elem){
	async_io_t *tun = (async_io_t *)data;
	int retval;
	int flags = 0;
	struct strbuf buf;
	buf.buf = elem->buf;
	buf.maxlen = elem->maxlen;
	buf.len = elem->maxlen;
	// vtep_log(VTEPD_DEBUG, "Device %i is reading into %p which has %i bytes available", tun->fd, elem, elem->maxlen);
	retval = getmsg(tun->read_io.fd, NULL, &buf, &flags);
	if (retval < 0) {
		// we need to find out why errno is 0 sometimes
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		}
		// vtep_log(VTEPD_DEBUG, "Device %i returned %i", tun->fd, retval);
		// vtep_log(VTEPD_WARNING, "FATAL ERROR: getmsg returned (%d) - (%d) %s", retval, errno, strerror(errno));
		return 0;
	}
	elem->len = buf.len;
	// vtep_log(VTEPD_DEBUG, "Device %i has more? %i cntl: %i data: %i", tun->fd, retval,MORECTL,MOREDATA);
	return 1;
}

static void 
tun_read_done(void* data, async_io_buf_t* elem){
	async_io_t *tun = (async_io_t *)data;
	// vtep_log(VTEPD_DEBUG, "Sending message");
	handle_local_frame(elem->buf, elem->len);
}

static void 
tun_read_cb(void* data, int status){
	async_io_t *tun = (async_io_t *)data;
}

static int 
tun_write_each(void* data, async_io_buf_t* elem){
	async_io_t *tun = (async_io_t *)data;
	int retval;
	int flags = 0;
	struct strbuf buf;
	buf.buf = elem->buf;
	buf.maxlen = elem->maxlen;
	buf.len = elem->len;
	// vtep_log(VTEPD_DEBUG, "Device %i is writing into %p which has %i bytes available", tun->fd, elem, elem->len);
	retval = putmsg(tun->write_io.fd, NULL, &buf, 0);
	if (retval < 0) {
		// we need to find out why errno is 0 sometimes
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		}
		// vtep_log(VTEPD_DEBUG, "Device %i returned %i", tun->fd, retval);
		// vtep_log(VTEPD_WARNING, "FATAL ERROR: putmsg (%d) %s", errno, strerror(errno));
		return 0;
	}
	// vtep_log(VTEPD_DEBUG, "Device %i has more? %i cntl: %i data: %i", tun->fd, retval,MORECTL,MOREDATA);
	return 1;
}

static void 
tun_write_done(void* data, async_io_buf_t* elem){
	async_io_t *tun = (async_io_t *)data;
}

static void 
tun_write_cb(void* data, int status){
	async_io_t *tun = (async_io_t *)data;
}

int
tun_init()
{
	int fd;
	int err;
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));

	if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
		return fd;
	}

	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
		close(fd);
		return err;
	}

	server.tun_name = strdup(ifr.ifrname);
	server.tun_fd = fd;

	char *cmd[] = {"ip", "route", "add", "224.0.0.0/4", "dev", server.tun_name, 0};

	run_cmd(cmd);

	return async_io_init(&server.tun_async_io, fd, (void *)&server.tun_async_io,
		2048, 16, tun_read_each, tun_read_done, tun_read_cb,
		2048, 16, tun_write_each, tun_write_done, tun_write_cb);
}