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

#ifndef VTEP_H
#define VTEP_H

#include <uv.h>

#include "async_io.h"
#include "util/dict.h"
#include "util/adlist.h"
#include "route.h"

// todo: move this to autobuild
#define VTEP_VERSION 	"0.0.1"

/* Error codes */
#define VTEP_OK		0
#define VTEP_ERR	-1

/* Limits */
#define VTEP_CONFIGLINE_MAX		1024
#define VTEP_MAX_LOGMSG_LEN		1024	/* Default maximum length of syslog messages */
#define VTEP_BINDADDR_MAX		16

/* Sensible defaults */
#define VTEP_DEFAULT_PID_FILE			 "/var/run/vtep.pid"
#define VTEP_DEFAULT_DAEMONIZE			 0
#define VTEP_DEFAULT_LOGFILE			 ""
#define VTEP_DEFAULT_SYSLOG_IDENT		 "vtep"
#define VTEP_DEFAULT_SERVERPORT			 4470	/* TCP port */
#define VTEP_DEFAULT_MAXIDLETIME		 0	/* default client timeout: infinite */
#define VTEP_DEFAULT_MAX_CLIENTS		 10000
#define VTEP_DEFAULT_TCP_KEEPALIVE		 0
#define VTEP_DEFAULT_SYSLOG_ENABLED		 0
#define VTEP_DEFAULT_TOWER_ADDR			 "127.0.0.1"
#define VTEP_DEFAULT_TOWER_PORT			 5610
#define VTEP_DEFAULT_SAVE_PATH 			 "/var/db/vtep/"
#define VTEP_DEFAULT_CONNECT_RETRY_DELAY 60000
#define VTEP_DEFAULT_ROUTING_ENABLED     1
#define VTEP_DEFAULT_UDP_PORT            4789
#define VTEP_DEFAULT_DEVICE_CHUNKSIZE    65536
#define VTEP_DEFAULT_DEVICE_BUF_SECONDS  0
#define VTEP_DEFAULT_DEVICE_BUF_USECONDS 1000
#define VTEP_DEFAULT_UDP_RECV_BUF        114688
#define VTEP_DEFAULT_UDP_SEND_BUF        114688

typedef struct vtep_server_s {
	/* General */
	uv_loop_t			*loop;							/* Event loop */

	/* Udp */
	char				*udp_listen_address;			/* Address for the udp server to listen to */
	int					udp_recv_buf;					/* udp receive buffer */
	int					udp_send_buf;					/* udp send buffer */
	async_io_t			udp_async_io;

	/* Stats */
	int					dump_stats_every;
	int					udp_send_count;
	unsigned long long	udp_send_amount;
	int					udp_recv_count;
	unsigned long long	udp_recv_amount;

	uv_timer_t			stats_timer;


	/* Configuration */
	char				*configfile;					/* Absolute config file path, or NULL */
	char				*pidfile;						/* PID file path */
	int					daemonize;						/* True if running as a daemon */

	/* Logging */
	int					verbosity;						/* Loglevel in vtep.conf */
	char				*logfile;						/* Path of log file */
	int					syslog_enabled;					/* Is syslog enabled? */
	char				*syslog_ident;					/* Syslog ident */
	int					syslog_facility;				/* Syslog facility */

	/* Networking */
	int					port;							/* TCP listening port */
	char				*bindaddr[VTEP_BINDADDR_MAX];	/* Addresses we should bind to */
	int					bindaddr_count;					/* Number of addresses in server.bindaddr[] */
	uv_tcp_t			*ipfd[VTEP_BINDADDR_MAX];		/* TCP socket file descriptors */
	int					ipfd_count;						/* Used slots in ipfd[] */
	int					maxidletime;					/* Client timeout in seconds */
	int					routing_enabled;				/* enable actual routing */

	/* Database */
	list				*nodes;							/* List of host nodes */
	list				*vxlan_ips;						/* List of vxlan_ips */

	/* VxLan */
	char				*vxlan_name;
	char				*vxlan_vni;
	char				*vxlan_group;
	char				*vxlan_port;
	char				*vxlan_interface;

	/* Save */
	char				*save_path;					/* Where to save things */
} vtep_server_t;

extern vtep_server_t server;

#endif