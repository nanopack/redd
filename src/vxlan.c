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
#include "vtep.h"
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
	return run_cmd(["ip", "link", "show", server.vxlan_name]);
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
		if (run_cmd(["ip", "link", "add", server.vxlan_name, "type", "vxlan", "id", server.vxlan_vni, "group", server.vxlan_group, "dev", server.vxlan_interface]) != 0) {
			vtep_log(VTEP_WARNING, "Failed to create link %s", server.vxlan_name);
			return -1;
		}
	}
	if (run_cmd(["ip", "link", "set", server.vxlan_name, "up"]) != 0)
	{
		vtep_log(VTEP_WARNING, "Failed to set link %s up", server.vxlan_name);
		return -1;
	}
	return 0;
}

static int
vxlan_has_ip(char *ip_address)
{
	char cmd[128];
	sprintf(&cmd, "'ip addr show dev %s | grep %s'", server.vxlan_name, ip_address);
	return run_cmd(["bash","-c", cmd]);
}

int
vxlan_add_ip(char *ip_address)
{
	int ret = 0;
	if (!vxlan_has_ip(ip_address)) {

		ret = run_cmd(["ip", "addr", "add", ip_address, "dev", "vxlan0"]);
	}
	return ret;
}

int
vxlan_remove_ip(char *ip_address)
{
	int ret = 0;
	if (vxlan_has_ip(ip_address)) {

		ret = run_cmd(["ip", "addr", "del", ip_address, "dev", "vxlan0"]);
	}
	return ret;
}
