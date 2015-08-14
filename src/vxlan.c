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

#include "log.h"
#include "util/adlist.h"
#include "util/cmd.h"
#include "vtepd.h"
#include "vxlan.h"

void *dup_ip(void *ptr)
{
	return (void *)strdup((char *)ptr);
}

void free_ip(void *ptr)
{
	free((char *)ptr);
}

int match_ip(void *ptr, void *key)
{
	if (strcmp((char *)ptr, (char *)key) == 0)
		return 1;
	else
		return 0;
}

static int
vxlan_exists()
{
	char *cmd[] = {"ip", "link", "show", server.vxlan_name, 0};
	return run_cmd(cmd);
}

int
vxlan_init()
{
	/*
	 *  Run the following:
	 *  ip link add vxlan0 type vxlan id 1 group 239.0.0.1 dev eth0
	 *  ip link set vxlan0 up
	 */
	if (vxlan_exists != 0) {
		char *add_link_cmd[] = {"ip", "link", "add", server.vxlan_name, "type", "vxlan", "id", server.vxlan_vni, "dev", server.vxlan_interface, 0};
		if (run_cmd(add_link_cmd) != 0) {
			vtep_log(VTEPD_WARNING, "Failed to create link %s", server.vxlan_name);
			return -1;
		}
	}
	char *set_link_cmd[] = {"ip", "link", "set", server.vxlan_name, "up", 0};
	if (run_cmd(set_link_cmd) != 0)
	{
		vtep_log(VTEPD_WARNING, "Failed to set link %s up", server.vxlan_name);
		return -1;
	}

	char *set_route_cmd[] = {"bridge", "fdb", "add", "to", "00:00:00:00:00:00", "dst", server.vxlan_group, "via", server.tun_name, "dev", server.vxlan_name, 0};
	if (run_cmd(set_route_cmd) != 0)
	{
		vtep_log(VTEPD_WARNING, "Failed to set route %s", server.vxlan_name);
		return -1;
	}
	return 0;
}

static int
vxlan_has_ip(char *ip_address)
{
	char cmd[128];
	sprintf(&cmd, "ip addr show dev %s | grep %s", server.vxlan_name, ip_address);
	char *bash_cmd[] = {"bash","-c", cmd, 0};
	return run_cmd(bash_cmd);
}

int
vxlan_add_ip(char *ip_address)
{
	int ret = 0;
	if (!vxlan_has_ip(ip_address)) {
		char *ip = strdup(ip_address);
		listAddNodeTail(server.vxlan_ips, (void *)ip);
		char *cmd[] = {"ip", "addr", "add", ip_address, "dev", server.vxlan_name, 0};
		ret = run_cmd(cmd);
	}
	return ret;
}

int
vxlan_remove_ip(char *ip_address)
{
	int ret = 0;
	if (vxlan_has_ip(ip_address)) {
		listNode *node = listSearchKey(server.vxlan_ips, (void *)ip_address);
		if (node)
			listDelNode(server.vxlan_ips, node);
		char *cmd[] = {"ip", "addr", "del", ip_address, "dev", server.vxlan_name, 0};
		ret = run_cmd(cmd);
	}
	return ret;
}
