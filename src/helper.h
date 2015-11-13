// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet
/*
 * Copyright (c) 2015 Pagoda Box Inc
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public License, v.
 * 2.0. If a copy of the MPL was not distributed with this file, You can obtain one
 * at http://mozilla.org/MPL/2.0/.
 */

#ifndef REDD_HELPER_H
#define REDD_HELPER_H

#include <msgpack.h>
#include <stdint.h>
#include <uv.h>

typedef void (*save_data_function)(msgpack_packer *packer);
typedef void (*load_data_function)(msgpack_object object);


// helper functions
void	msgpack_pack_key_value(msgpack_packer *packer, char *key, 
			int key_len, char *value, int value_len);

char	*parse_ip_address(char *ip_address_string, int size);
char	*parse_host(char *host_string, int len);

int		validate_ip_address(char *ip_address);
int		validate_host(char *host);

char	*blank_ip_address();
char	*blank_host();

char	*pack_ip_address(char *ip_address);
char	*pack_host(char *host);

void	ngx_empty_into(ngx_queue_t *from,ngx_queue_t *to,int limit);

void	save_data(char *filename, save_data_function pack_function);
void	load_data(char *filename, load_data_function unpack_funciton);

#endif
