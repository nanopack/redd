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

// helper functions

#include <ctype.h>
#include <msgpack.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

#include "log.h"
#include "util/cmd.h"
#include "helper.h"
#include "vtepd.h"

void 
msgpack_pack_key_value(msgpack_packer *packer, char *key, int key_len, char *value, int value_len)
{
	msgpack_pack_raw(packer, key_len);
	msgpack_pack_raw_body(packer, key, key_len);
	msgpack_pack_raw(packer, value_len);
	msgpack_pack_raw_body(packer, value, value_len);
}

char
*parse_ip_address(char *ip_address_string, int len)
{
	char *ip_address = strndup(ip_address_string, len);
	return ip_address;
}

char
*parse_host(char *host_string, int len)
{
	char *host = strndup(host_string, len);
	return host;
}

int
validate_ip_address(char *ip_address)
{
	if (ip_address != NULL)
		return 1;
	else
		return 0;
}

int
validate_host(char *host)
{
	// add check to see if host lookup works?
	if (host != NULL)
		return 1;
	else
		return 0;
}

char
*blank_ip_address()
{
	return NULL;
}

char
*blank_host()
{
	return NULL;
}

char 
*pack_ip_address(char *ip_address)
{
	char *packed;
	packed = strdup(ip_address);
	return packed;
}

char 
*pack_host(char *host)
{
	char *packed;
	packed = strdup(host);
	return packed;
}

void
ngx_empty_into(ngx_queue_t *from,ngx_queue_t *to,int limit)
{
	ngx_queue_t *q;
	while (!ngx_queue_empty(from) && limit-- != 0) {
		q = ngx_queue_last(from);
		ngx_queue_remove(q);
		ngx_queue_insert_head(to,q);
	}
}

static char
*get_save_file_path(char *file_name)
{
	char *save_file_name;
	struct stat file_stat;
	int len;
	if (stat(server.save_path, &file_stat) != 0) {
		char *cmd[] = {"mkdir", "-p", server.save_path, 0};
		if (!run_cmd(cmd))
			return NULL;
	}
	if (server.save_path == NULL) {
		return NULL;
	}
	len = strlen(server.save_path);
	if (len == 0) {
		return NULL;
	}
	if (server.save_path[len - 1] == '/') {
		save_file_name = (char *)malloc(len + strlen(file_name) + 1);
		sprintf(save_file_name, "%s%s", server.save_path, file_name);
	} else {
		save_file_name = (char *)malloc(len + strlen(file_name) + 2);
		sprintf(save_file_name, "%s/%s", server.save_path, file_name);
	}
	return save_file_name;
}

void
save_data(char *filename, save_data_function pack_function)
{
	msgpack_sbuffer *buffer = NULL;
	msgpack_packer *packer  = NULL;
	char *save_file_name;
	FILE *save_file;

	if ((save_file_name = get_save_file_path(filename)) == NULL) {
		vtep_log(VTEPD_WARNING, "get_save_file_path returned NULL");
		return;
	}

	buffer = msgpack_sbuffer_new();
	msgpack_sbuffer_init(buffer);

	packer = msgpack_packer_new((void *)buffer, msgpack_sbuffer_write);

	msgpack_pack_map(packer, 1);
	pack_function(packer);

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
load_data(char *filename, load_data_function unpack_function)
{
	char *save_file_name;
	char *save_data;
	FILE *save_file;
	int len;
	struct stat file_stat;
	msgpack_zone *mempool;
	msgpack_object deserialized;

	if ((save_file_name = get_save_file_path(filename)) == NULL) {
		vtep_log(VTEPD_WARNING, "get_save_file_path returned NULL");
		return;
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

	unpack_function(deserialized);

	free(save_file_name);
	save_file_name = NULL;
	free(save_data);
	save_data = NULL;
	msgpack_zone_destroy(mempool);
	free(mempool);
	mempool = NULL;
}
